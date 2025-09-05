#include "BraidyVoice.h"
#include "../BraidyCore/BraidyMath.h"
#include <cmath>
#include <algorithm>

namespace braidy {

// ADEnvelope implementation
ADEnvelope::ADEnvelope() : state_(State::IDLE), level_(0.0f), sample_rate_(44100.0f) {
    SetAttack(10.0f);   // 10ms default attack
    SetDecay(500.0f);   // 500ms default decay
}

void ADEnvelope::Init() {
    state_ = State::IDLE;
    level_ = 0.0f;
}

void ADEnvelope::Trigger() {
    state_ = State::ATTACK;
    level_ = 0.0f;
}

void ADEnvelope::Release() {
    if (state_ != State::IDLE) {
        state_ = State::DECAY;
    }
}

void ADEnvelope::SetAttack(float attack_ms) {
    if (attack_ms > 0.0f) {
        attack_increment_ = 1000.0f / (attack_ms * sample_rate_);
    } else {
        attack_increment_ = 1.0f;  // Instant attack
    }
}

void ADEnvelope::SetDecay(float decay_ms) {
    if (decay_ms > 0.0f) {
        decay_increment_ = 1000.0f / (decay_ms * sample_rate_);
    } else {
        decay_increment_ = 1.0f;  // Instant decay
    }
}

void ADEnvelope::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    // Recalculate increments
    SetAttack(1000.0f / (attack_increment_ * sample_rate_));
    SetDecay(1000.0f / (decay_increment_ * sample_rate_));
}

float ADEnvelope::Process() {
    switch (state_) {
        case State::ATTACK:
            level_ += attack_increment_;
            if (level_ >= 1.0f) {
                level_ = 1.0f;
                state_ = State::DECAY;
            }
            break;
            
        case State::DECAY:
            level_ -= decay_increment_;
            if (level_ <= 0.0f) {
                level_ = 0.0f;
                state_ = State::IDLE;
            }
            break;
            
        case State::IDLE:
        default:
            level_ = 0.0f;
            break;
    }
    
    return level_;
}

// BraidyVoice implementation
BraidyVoice::BraidyVoice() 
    : voice_id_(0)
    , midi_note_(-1)
    , velocity_(0.0f)
    , note_active_(false)
    , sample_rate_(44100.0f)
    , current_shape_(MacroOscillatorShape::CSAW)
    , current_timbre_(0)
    , current_color_(0)
    , volume_(0.7f)
{
    Init();
}

void BraidyVoice::Init() {
    macro_oscillator_.Init();
    envelope_.Init();
    
    midi_note_ = -1;
    velocity_ = 0.0f;
    note_active_ = false;
    
    // Initialize buffers
    for (int i = 0; i < kBlockSize; ++i) {
        temp_buffer_[i] = 0;
        sync_buffer_[i] = 0;
    }
}

void BraidyVoice::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    envelope_.SetSampleRate(sample_rate);
}

void BraidyVoice::NoteOn(int midi_note, float velocity) {
    midi_note_ = midi_note;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    note_active_ = true;
    
    // Set pitch for macro oscillator
    int16_t pitch = MidiNoteToInt16Pitch(midi_note);
    macro_oscillator_.set_pitch(pitch);
    
    // Trigger envelope
    envelope_.Trigger();
    
    // Strike the macro oscillator for percussion sounds
    macro_oscillator_.Strike();
}

void BraidyVoice::NoteOff() {
    note_active_ = false;
    envelope_.Release();
}

void BraidyVoice::AllNotesOff() {
    note_active_ = false;
    envelope_.Release();
    midi_note_ = -1;
    velocity_ = 0.0f;
}

void BraidyVoice::UpdateFromSettings(const BraidySettings& settings) {
    // Update oscillator parameters
    MacroOscillatorShape new_shape = settings.GetShape();
    if (new_shape != current_shape_) {
        macro_oscillator_.set_shape(new_shape);
        current_shape_ = new_shape;
    }
    
    // Update timbre and color
    int16_t new_timbre = settings.GetSmoothedTimbre();
    int16_t new_color = settings.GetSmoothedColor();
    
    if (new_timbre != current_timbre_ || new_color != current_color_) {
        macro_oscillator_.set_parameters(new_timbre, new_color);
        current_timbre_ = new_timbre;
        current_color_ = new_color;
    }
    
    // Update envelope parameters
    float attack = settings.GetParameter(BraidyParameter::ATTACK) * 1000.0f;  // Convert to ms
    float decay = settings.GetParameter(BraidyParameter::DECAY) * 2000.0f;   // Convert to ms
    
    envelope_.SetAttack(attack);
    envelope_.SetDecay(decay);
    
    // Update volume
    volume_ = settings.GetParameter(BraidyParameter::VOLUME);
}

void BraidyVoice::Process(float* output, int num_samples) {
    if (!IsActive()) {
        // Voice is inactive - output silence
        for (int i = 0; i < num_samples; ++i) {
            output[i] = 0.0f;
        }
        return;
    }
    
    // Process in blocks of kBlockSize
    int samples_remaining = num_samples;
    int output_index = 0;
    
    while (samples_remaining > 0) {
        int block_size = std::min(samples_remaining, kBlockSize);
        
        // Generate audio block using macro oscillator
        macro_oscillator_.Render(sync_buffer_, temp_buffer_, block_size);
        
        // Apply envelope and convert to float
        for (int i = 0; i < block_size; ++i) {
            float env_level = envelope_.Process();
            float sample = static_cast<float>(temp_buffer_[i]) / 32768.0f;  // Convert to float
            sample *= env_level * velocity_ * volume_;
            output[output_index + i] = sample;
        }
        
        samples_remaining -= block_size;
        output_index += block_size;
    }
}

bool BraidyVoice::IsActive() const {
    return envelope_.IsActive();
}

int16_t BraidyVoice::MidiNoteToInt16Pitch(int midi_note) const {
    // Convert MIDI note to internal pitch format
    // Internal format: semitone * 128 (7-bit fractional)
    return static_cast<int16_t>(midi_note * 128);
}

}  // namespace braidy