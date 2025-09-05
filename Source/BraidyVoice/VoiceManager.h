#pragma once

#include "BraidyVoice.h"
#include "../BraidyCore/BraidySettings.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace braidy {

class VoiceManager {
public:
    VoiceManager();
    ~VoiceManager() = default;
    
    void Init();
    void SetSampleRate(float sample_rate);
    
    // Voice management
    void SetMaxPolyphony(int max_voices);
    int GetMaxPolyphony() const { return max_polyphony_; }
    
    // MIDI handling
    void NoteOn(int midi_note, float velocity);
    void NoteOff(int midi_note);
    void AllNotesOff();
    void SetPitchBend(float pitch_bend);  // -1.0 to 1.0
    
    // Parameter updates
    void UpdateFromSettings(const BraidySettings& settings);
    
    // Audio processing
    void Process(juce::AudioBuffer<float>& buffer, int num_samples);
    
    // Voice status
    int GetActiveVoiceCount() const;
    bool IsVoiceActive(int voice_index) const;
    
private:
    // Voice allocation strategies
    enum class AllocationStrategy {
        OLDEST_NOTE_FIRST,  // Replace oldest note
        LOWEST_VELOCITY,    // Replace quietest note
        ROUND_ROBIN        // Cycle through voices
    };
    
    // Voice management
    std::array<BraidyVoice, kMaxPolyphony> voices_;
    int max_polyphony_;
    int next_voice_index_;  // For round-robin allocation
    AllocationStrategy allocation_strategy_;
    
    // MIDI state
    float current_pitch_bend_;
    std::array<bool, 128> held_notes_;  // Track which MIDI notes are held
    
    // Audio processing
    float sample_rate_;
    std::vector<float> voice_output_buffer_;
    
    // Internal methods
    int FindFreeVoice();
    int FindVoiceForNote(int midi_note);
    int AllocateVoice();
    int AllocateOldestVoice();
    int AllocateLowestVelocityVoice();
    int AllocateRoundRobinVoice();
    void ReclaimOldestVoice();
    
    DISALLOW_COPY_AND_ASSIGN(VoiceManager);
};

}  // namespace braidy