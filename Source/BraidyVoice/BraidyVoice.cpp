#include "BraidyVoice.h"
#include "../BraidyCore/BraidyMath.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <juce_core/juce_core.h>

namespace braidy {

// Global time counter for voice stealing
static uint32_t g_voice_time_counter = 0;

// ADSREnvelope implementation
ADSREnvelope::ADSREnvelope() 
    : state_(State::IDLE)
    , level_(0.0f)
    , sustain_level_(0.6f)
    , sample_rate_(44100.0f)
    , shape_(EnvelopeShape::EXPONENTIAL)
{
    SetAttack(10.0f);   // 10ms default attack
    SetDecay(200.0f);   // 200ms default decay
    SetRelease(500.0f); // 500ms default release
}

void ADSREnvelope::Init() {
    state_ = State::IDLE;
    level_ = 0.0f;
}

void ADSREnvelope::Trigger() {
    state_ = State::ATTACK;
    level_ = 0.0f;
}

void ADSREnvelope::Release() {
    if (state_ != State::IDLE) {
        state_ = State::RELEASE;
    }
}

void ADSREnvelope::SetAttack(float attack_ms) {
    if (attack_ms > 0.0f) {
        attack_increment_ = 1000.0f / (attack_ms * sample_rate_);
    } else {
        attack_increment_ = 1.0f;  // Instant attack
    }
}

void ADSREnvelope::SetDecay(float decay_ms) {
    if (decay_ms > 0.0f) {
        decay_increment_ = (1.0f - sustain_level_) * 1000.0f / (decay_ms * sample_rate_);
    } else {
        decay_increment_ = 1.0f;  // Instant decay
    }
}

void ADSREnvelope::SetSustain(float sustain_level) {
    sustain_level_ = std::clamp(sustain_level, 0.0f, 1.0f);
    // Recalculate decay increment
    SetDecay(1000.0f / (decay_increment_ * sample_rate_));
}

void ADSREnvelope::SetRelease(float release_ms) {
    if (release_ms > 0.0f) {
        release_increment_ = 1000.0f / (release_ms * sample_rate_);
    } else {
        release_increment_ = 1.0f;  // Instant release
    }
}

void ADSREnvelope::SetShape(EnvelopeShape shape) {
    shape_ = shape;
}

void ADSREnvelope::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    // Recalculate increments
    SetAttack(1000.0f / (attack_increment_ * sample_rate_));
    SetDecay(1000.0f / (decay_increment_ * sample_rate_));
    SetRelease(1000.0f / (release_increment_ * sample_rate_));
}

float ADSREnvelope::Process() {
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
            if (level_ <= sustain_level_) {
                level_ = sustain_level_;
                state_ = State::SUSTAIN;
            }
            break;
            
        case State::SUSTAIN:
            level_ = sustain_level_;
            break;
            
        case State::RELEASE:
            level_ -= release_increment_;
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
    
    return ApplyCurve(level_);
}

float ADSREnvelope::ApplyCurve(float linear_value) const {
    switch (shape_) {
        case EnvelopeShape::LINEAR:
            return linear_value;
            
        case EnvelopeShape::EXPONENTIAL:
            return linear_value * linear_value;
            
        case EnvelopeShape::LOGARITHMIC:
            return std::sqrt(linear_value);
            
        case EnvelopeShape::S_CURVE:
            // Smooth S-curve using cubic interpolation
            return linear_value * linear_value * (3.0f - 2.0f * linear_value);
            
        default:
            return linear_value;
    }
}

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
    , start_time_(0)
    , pitch_bend_(0.0f)
    , aftertouch_(0.0f)
    , mod_wheel_(0.0f)
    , sample_rate_(44100.0f)
    , current_shape_(MacroOscillatorShape::CSAW)
    , current_timbre_(0)
    , current_color_(0)
    , volume_(0.7f)
    , use_adsr_envelope_(true)  // Default to ADSR envelope
{
    Init();
}

void BraidyVoice::Init() {
    macro_oscillator_.Init();
    envelope_.Init();
    adsr_envelope_.Init();
    
    midi_note_ = -1;
    velocity_ = 0.0f;
    note_active_ = false;
    start_time_ = 0;
    
    // Initialize modulation
    pitch_bend_ = 0.0f;
    aftertouch_ = 0.0f;
    mod_wheel_ = 0.0f;
    cc_values_.fill(0.0f);
    
    // Initialize effects
    bit_crusher_.SetBits(16.0f);
    bit_crusher_.SetRate(1.0f);
    wave_shaper_.SetAmount(0.0f);
    wave_shaper_.SetType(WaveShaper::SOFT_CLIP);
    
    // Initialize buffers
    for (int i = 0; i < kBlockSize; ++i) {
        temp_buffer_[i] = 0;
        sync_buffer_[i] = 0;
    }
}

void BraidyVoice::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    envelope_.SetSampleRate(sample_rate);
    adsr_envelope_.SetSampleRate(sample_rate);
}

