#include "VoiceManager.h"
#include <algorithm>

namespace braidy {

VoiceManager::VoiceManager() 
    : max_polyphony_(kMaxPolyphony)
    , next_voice_index_(0)
    , allocation_strategy_(AllocationStrategy::OLDEST_NOTE_FIRST)
    , current_pitch_bend_(0.0f)
    , sample_rate_(44100.0f)
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
    
    next_voice_index_ = 0;
    current_pitch_bend_ = 0.0f;
    
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

void VoiceManager::NoteOn(int midi_note, float velocity) {
    if (midi_note < 0 || midi_note >= 128) {
        return;  // Invalid MIDI note
    }
    
    // Check if this note is already playing
    int existing_voice = FindVoiceForNote(midi_note);
    if (existing_voice >= 0) {
        // Retrigger the existing voice
        voices_[existing_voice].NoteOn(midi_note, velocity);
        held_notes_[midi_note] = true;
        return;
    }
    
    // Find a free voice or allocate one
    int voice_index = FindFreeVoice();
    if (voice_index < 0) {
        voice_index = AllocateVoice();
    }
    
    if (voice_index >= 0 && voice_index < max_polyphony_) {
        voices_[voice_index].NoteOn(midi_note, velocity);
        held_notes_[midi_note] = true;
    }
}

void VoiceManager::NoteOff(int midi_note) {
    if (midi_note < 0 || midi_note >= 128) {
        return;
    }
    
    held_notes_[midi_note] = false;
    
    // Find the voice playing this note
    int voice_index = FindVoiceForNote(midi_note);
    if (voice_index >= 0) {
        voices_[voice_index].NoteOff();
    }
}

void VoiceManager::AllNotesOff() {
    for (int i = 0; i < max_polyphony_; ++i) {
        voices_[i].AllNotesOff();
    }
    
    held_notes_.fill(false);
}

void VoiceManager::SetPitchBend(float pitch_bend) {
    current_pitch_bend_ = std::clamp(pitch_bend, -1.0f, 1.0f);
    
    // TODO: Apply pitch bend to active voices
    // This would require modifying the MacroOscillator pitch in real-time
}

void VoiceManager::UpdateFromSettings(const BraidySettings& settings) {
    // Update all voices with current settings
    for (int i = 0; i < max_polyphony_; ++i) {
        voices_[i].UpdateFromSettings(settings);
    }
}

void VoiceManager::Process(juce::AudioBuffer<float>& buffer, int num_samples) {
    int num_channels = buffer.getNumChannels();
    
    // Ensure our temp buffer is large enough
    if (voice_output_buffer_.size() < static_cast<size_t>(num_samples)) {
        voice_output_buffer_.resize(num_samples);
    }
    
    // Clear the output buffer
    buffer.clear();
    
    // Process each active voice
    for (int voice_idx = 0; voice_idx < max_polyphony_; ++voice_idx) {
        if (voices_[voice_idx].IsActive()) {
            // Process the voice into our temp buffer
            voices_[voice_idx].Process(voice_output_buffer_.data(), num_samples);
            
            // Mix into the output buffer
            for (int channel = 0; channel < num_channels; ++channel) {
                float* channel_data = buffer.getWritePointer(channel);
                
                for (int sample = 0; sample < num_samples; ++sample) {
                    channel_data[sample] += voice_output_buffer_[sample];
                }
            }
        }
    }
    
    // Apply master volume scaling if we have many voices playing
    int active_voices = GetActiveVoiceCount();
    if (active_voices > 1) {
        float voice_scaling = 1.0f / std::sqrt(static_cast<float>(active_voices));
        buffer.applyGain(voice_scaling);
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
    if (voice_index >= 0 && voice_index < max_polyphony_) {
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
    return -1;  // No free voice found
}

int VoiceManager::FindVoiceForNote(int midi_note) {
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetMidiNote() == midi_note) {
            return i;
        }
    }
    return -1;  // Note not found
}

int VoiceManager::AllocateVoice() {
    switch (allocation_strategy_) {
        case AllocationStrategy::OLDEST_NOTE_FIRST:
            return AllocateOldestVoice();
            
        case AllocationStrategy::LOWEST_VELOCITY:
            return AllocateLowestVelocityVoice();
            
        case AllocationStrategy::ROUND_ROBIN:
        default:
            return AllocateRoundRobinVoice();
    }
}

int VoiceManager::AllocateOldestVoice() {
    // Find the voice that has been playing the longest
    // For simplicity, just use round-robin for now
    return AllocateRoundRobinVoice();
}

int VoiceManager::AllocateLowestVelocityVoice() {
    int lowest_voice = -1;
    float lowest_velocity = 2.0f;  // Higher than any possible velocity
    
    for (int i = 0; i < max_polyphony_; ++i) {
        if (voices_[i].IsActive() && voices_[i].GetVelocity() < lowest_velocity) {
            lowest_velocity = voices_[i].GetVelocity();
            lowest_voice = i;
        }
    }
    
    return (lowest_voice >= 0) ? lowest_voice : AllocateRoundRobinVoice();
}

int VoiceManager::AllocateRoundRobinVoice() {
    int voice_index = next_voice_index_;
    next_voice_index_ = (next_voice_index_ + 1) % max_polyphony_;
    return voice_index;
}

void VoiceManager::ReclaimOldestVoice() {
    // Force the oldest voice to stop
    int voice_to_reclaim = AllocateOldestVoice();
    if (voice_to_reclaim >= 0) {
        voices_[voice_to_reclaim].AllNotesOff();
    }
}

}  // namespace braidy