#include "VoiceManager.h"
#include <algorithm>
#include <cmath>

namespace braidy {

VoiceManager::VoiceManager() 
    : max_polyphony_(kMaxPolyphony)
    , next_voice_index_(0)
    , allocation_strategy_(AllocationStrategy::OLDEST_NOTE_FIRST)
    , sample_rate_(44100.0f)
    , global_pitch_bend_range_(2.0f)
{
    Init();
}

void VoiceManager::Init() {
    // Initialize all voices
    for (int i = 0; i < kMaxPolyphony; ++i) {
        voices_[i].Init();
        voices_[i].SetVoiceId(i);
    }
    
    // Clear MIDI note tracking
    held_notes_.fill(false);
    note_to_voice_.fill(-1);
    note_to_channel_.fill(-1);
    
    // Initialize MIDI state arrays
    channel_pitch_bend_.fill(0.0f);
    channel_aftertouch_.fill(0.0f);
    channel_mod_wheel_.fill(0.0f);
    
    for (int ch = 0; ch < 16; ++ch) {
        channel_cc_[ch].fill(0.0f);
        note_aftertouch_[ch].fill(0.0f);
    }
    
    next_voice_index_ = 0;
    
    // Initialize MPE settings
    mpe_settings_.enabled = false;
    mpe_settings_.master_channel = 1;
    mpe_settings_.start_channel = 2;
    mpe_settings_.end_channel = 16;
    mpe_settings_.pitch_bend_range = 2.0f;
    
    // Initialize audio buffer
    voice_output_buffer_.resize(1024);  // Will be resized as needed
}

void VoiceManager::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    
    for (int i = 0; i < kMaxPolyphony; ++i) {
        voices_[i].SetSampleRate(sample_rate);
    }
}

void VoiceManager::SetMaxPolyphony(int max_voices) {
    max_polyphony_ = std::clamp(max_voices, 1, kMaxPolyphony);
    
    // Turn off voices beyond the new limit
    for (int i = max_polyphony_; i < kMaxPolyphony; ++i) {
        voices_[i].AllNotesOff();
    }
}

void VoiceManager::NoteOn(int midi_note, float velocity, int mpe_channel) {
    if (midi_note < 0 || midi_note > 127) return;
    
    // Handle MPE channel assignment
    int channel = mpe_channel;
    if (mpe_settings_.enabled && mpe_channel == 0) {
        channel = GetMPEChannelForNote(midi_note);
    }
    
    // Find an existing voice for this note or allocate a new one
    int voice_index = FindVoiceForNote(midi_note);
    if (voice_index == -1) {
        voice_index = AllocateVoice();
    }
    
    if (voice_index != -1) {
        // Assign note to voice
        note_to_voice_[midi_note] = voice_index;
        note_to_channel_[midi_note] = channel;
        held_notes_[midi_note] = true;
        
        // Start the note
        voices_[voice_index].NoteOn(midi_note, velocity);
        
        // Apply current modulation state for this channel
        UpdateVoiceModulation(voice_index);
    }
}

void VoiceManager::NoteOff(int midi_note, int mpe_channel) {
    if (midi_note < 0 || midi_note > 127) return;
    
    int voice_index = note_to_voice_[midi_note];
    if (voice_index != -1 && voice_index < kMaxPolyphony) {
        voices_[voice_index].NoteOff(midi_note);
        note_to_voice_[midi_note] = -1;
        note_to_channel_[midi_note] = -1;
    }
    
    held_notes_[midi_note] = false;
}

void VoiceManager::AllNotesOff() {
    for (int i = 0; i < kMaxPolyphony; ++i) {
        voices_[i].AllNotesOff();
    }
    
    held_notes_.fill(false);
    note_to_voice_.fill(-1);
    note_to_channel_.fill(-1);
}

void VoiceManager::SetPitchBend(float pitch_bend, int channel) {
    // Clamp pitch bend to valid range
    pitch_bend = std::clamp(pitch_bend, -1.0f, 1.0f);
    
    if (channel == -1) {
        // Global pitch bend - apply to all channels
        for (int ch = 0; ch < 16; ++ch) {
            channel_pitch_bend_[ch] = pitch_bend;
        }
        
        // Update all active voices
        for (int i = 0; i < kMaxPolyphony; ++i) {
            if (voices_[i].IsActive()) {
                UpdateVoiceModulation(i);
            }
        }
    } else if (channel >= 0 && channel < 16) {
        // Per-channel pitch bend
        channel_pitch_bend_[channel] = pitch_bend;
        
        // Update voices on this channel
        for (int note = 0; note < 128; ++note) {
            if (note_to_channel_[note] == channel) {
                int voice_index = note_to_voice_[note];
                if (voice_index != -1) {
                    UpdateVoiceModulation(voice_index);
                }
            }
        }
    }
}