void BraidyVoice::NoteOn(int midi_note, float velocity) {
    midi_note_ = midi_note;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    note_active_ = true;
    start_time_ = g_voice_time_counter++;
    
    // Set pitch for macro oscillator (including current pitch bend)
    int16_t pitch = MidiNoteToInt16Pitch(midi_note) + static_cast<int16_t>(pitch_bend_ * 128.0f);
    macro_oscillator_.set_pitch(pitch);
    
    // Trigger appropriate envelope
    if (use_adsr_envelope_) {
        adsr_envelope_.Trigger();
    } else {
        envelope_.Trigger();
    }
    
    // Strike the macro oscillator for percussion sounds
    macro_oscillator_.Strike();
}

void BraidyVoice::NoteOff(int midi_note) {
    // If specific note is provided, only release if it matches
    if (midi_note != -1 && midi_note != midi_note_) {
        return;
    }
    
    note_active_ = false;
    if (use_adsr_envelope_) {
        adsr_envelope_.Release();
    } else {
        envelope_.Release();
    }
}

void BraidyVoice::AllNotesOff() {
    note_active_ = false;
    if (use_adsr_envelope_) {
        adsr_envelope_.Release();
    } else {
        envelope_.Release();
    }
    midi_note_ = -1;
    velocity_ = 0.0f;
}

void BraidyVoice::UpdateFromSettings(const BraidySettings& settings) {
    DBG("=== BRAIDY VOICE UPDATE FROM SETTINGS DEBUG ===");
    
    // Update oscillator parameters
    MacroOscillatorShape new_shape = settings.GetShape();
    DBG("Voice " + juce::String(voice_id_) + " - Current shape: " + juce::String(static_cast<int>(current_shape_)));
    DBG("Voice " + juce::String(voice_id_) + " - New shape: " + juce::String(static_cast<int>(new_shape)));
    
    if (new_shape != current_shape_) {
        DBG("Voice " + juce::String(voice_id_) + " - Shape changed! Setting macro_oscillator shape to: " + juce::String(static_cast<int>(new_shape)));
        macro_oscillator_.set_shape(new_shape);
        current_shape_ = new_shape;
        DBG("Voice " + juce::String(voice_id_) + " - macro_oscillator_.set_shape() called");
    } else {
        DBG("Voice " + juce::String(voice_id_) + " - Shape unchanged, skipping set_shape call");
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
    
    if (use_adsr_envelope_) {
        // Update ADSR parameters
        float sustain = settings.GetParameter(BraidyParameter::ENVELOPE_SUSTAIN);
        float release = settings.GetParameter(BraidyParameter::ENVELOPE_RELEASE) * 3000.0f;  // Convert to ms
        int envelope_shape = static_cast<int>(settings.GetParameter(BraidyParameter::ENVELOPE_SHAPE));
        
        adsr_envelope_.SetAttack(attack);
        adsr_envelope_.SetDecay(decay);
        adsr_envelope_.SetSustain(sustain);
        adsr_envelope_.SetRelease(release);
        adsr_envelope_.SetShape(static_cast<ADSREnvelope::EnvelopeShape>(envelope_shape));
    } else {
        // Update AD parameters
        envelope_.SetAttack(attack);
        envelope_.SetDecay(decay);
    }
    
    // Update effects parameters
    float bit_crusher_bits = settings.GetParameter(BraidyParameter::BIT_CRUSHER_BITS);
    float bit_crusher_rate = settings.GetParameter(BraidyParameter::BIT_CRUSHER_RATE);
    float waveshaper_amount = settings.GetParameter(BraidyParameter::WAVESHAPER_AMOUNT);
    int waveshaper_type = static_cast<int>(settings.GetParameter(BraidyParameter::WAVESHAPER_TYPE));
    
    bit_crusher_.SetBits(bit_crusher_bits);
    bit_crusher_.SetRate(bit_crusher_rate);
    wave_shaper_.SetAmount(waveshaper_amount);
    wave_shaper_.SetType(static_cast<WaveShaper::Type>(waveshaper_type));
    
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
            float env_level;
            if (use_adsr_envelope_) {
                env_level = adsr_envelope_.Process();
            } else {
                env_level = envelope_.Process();
            }
            
            float sample = static_cast<float>(temp_buffer_[i]) / 32768.0f;  // Convert to float
            sample *= env_level * velocity_ * volume_;
            
            // Apply effects processing
            sample = bit_crusher_.Process(sample);
            sample = wave_shaper_.Process(sample);
            
            output[output_index + i] = sample;
        }
        
        samples_remaining -= block_size;
        output_index += block_size;
    }
}

bool BraidyVoice::IsActive() const {
    if (use_adsr_envelope_) {
        return adsr_envelope_.IsActive();
    } else {
        return envelope_.IsActive();
    }
}

int16_t BraidyVoice::MidiNoteToInt16Pitch(int midi_note) const {
    // Convert MIDI note to internal pitch format
    // Internal format: semitone * 128 (7-bit fractional)
    return static_cast<int16_t>(midi_note * 128);
}

