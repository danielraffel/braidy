#pragma once

#include "BraidyVoice.h"
#include "../BraidyCore/BraidySettings.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace braidy {

class VoiceManager {
public:
    // Forward declarations for public interface
    enum class AllocationStrategy {
        OLDEST_NOTE_FIRST,  // Replace oldest note
        LOWEST_VELOCITY,    // Replace quietest note
        HIGHEST_NOTE,       // Replace highest pitch note
        LOWEST_NOTE,        // Replace lowest pitch note
        ROUND_ROBIN         // Cycle through voices
    };
    
    // MPE configuration
    struct MPESettings {
        bool enabled = false;
        int master_channel = 1;  // 1-based
        int start_channel = 2;   // 1-based
        int end_channel = 16;    // 1-based
        float pitch_bend_range = 2.0f;  // semitones
    };
    
    VoiceManager();
    ~VoiceManager() = default;
    
    void Init();
    void SetSampleRate(float sample_rate);
    
    // Voice management
    void SetMaxPolyphony(int max_voices);
    int GetMaxPolyphony() const { return max_polyphony_; }
    
    // MIDI handling
    void NoteOn(int midi_note, float velocity, int mpe_channel = 0);
    void NoteOff(int midi_note, int mpe_channel = 0);
    void AllNotesOff();
    void SetPitchBend(float pitch_bend, int channel = -1);  // -1.0 to 1.0, channel -1 for global
    void SetAftertouch(float aftertouch, int midi_note = -1, int channel = -1);  // 0.0 to 1.0
    void SetModWheel(float mod_wheel, int channel = -1);  // 0.0 to 1.0
    void SetCC(int cc_number, float value, int channel = -1);  // Generic CC handler
    
    // Parameter updates
    void UpdateFromSettings(const BraidySettings& settings);
    
    // Voice allocation and MPE settings
    void SetAllocationStrategy(AllocationStrategy strategy) { allocation_strategy_ = strategy; }
    AllocationStrategy GetAllocationStrategy() const { return allocation_strategy_; }
    void SetMPESettings(const MPESettings& settings);
    const MPESettings& GetMPESettings() const { return mpe_settings_; }
    void SetGlobalPitchBendRange(float range) { global_pitch_bend_range_ = range; }
    float GetGlobalPitchBendRange() const { return global_pitch_bend_range_; }
    
    // Audio processing
    void Process(juce::AudioBuffer<float>& buffer, int num_samples);
    
    // Voice status
    int GetActiveVoiceCount() const;
    bool IsVoiceActive(int voice_index) const;
    
private:
    
    // Voice management
    std::array<BraidyVoice, kMaxPolyphony> voices_;
    int max_polyphony_;
    int next_voice_index_;  // For round-robin allocation
    AllocationStrategy allocation_strategy_;
    
    // MIDI state
    std::array<float, 16> channel_pitch_bend_;     // Per-channel pitch bend
    std::array<float, 16> channel_aftertouch_;     // Per-channel channel aftertouch
    std::array<float, 16> channel_mod_wheel_;      // Per-channel mod wheel (CC1)
    std::array<std::array<float, 128>, 16> channel_cc_;  // Per-channel CC values
    std::array<std::array<float, 128>, 16> note_aftertouch_;  // Per-note aftertouch
    std::array<bool, 128> held_notes_;             // Track which MIDI notes are held
    std::array<int, 128> note_to_voice_;           // Map MIDI note to voice index
    std::array<int, 128> note_to_channel_;         // Map MIDI note to MPE channel
    
    // MPE configuration
    MPESettings mpe_settings_;
    
    // Pitch bend configuration
    float global_pitch_bend_range_;  // Global pitch bend range in semitones
    
    // Audio processing
    float sample_rate_;
    std::vector<float> voice_output_buffer_;
    
    // Internal methods
    int FindFreeVoice();
    int FindVoiceForNote(int midi_note);
    int AllocateVoice();
    int AllocateOldestVoice();
    int AllocateLowestVelocityVoice();
    int AllocateHighestNoteVoice();
    int AllocateLowestNoteVoice();
    int AllocateRoundRobinVoice();
    void ReclaimOldestVoice();
    
    // MPE helpers
    bool IsMPEChannel(int channel) const;
    int GetMPEChannelForNote(int midi_note);
    void UpdateVoiceModulation(int voice_index);
    
    // CC routing
    void RouteCC(int cc_number, float value, int channel, int voice_index = -1);
    
    DISALLOW_COPY_AND_ASSIGN(VoiceManager);
};

}  // namespace braidy