void VoiceManager::SetAftertouch(float aftertouch, int midi_note, int channel) {
    aftertouch = std::clamp(aftertouch, 0.0f, 1.0f);
    
    if (midi_note != -1 && channel != -1) {
        // Per-note aftertouch
        if (channel >= 0 && channel < 16 && midi_note >= 0 && midi_note < 128) {
            note_aftertouch_[channel][midi_note] = aftertouch;
            
            int voice_index = note_to_voice_[midi_note];
            if (voice_index != -1) {
                UpdateVoiceModulation(voice_index);
            }
        }
    } else if (channel != -1) {
        // Channel aftertouch
        if (channel >= 0 && channel < 16) {
            channel_aftertouch_[channel] = aftertouch;
            
            // Update all voices on this channel
            for (int note = 0; note < 128; ++note) {
                if (note_to_channel_[note] == channel) {
                    int voice_index = note_to_voice_[note];
                    if (voice_index != -1) {
                        UpdateVoiceModulation(voice_index);
                    }
                }
            }
        }
    }
}

void VoiceManager::SetModWheel(float mod_wheel, int channel) {
    mod_wheel = std::clamp(mod_wheel, 0.0f, 1.0f);
    
    if (channel == -1) {
        // Global mod wheel
        for (int ch = 0; ch < 16; ++ch) {
            channel_mod_wheel_[ch] = mod_wheel;
        }
        
        // Update all active voices
        for (int i = 0; i < kMaxPolyphony; ++i) {
            if (voices_[i].IsActive()) {
                UpdateVoiceModulation(i);
            }
        }
    } else if (channel >= 0 && channel < 16) {
        // Per-channel mod wheel
        channel_mod_wheel_[channel] = mod_wheel;
        
        // Update voices on this channel
        for (int note = 0; note < 128; ++note) {
            if (note_to_channel_[note] == channel) {
                int voice_index = note_to_voice_[note];
                if (voice_index != -1) {
                    UpdateVoiceModulation(voice_index);
                }
            }
        }
    }
}

void VoiceManager::SetCC(int cc_number, float value, int channel) {
    value = std::clamp(value, 0.0f, 1.0f);
    
    if (cc_number < 0 || cc_number > 127) return;
    
    // Handle special CCs
    if (cc_number == 1) {  // Mod wheel
        SetModWheel(value, channel);
        return;
    }
    
    if (channel == -1) {
        // Global CC - apply to all channels
        for (int ch = 0; ch < 16; ++ch) {
            channel_cc_[ch][cc_number] = value;
            RouteCC(cc_number, value, ch);
        }
    } else if (channel >= 0 && channel < 16) {
        // Per-channel CC
        channel_cc_[channel][cc_number] = value;
        RouteCC(cc_number, value, channel);
    }
}

void VoiceManager::UpdateFromSettings(const BraidySettings& settings) {
    for (int i = 0; i < kMaxPolyphony; ++i) {
        voices_[i].UpdateFromSettings(settings);
    }
}

void VoiceManager::SetMPESettings(const MPESettings& settings) {
    mpe_settings_ = settings;
    
    // Clamp channel values
    mpe_settings_.master_channel = std::clamp(settings.master_channel, 1, 16);
    mpe_settings_.start_channel = std::clamp(settings.start_channel, 1, 16);
    mpe_settings_.end_channel = std::clamp(settings.end_channel, 1, 16);
    
    // Ensure start <= end
    if (mpe_settings_.start_channel > mpe_settings_.end_channel) {
        std::swap(mpe_settings_.start_channel, mpe_settings_.end_channel);
    }
}

void VoiceManager::Process(juce::AudioBuffer<float>& buffer, int num_samples) {
    buffer.clear();
    
    if (voice_output_buffer_.size() < static_cast<size_t>(num_samples)) {
        voice_output_buffer_.resize(num_samples);
    }
    
    // Process each voice
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive()) {
            // Clear the voice buffer
            std::fill(voice_output_buffer_.begin(), 
                     voice_output_buffer_.begin() + num_samples, 0.0f);
            
            // Process the voice
            voices_[i].Process(voice_output_buffer_.data(), num_samples);
            
            // Add to output buffer
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                buffer.addFrom(ch, 0, voice_output_buffer_.data(), num_samples);
            }
        }
    }
}

int VoiceManager::GetActiveVoiceCount() const {
    int count = 0;
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive()) {
            ++count;
        }
    }
    return count;
}

bool VoiceManager::IsVoiceActive(int voice_index) const {
    if (voice_index >= 0 && voice_index < kMaxPolyphony) {
        return voices_[voice_index].IsActive();
    }
    return false;
}

int VoiceManager::FindFreeVoice() {
    for (int i = 0; i < max_polyphony_; ++i) {
        if (!voices_[i].IsActive()) {
            return i;
        }
    }
    return -1;
}

int VoiceManager::FindVoiceForNote(int midi_note) {
    return note_to_voice_[midi_note];
}