void BraidyVoice::SetPitchBend(float pitch_bend) {
    pitch_bend_ = pitch_bend;
    
    // Update oscillator pitch if voice is active
    if (note_active_ && midi_note_ != -1) {
        int16_t base_pitch = MidiNoteToInt16Pitch(midi_note_);
        int16_t bent_pitch = base_pitch + static_cast<int16_t>(pitch_bend * 128.0f);
        macro_oscillator_.set_pitch(bent_pitch);
    }
}

void BraidyVoice::SetAftertouch(float aftertouch) {
    aftertouch_ = std::clamp(aftertouch, 0.0f, 1.0f);
    
    // Apply aftertouch to timbre modulation
    if (note_active_) {
        int16_t modulated_timbre = current_timbre_ + static_cast<int16_t>(aftertouch * 16384.0f);
        modulated_timbre = std::clamp(modulated_timbre, static_cast<int16_t>(0), static_cast<int16_t>(32767));
        macro_oscillator_.set_parameters(modulated_timbre, current_color_);
    }
}

void BraidyVoice::SetModWheel(float mod_wheel) {
    mod_wheel_ = std::clamp(mod_wheel, 0.0f, 1.0f);
    
    // Apply mod wheel to color modulation
    if (note_active_) {
        int16_t modulated_color = current_color_ + static_cast<int16_t>(mod_wheel * 16384.0f);
        modulated_color = std::clamp(modulated_color, static_cast<int16_t>(0), static_cast<int16_t>(32767));
        macro_oscillator_.set_parameters(current_timbre_, modulated_color);
    }
}

void BraidyVoice::SetCC(int cc_number, float value) {
    if (cc_number >= 0 && cc_number < 128) {
        cc_values_[cc_number] = std::clamp(value, 0.0f, 1.0f);
        
        // Route specific CCs to parameters
        switch (cc_number) {
            case 7:   // Volume
                volume_ = value * 0.8f;
                break;
            case 74:  // Timbre (common CC for filter cutoff)
                if (note_active_) {
                    int16_t cc_timbre = static_cast<int16_t>(value * 32767.0f);
                    macro_oscillator_.set_parameters(cc_timbre, current_color_);
                }
                break;
            case 71:  // Color (common CC for resonance)
                if (note_active_) {
                    int16_t cc_color = static_cast<int16_t>(value * 32767.0f);
                    macro_oscillator_.set_parameters(current_timbre_, cc_color);
                }
                break;
            // Add more CC mappings as needed
        }
    }
}

// BitCrusher implementation
float BraidyVoice::BitCrusher::Process(float input) {
    // Sample rate reduction
    if (rate_reduction > 1.0f) {
        if (hold_counter <= 0) {
            hold_sample = input;
            hold_counter = static_cast<int>(rate_reduction);
        } else {
            --hold_counter;
        }
        input = hold_sample;
    }
    
    // Bit depth reduction
    if (bits < 16.0f) {
        float levels = std::pow(2.0f, bits);
        float quantized = std::floor(input * levels + 0.5f) / levels;
        return std::clamp(quantized, -1.0f, 1.0f);
    }
    
    return input;
}

void BraidyVoice::BitCrusher::SetBits(float bits_param) {
    bits = std::clamp(bits_param, 1.0f, 16.0f);
}

void BraidyVoice::BitCrusher::SetRate(float rate_param) {
    rate_reduction = std::clamp(rate_param, 1.0f, 32.0f);
    hold_period = static_cast<int>(rate_reduction);
}

// WaveShaper implementation
float BraidyVoice::WaveShaper::Process(float input) {
    if (amount <= 0.0f) {
        return input;  // Bypass if no drive
    }
    
    float driven = input * (1.0f + amount * 9.0f);  // Scale drive 0-10x
    
    switch (type) {
        case SOFT_CLIP:
            // Soft clipping using tanh
            return std::tanh(driven) * (1.0f / (1.0f + amount * 0.3f));
            
        case HARD_CLIP:
            // Hard clipping
            return std::clamp(driven, -1.0f, 1.0f);
            
        case FOLD:
            // Wave folding
            {
                float folded = driven;
                while (folded > 1.0f) folded = 2.0f - folded;
                while (folded < -1.0f) folded = -2.0f - folded;
                return folded;
            }
            
        case TUBE:
            // Tube-style saturation
            {
                float abs_x = std::abs(driven);
                float sign = driven >= 0.0f ? 1.0f : -1.0f;
                if (abs_x < 1.0f/3.0f) {
                    return 2.0f * driven;
                } else if (abs_x < 2.0f/3.0f) {
                    return sign * (3.0f - std::pow(2.0f - 3.0f * abs_x, 2.0f)) / 3.0f;
                } else {
                    return sign;
                }
            }
            
        case ASYM:
            // Asymmetric distortion
            if (driven >= 0.0f) {
                return std::tanh(driven * 1.2f) * 0.8f;
            } else {
                return std::tanh(driven * 0.8f) * 1.2f;
            }
            
        default:
            return input;
    }
}

void BraidyVoice::WaveShaper::SetType(Type new_type) {
    type = new_type;
}

void BraidyVoice::WaveShaper::SetAmount(float drive) {
    amount = std::clamp(drive, 0.0f, 1.0f);
}

}  // namespace braidy