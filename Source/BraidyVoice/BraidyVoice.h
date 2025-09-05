#pragma once

#include "../BraidyCore/BraidyTypes.h"
#include "../BraidyCore/MacroOscillator.h"
#include "../BraidyCore/BraidySettings.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace braidy {

// Enhanced ADSR envelope for Braidy voices
class ADSREnvelope {
public:
    enum class EnvelopeShape {
        LINEAR = 0,
        EXPONENTIAL,
        LOGARITHMIC,
        S_CURVE
    };

    ADSREnvelope();
    ~ADSREnvelope() = default;
    
    void Init();
    void Trigger();
    void Release();
    
    void SetAttack(float attack_ms);
    void SetDecay(float decay_ms);
    void SetSustain(float sustain_level);  // 0.0 to 1.0
    void SetRelease(float release_ms);
    void SetShape(EnvelopeShape shape);
    void SetSampleRate(float sample_rate);
    
    float Process();
    bool IsActive() const { return state_ != State::IDLE; }
    bool IsReleased() const { return state_ == State::RELEASE || state_ == State::IDLE; }
    
private:
    enum class State {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };
    
    State state_;
    float level_;
    float sustain_level_;
    float attack_increment_;
    float decay_increment_;
    float release_increment_;
    float sample_rate_;
    EnvelopeShape shape_;
    
    // Curve shaping
    float ApplyCurve(float linear_value) const;
};

// Simple AD envelope for compatibility (legacy)
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
    void NoteOff(int midi_note = -1);  // Support for specific note off
    void AllNotesOff();
    
    // Parameter updates
    void UpdateFromSettings(const BraidySettings& settings);
    
    // Audio generation
    void Process(float* output, int num_samples);
    
    // Voice state queries
    bool IsActive() const;
    int GetMidiNote() const { return midi_note_; }
    float GetVelocity() const { return velocity_; }
    uint32_t GetStartTime() const { return start_time_; }
    
    // Voice management
    void SetVoiceId(int voice_id) { voice_id_ = voice_id; }
    int GetVoiceId() const { return voice_id_; }
    
    // Modulation control
    void SetPitchBend(float pitch_bend);  // In semitones
    void SetAftertouch(float aftertouch); // 0.0 to 1.0
    void SetModWheel(float mod_wheel);    // 0.0 to 1.0
    void SetCC(int cc_number, float value);  // Generic CC handler
    
private:
    // Voice identification
    int voice_id_;
    int midi_note_;
    float velocity_;
    bool note_active_;
    uint32_t start_time_;  // For voice stealing algorithms
    
    // Modulation state
    float pitch_bend_;     // In semitones
    float aftertouch_;     // 0.0 to 1.0
    float mod_wheel_;      // 0.0 to 1.0
    std::array<float, 128> cc_values_;  // CC controller values
    
    // Audio processing
    float sample_rate_;
    MacroOscillator macro_oscillator_;
    ADSREnvelope adsr_envelope_;  // Enhanced ADSR envelope
    ADEnvelope envelope_;         // Legacy AD envelope (kept for compatibility)
    
    // Effects processing
    struct BitCrusher {
        float bits = 16.0f;
        float rate_reduction = 1.0f;
        float hold_sample = 0.0f;
        int hold_counter = 0;
        int hold_period = 1;
        
        float Process(float input);
        void SetBits(float bits_param);
        void SetRate(float rate_param);
    } bit_crusher_;
    
    struct WaveShaper {
        enum Type { SOFT_CLIP = 0, HARD_CLIP, FOLD, TUBE, ASYM };
        Type type = SOFT_CLIP;
        float amount = 0.0f;
        
        float Process(float input);
        void SetType(Type new_type);
        void SetAmount(float drive);
    } wave_shaper_;
    
    // Parameter caching
    MacroOscillatorShape current_shape_;
    int16_t current_timbre_;
    int16_t current_color_;
    float volume_;
    bool use_adsr_envelope_;  // Switch between ADSR and AD envelope modes
    
    // Audio buffers for processing
    int16_t temp_buffer_[kBlockSize];
    uint8_t sync_buffer_[kBlockSize];
    
    // Utility functions
    int16_t MidiNoteToInt16Pitch(int midi_note) const;
    
    DISALLOW_COPY_AND_ASSIGN(BraidyVoice);
};

}  // namespace braidy