int VoiceManager::AllocateVoice() {
    // First try to find a free voice
    int free_voice = FindFreeVoice();
    if (free_voice != -1) {
        return free_voice;
    }
    
    // No free voices, use allocation strategy
    switch (allocation_strategy_) {
        case AllocationStrategy::OLDEST_NOTE_FIRST:
            return AllocateOldestVoice();
        case AllocationStrategy::LOWEST_VELOCITY:
            return AllocateLowestVelocityVoice();
        case AllocationStrategy::HIGHEST_NOTE:
            return AllocateHighestNoteVoice();
        case AllocationStrategy::LOWEST_NOTE:
            return AllocateLowestNoteVoice();
        case AllocationStrategy::ROUND_ROBIN:
            return AllocateRoundRobinVoice();
        default:
            return AllocateOldestVoice();
    }
}

int VoiceManager::AllocateOldestVoice() {
    int oldest_voice = -1;
    uint32_t oldest_time = UINT32_MAX;
    
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetStartTime() < oldest_time) {
            oldest_time = voices_[i].GetStartTime();
            oldest_voice = i;
        }
    }
    
    return oldest_voice;
}

int VoiceManager::AllocateLowestVelocityVoice() {
    int quietest_voice = -1;
    float lowest_velocity = 1.1f;
    
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetVelocity() < lowest_velocity) {
            lowest_velocity = voices_[i].GetVelocity();
            quietest_voice = i;
        }
    }
    
    return quietest_voice;
}

int VoiceManager::AllocateHighestNoteVoice() {
    int highest_voice = -1;
    int highest_note = -1;
    
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetMidiNote() > highest_note) {
            highest_note = voices_[i].GetMidiNote();
            highest_voice = i;
        }
    }
    
    return highest_voice;
}

int VoiceManager::AllocateLowestNoteVoice() {
    int lowest_voice = -1;
    int lowest_note = 128;
    
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetMidiNote() < lowest_note) {
            lowest_note = voices_[i].GetMidiNote();
            lowest_voice = i;
        }
    }
    
    return lowest_voice;
}

int VoiceManager::AllocateRoundRobinVoice() {
    int voice = next_voice_index_;
    next_voice_index_ = (next_voice_index_ + 1) % max_polyphony_;
    return voice;
}

void VoiceManager::ReclaimOldestVoice() {
    int oldest_voice = AllocateOldestVoice();
    if (oldest_voice != -1) {
        voices_[oldest_voice].AllNotesOff();
    }
}

bool VoiceManager::IsMPEChannel(int channel) const {
    if (!mpe_settings_.enabled) return false;
    
    // Convert to 0-based for comparison
    int ch = channel + 1;
    return ch >= mpe_settings_.start_channel && ch <= mpe_settings_.end_channel;
}

int VoiceManager::GetMPEChannelForNote(int midi_note) {
    if (!mpe_settings_.enabled) return 0;
    
    // Simple round-robin assignment to MPE channels
    int mpe_channel_count = mpe_settings_.end_channel - mpe_settings_.start_channel + 1;
    int channel_offset = midi_note % mpe_channel_count;
    
    // Convert back to 0-based
    return (mpe_settings_.start_channel + channel_offset) - 1;
}

void VoiceManager::UpdateVoiceModulation(int voice_index) {
    if (voice_index < 0 || voice_index >= kMaxPolyphony) return;
    
    BraidyVoice& voice = voices_[voice_index];
    if (!voice.IsActive()) return;
    
    int midi_note = voice.GetMidiNote();
    int channel = note_to_channel_[midi_note];
    if (channel < 0 || channel >= 16) channel = 0;
    
    // Calculate effective pitch bend
    float pitch_bend_range = mpe_settings_.enabled ? mpe_settings_.pitch_bend_range : global_pitch_bend_range_;
    float pitch_bend = channel_pitch_bend_[channel] * pitch_bend_range;
    voice.SetPitchBend(pitch_bend);
    
    // Apply aftertouch (per-note takes precedence over channel)
    float aftertouch = note_aftertouch_[channel][midi_note];
    if (aftertouch == 0.0f) {
        aftertouch = channel_aftertouch_[channel];
    }
    voice.SetAftertouch(aftertouch);
    
    // Apply mod wheel
    voice.SetModWheel(channel_mod_wheel_[channel]);
}

void VoiceManager::RouteCC(int cc_number, float value, int channel, int voice_index) {
    // Route specific CCs to voice parameters
    if (voice_index != -1) {
        // Route to specific voice
        if (voice_index >= 0 && voice_index < kMaxPolyphony) {
            voices_[voice_index].SetCC(cc_number, value);
        }
    } else {
        // Route to all voices on this channel
        for (int note = 0; note < 128; ++note) {
            if (note_to_channel_[note] == channel) {
                int v_index = note_to_voice_[note];
                if (v_index != -1) {
                    voices_[v_index].SetCC(cc_number, value);
                }
            }
        }
    }
}

}  // namespace braidy