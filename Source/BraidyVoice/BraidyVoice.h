#pragma once

#include "../BraidyCore/BraidyTypes.h"
#include "../BraidyCore/MacroOscillator.h"
#include "../BraidyCore/BraidySettings.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace braidy {

// Simple AD envelope for Braidy voices
class ADEnvelope {
public:
    ADEnvelope();
    ~ADEnvelope() = default;
    
    void Init();
    void Trigger();
    void Release();
    
    void SetAttack(float attack_ms);
    void SetDecay(float decay_ms);
    void SetSampleRate(float sample_rate);
    
    float Process();
    bool IsActive() const { return state_ != State::IDLE; }
    
private:
    enum class State {
        IDLE,
        ATTACK,
        DECAY
    };
    
    State state_;
    float level_;
    float attack_increment_;
    float decay_increment_;
    float sample_rate_;
};

// Individual voice in the polyphonic synthesizer
class BraidyVoice {
public:
    BraidyVoice();
    ~BraidyVoice() = default;
    
    void Init();
    void SetSampleRate(float sample_rate);
    
    // Voice control
    void NoteOn(int midi_note, float velocity);
    void NoteOff();
    void AllNotesOff();
    
    // Parameter updates
    void UpdateFromSettings(const BraidySettings& settings);
    
    // Audio generation
    void Process(float* output, int num_samples);
    
    // Voice state queries
    bool IsActive() const;
    int GetMidiNote() const { return midi_note_; }
    float GetVelocity() const { return velocity_; }
    
    // Voice management
    void SetVoiceId(int voice_id) { voice_id_ = voice_id; }
    int GetVoiceId() const { return voice_id_; }
    
private:
    // Voice identification
    int voice_id_;
    int midi_note_;
    float velocity_;
    bool note_active_;
    
    // Audio processing
    float sample_rate_;
    MacroOscillator macro_oscillator_;
    ADEnvelope envelope_;
    
    // Parameter caching
    MacroOscillatorShape current_shape_;
    int16_t current_timbre_;
    int16_t current_color_;
    float volume_;
    
    // Audio buffers for processing
    int16_t temp_buffer_[kBlockSize];
    uint8_t sync_buffer_[kBlockSize];
    
    // Utility functions
    int16_t MidiNoteToInt16Pitch(int midi_note) const;
    
    DISALLOW_COPY_AND_ASSIGN(BraidyVoice);
};

}  // namespace braidy