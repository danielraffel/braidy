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
    , volume_(1.0f)  // Full volume by default
    , use_adsr_envelope_(true)  // Default to ADSR envelope
    , vca_enabled_(true)  // Enable VCA so envelopes work correctly
    , current_gain_(0.0f)  // Start with zero gain for smooth fade-in
    , last_sample_(0.0f)  // DC blocking filter state
    , timbre_smooth_(0.0f)
    , color_smooth_(0.0f)
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
    
    // Initialize envelope modulation amounts
    envelope_fm_amount_ = 0.0f;
    envelope_timbre_amount_ = 0.0f;
    envelope_color_amount_ = 0.0f;
    
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
    DBG("=== BRAIDY VOICE NOTE ON ===");
    DBG("Voice ID: " + juce::String(voice_id_));
    DBG("MIDI Note: " + juce::String(midi_note));
    DBG("Velocity: " + juce::String(velocity));
    
    midi_note_ = midi_note;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    note_active_ = true;
    start_time_ = g_voice_time_counter++;
    
    // Set pitch for macro oscillator (including current pitch bend)
    int16_t pitch = MidiNoteToInt16Pitch(midi_note) + static_cast<int16_t>(pitch_bend_ * 128.0f);
    DBG("Setting pitch: " + juce::String(pitch));
    macro_oscillator_.set_pitch(pitch);
    
    // Trigger appropriate envelope
    if (use_adsr_envelope_) {
        DBG("Triggering ADSR envelope");
        adsr_envelope_.Trigger();
    } else {
        DBG("Triggering AD envelope");
        envelope_.Trigger();
    }
    
    // Strike the macro oscillator for percussion sounds
    DBG("Striking macro oscillator");
    macro_oscillator_.Strike();
    
    DBG("Voice is now active: " + juce::String(IsActive() ? "true" : "false"));
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
    // Update oscillator parameters
    MacroOscillatorShape new_shape = settings.GetShape();
    
    if (new_shape != current_shape_) {
        macro_oscillator_.set_shape(new_shape);
        current_shape_ = new_shape;
        
        // Apply waveform-specific parameter scaling
        ApplyWaveformSpecificSettings(new_shape, settings);
    }
    
    // Get raw parameters with smoothing
    int16_t new_timbre = settings.GetSmoothedTimbre();
    int16_t new_color = settings.GetSmoothedColor();
    
    // Apply additional smoothing to prevent clicks during parameter changes
    const float smooth_rate = 0.1f;
    timbre_smooth_ += (static_cast<float>(new_timbre) - timbre_smooth_) * smooth_rate;
    color_smooth_ += (static_cast<float>(new_color) - color_smooth_) * smooth_rate;
    
    int16_t smoothed_timbre = static_cast<int16_t>(timbre_smooth_);
    int16_t smoothed_color = static_cast<int16_t>(color_smooth_);
    
    if (smoothed_timbre != current_timbre_ || smoothed_color != current_color_) {
        macro_oscillator_.set_parameters(smoothed_timbre, smoothed_color);
        current_timbre_ = smoothed_timbre;
        current_color_ = smoothed_color;
    }
    
    // Update FM parameters
    int16_t fm_amount = static_cast<int16_t>(settings.GetParameter(BraidyParameter::FM_AMOUNT) * kParameterMax);
    int16_t fm_ratio = static_cast<int16_t>(settings.GetParameter(BraidyParameter::FM_RATIO) * kParameterMax / 8.0f);  // Scale 0.125-8 to 0-32767
    macro_oscillator_.set_fm_parameters(fm_amount, fm_ratio);
    
    // Update pitch from coarse and fine controls
    float coarse_pitch = settings.GetParameter(BraidyParameter::COARSE) * 12.0f;  // Convert octaves to semitones
    float fine_pitch = settings.GetParameter(BraidyParameter::FINE) * 0.01f;      // Convert cents to semitones
    float base_note = static_cast<float>(midi_note_ - 60);  // C4 = 60 = 0 semitones reference
    int16_t total_pitch = static_cast<int16_t>((base_note + coarse_pitch + fine_pitch + pitch_bend_) * 128.0f);
    macro_oscillator_.set_pitch(total_pitch);
    
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
    static int voiceProcessCounter = 0;
    
    // Only log first few calls or when note changes
    static int lastLoggedNote = -1;
    if (voiceProcessCounter++ < 10 || midi_note_ != lastLoggedNote) {
        if (IsActive()) {
            DBG("VOICE ACTIVE: Note=" + juce::String(midi_note_) + " samples=" + juce::String(num_samples));
            lastLoggedNote = midi_note_;
        }
    }
    
    if (!IsActive()) {
        // Voice is inactive - fade out smoothly to prevent clicks
        const float fade_rate = 0.99f;
        for (int i = 0; i < num_samples; ++i) {
            current_gain_ *= fade_rate;
            output[i] = 0.0f;
        }
        if (current_gain_ < 0.0001f) {
            current_gain_ = 0.0f;
        }
        return;
    }
    
    // Process in blocks of kBlockSize
    int samples_remaining = num_samples;
    int output_index = 0;
    
    while (samples_remaining > 0) {
        int block_size = std::min(samples_remaining, kBlockSize);
        
        // Calculate envelope level ONCE per block to avoid clicks
        float env_level = 1.0f;
        if (vca_enabled_) {
            if (use_adsr_envelope_) {
                env_level = adsr_envelope_.Process();
            } else {
                env_level = envelope_.Process();
            }
        }
        
        // Apply envelope modulation to parameters
        int16_t modulated_timbre = current_timbre_;
        int16_t modulated_color = current_color_;
        
        if (envelope_timbre_amount_ > 0.0f) {
            // Envelope modulates timbre
            int16_t timbre_mod = static_cast<int16_t>(env_level * envelope_timbre_amount_ * 32767.0f);
            int32_t modulated = current_timbre_ + timbre_mod;
            if (modulated < 0) modulated = 0;
            if (modulated > 32767) modulated = 32767;
            modulated_timbre = static_cast<int16_t>(modulated);
        }
        
        if (envelope_color_amount_ > 0.0f) {
            // Envelope modulates color
            int16_t color_mod = static_cast<int16_t>(env_level * envelope_color_amount_ * 32767.0f);
            int32_t modulated = current_color_ + color_mod;
            if (modulated < 0) modulated = 0;
            if (modulated > 32767) modulated = 32767;
            modulated_color = static_cast<int16_t>(modulated);
        }
        
        // Set modulated parameters
        macro_oscillator_.set_parameters(modulated_timbre, modulated_color);
        
        // TODO: Add FM modulation when FM input is implemented
        // if (envelope_fm_amount_ > 0.0f) {
        //     int16_t fm_mod = static_cast<int16_t>(env_level * envelope_fm_amount_ * 32767.0f);
        //     // Apply to FM input
        // }
        
        // Generate audio block using macro oscillator
        macro_oscillator_.Render(sync_buffer_, temp_buffer_, block_size);
        
        // Debug: Check if oscillator is producing any output
        static int osc_debug_counter = 0;
        if (osc_debug_counter++ % 100 == 0) {
            int16_t max_sample = 0;
            for (int i = 0; i < block_size; ++i) {
                int16_t abs_sample = std::abs(temp_buffer_[i]);
                if (abs_sample > max_sample) max_sample = abs_sample;
            }
            DBG("[OSC OUTPUT] Max sample in block: " << max_sample << " (expected ~32767 for full scale)");
        }
        
        // Calculate target gain and smoothing increment
        float target_gain = env_level * velocity_ * volume_;
        const float smooth_factor = 0.1f;  // Fast gain changes (was 0.001f - too slow!)
        
        // Debug logging for first sample of each block
        static int debug_counter = 0;
        if (debug_counter++ % 100 == 0) {  // Log every 100th block
            DBG("[GAIN DEBUG] env_level=" << env_level 
                << " velocity=" << velocity_ 
                << " volume=" << volume_ 
                << " target_gain=" << target_gain 
                << " current_gain=" << current_gain_);
        }
        
        // Process audio with smoothing
        for (int i = 0; i < block_size; ++i) {
            // Smooth gain transition to prevent clicks
            current_gain_ += (target_gain - current_gain_) * smooth_factor;
            
            // Convert from 16-bit to float with proper scaling
            float sample = static_cast<float>(temp_buffer_[i]) / 32767.0f;
            
            // Apply smoothed gain
            sample *= current_gain_;
            
            // Apply DC blocking to prevent clicks and pops
            float filtered = sample - last_sample_ * 0.995f;
            last_sample_ = sample;
            
            // Apply effects processing
            filtered = bit_crusher_.Process(filtered);
            filtered = wave_shaper_.Process(filtered);
            
            output[output_index + i] = filtered;
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
    // Convert MIDI note to internal pitch format used by Braids
    // Braids uses 128 units per semitone for fine pitch control
    // Center around C4 (MIDI note 60) for best range
    // Add offset to match Braids' pitch scaling
    const int center_note = 60;
    const int pitch_offset = (midi_note - center_note) * 128;
    return static_cast<int16_t>(pitch_offset + (60 << 7));  // Base pitch at C4
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

void BraidyVoice::ApplyWaveformSpecificSettings(MacroOscillatorShape shape, const BraidySettings& settings) {
    // Apply waveform-specific parameter scaling based on original Braids implementation
    // Each waveform has different parameter ranges and behaviors
    
    int16_t timbre = settings.GetTimbre();
    int16_t color = settings.GetColor();
    
    switch (shape) {
        case MacroOscillatorShape::CSAW:
        case MacroOscillatorShape::MORPH:
        case MacroOscillatorShape::SAW_SQUARE:
        case MacroOscillatorShape::SINE_TRIANGLE:
        case MacroOscillatorShape::BUZZ:
            // These analog-style waveforms work well with full parameter range
            macro_oscillator_.set_parameters(timbre, color);
            break;
            
        case MacroOscillatorShape::TRIPLE_SAW:
        case MacroOscillatorShape::TRIPLE_SQUARE:
        case MacroOscillatorShape::TRIPLE_TRIANGLE:
        case MacroOscillatorShape::TRIPLE_SINE:
        case MacroOscillatorShape::TRIPLE_RING_MOD:
            // Triple oscillators need specific detuning ranges
            // Timbre controls detuning amount
            macro_oscillator_.set_parameters(timbre >> 1, color);  // Reduce detuning range
            break;
            
        case MacroOscillatorShape::SAW_SWARM:
        case MacroOscillatorShape::SAW_COMB:
            // Swarm and comb need careful filtering
            // Color controls filter cutoff
            macro_oscillator_.set_parameters(timbre, std::min(color, int16_t(28000)));
            break;
            
        case MacroOscillatorShape::TOY:
            // Toy mode needs specific bit reduction settings
            // Timbre = sample rate reduction, Color = bit depth
            macro_oscillator_.set_parameters(timbre >> 2, color >> 2);
            break;
            
        case MacroOscillatorShape::DIGITAL_FILTER_LP:
        case MacroOscillatorShape::DIGITAL_FILTER_BP:
        case MacroOscillatorShape::DIGITAL_FILTER_HP:
            // Filter modes need resonance limiting to prevent instability
            macro_oscillator_.set_parameters(timbre, std::min(color, int16_t(30000)));
            break;
            
        case MacroOscillatorShape::VOSIM:
        case MacroOscillatorShape::VOWEL:
        case MacroOscillatorShape::VOWEL_FOF:
            // Vowel/formant modes need specific formant ranges
            macro_oscillator_.set_parameters(timbre, color);
            break;
            
        case MacroOscillatorShape::FM:
        case MacroOscillatorShape::FEEDBACK_FM:
        case MacroOscillatorShape::CHAOTIC_FEEDBACK_FM:
            // FM modes need careful modulation index control
            // Timbre = modulation index, Color = ratio
            macro_oscillator_.set_parameters(std::min(timbre, int16_t(25000)), color >> 1);
            break;
            
        case MacroOscillatorShape::PLUCKED:
        case MacroOscillatorShape::BOWED:
        case MacroOscillatorShape::BLOWN:
        case MacroOscillatorShape::FLUTED:
            // Physical modeling modes need damping control
            macro_oscillator_.set_parameters(timbre, color);
            macro_oscillator_.Strike();  // Retrigger excitation
            break;
            
        case MacroOscillatorShape::STRUCK_BELL:
        case MacroOscillatorShape::STRUCK_DRUM:
        case MacroOscillatorShape::KICK:
        case MacroOscillatorShape::SNARE:
        case MacroOscillatorShape::CYMBAL:
            // Percussion modes need specific decay settings
            macro_oscillator_.set_parameters(timbre >> 1, color);
            macro_oscillator_.Strike();  // Trigger percussion
            break;
            
        case MacroOscillatorShape::WAVETABLES:
        case MacroOscillatorShape::WAVE_MAP:
        case MacroOscillatorShape::WAVE_LINE:
        case MacroOscillatorShape::WAVE_PARAPHONIC:
            // Wavetable modes need position/morph control
            macro_oscillator_.set_parameters(timbre, color);
            break;
            
        case MacroOscillatorShape::FILTERED_NOISE:
        case MacroOscillatorShape::TWIN_PEAKS_NOISE:
        case MacroOscillatorShape::CLOCKED_NOISE:
        case MacroOscillatorShape::GRANULAR_CLOUD:
        case MacroOscillatorShape::PARTICLE_NOISE:
            // Noise modes need filtering/density control
            macro_oscillator_.set_parameters(timbre, std::min(color, int16_t(30000)));
            break;
            
        case MacroOscillatorShape::DIGITAL_MODULATION:
        case MacroOscillatorShape::QUESTION_MARK:
            // Special modes
            macro_oscillator_.set_parameters(timbre, color);
            break;
            
        default:
            // Default scaling
            macro_oscillator_.set_parameters(timbre, color);
            break;
    }
}

void BraidyVoice::SetMetaModeEnabled(bool enabled) {
    macro_oscillator_.SetMetaMode(enabled);
}

void BraidyVoice::SetQuantizerEnabled(bool enabled) {
    macro_oscillator_.GetQuantizer().setEnabled(enabled);
}

void BraidyVoice::SetBitCrusherEnabled(bool enabled) {
    macro_oscillator_.GetBitCrusher().setEnabled(enabled);
}

}  // namespace braidy