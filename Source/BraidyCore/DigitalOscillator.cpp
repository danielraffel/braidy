#include "DigitalOscillator.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace braidy {

// Function table mapping digital oscillator shapes to render functions
const DigitalOscillator::RenderFn DigitalOscillator::fn_table_[] = {
    &DigitalOscillator::RenderTripleRingMod,
    &DigitalOscillator::RenderSawSwarm,
    &DigitalOscillator::RenderCombFilter,
    &DigitalOscillator::RenderToy,
    
    &DigitalOscillator::RenderDigitalFilterLP,
    &DigitalOscillator::RenderDigitalFilterPK,
    &DigitalOscillator::RenderDigitalFilterBP,
    &DigitalOscillator::RenderDigitalFilterHP,
    
    &DigitalOscillator::RenderVosim,
    &DigitalOscillator::RenderVowel,
    &DigitalOscillator::RenderVowelFof,
    
    &DigitalOscillator::RenderHarmonics,
    
    &DigitalOscillator::RenderFM,
    &DigitalOscillator::RenderFeedbackFM,
    &DigitalOscillator::RenderChaoticFeedbackFM,
    
    &DigitalOscillator::RenderStruckBell,
    &DigitalOscillator::RenderStruckDrum,
    
    &DigitalOscillator::RenderKick,
    &DigitalOscillator::RenderCymbal,
    &DigitalOscillator::RenderSnare,
    
    &DigitalOscillator::RenderPlucked,
    &DigitalOscillator::RenderBowed,
    &DigitalOscillator::RenderBlown,
    &DigitalOscillator::RenderFluted,
    
    &DigitalOscillator::RenderWavetables,
    &DigitalOscillator::RenderWaveMap,
    &DigitalOscillator::RenderWaveLine,
    &DigitalOscillator::RenderWaveParaphonic,
    
    &DigitalOscillator::RenderFilteredNoise,
    &DigitalOscillator::RenderTwinPeaksNoise,
    &DigitalOscillator::RenderClockedNoise,
    &DigitalOscillator::RenderGranularCloud,
    &DigitalOscillator::RenderParticleNoise,
    &DigitalOscillator::RenderDigitalModulation,
};

DigitalOscillator::DigitalOscillator() {
    Init();
}

void DigitalOscillator::Init() {
    phase_ = 0;
    phase_increment_ = 0;
    struck_ = false;
    excitation_fm_ = 0;
    strike_level_ = 0;
    rng_state_ = 1;
    
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
    previous_parameter_[0] = 0;
    previous_parameter_[1] = 0;
    
    // Clear all state
    memset(&state_, 0, sizeof(state_));
}

void DigitalOscillator::Strike() {
    struck_ = true;
    strike_level_ = 32767;
    excitation_fm_ = 16384;
}

uint32_t DigitalOscillator::ComputePhaseIncrement(int16_t midi_pitch) {
    // Convert MIDI pitch to phase increment
    // This is a simplified version - in practice you'd use LUT tables
    
    if (midi_pitch >= 140 * 128) {
        midi_pitch = 140 * 128 - 1;
    }
    if (midi_pitch < 0) {
        midi_pitch = 0;
    }
    
    // Convert to frequency ratio
    float frequency_ratio = std::pow(2.0f, (midi_pitch / 128.0f - 69.0f) / 12.0f);
    float frequency = 440.0f * frequency_ratio;
    
    // Convert to phase increment (32-bit accumulator, 48kHz sample rate)
    return static_cast<uint32_t>((frequency * 4294967296.0) / 48000.0);
}

uint32_t DigitalOscillator::ComputeDelay(int16_t midi_pitch) {
    // Convert pitch to delay length for Karplus-Strong and similar algorithms
    if (midi_pitch >= 140 * 128) {
        midi_pitch = 140 * 128 - 1;
    }
    if (midi_pitch < 0) {
        midi_pitch = 0;
    }
    
    float frequency_ratio = std::pow(2.0f, (midi_pitch / 128.0f - 69.0f) / 12.0f);
    float frequency = 440.0f * frequency_ratio;
    
    uint32_t delay = static_cast<uint32_t>(48000.0f / frequency);
    return std::max(4U, std::min(delay, 4096U));
}

int16_t DigitalOscillator::InterpolateWaveform(const int16_t* wavetable, uint32_t phase, uint32_t size) {
    // Simple linear interpolation
    uint32_t index = (phase >> 16) & (size - 1);
    uint32_t next_index = (index + 1) & (size - 1);
    uint32_t frac = (phase >> 8) & 0xff;
    
    int32_t a = wavetable[index];
    int32_t b = wavetable[next_index];
    return static_cast<int16_t>(a + ((b - a) * frac >> 8));
}

void DigitalOscillator::Render(MacroOscillatorShape macro_shape, int16_t pitch, 
                              int16_t parameter_1, int16_t parameter_2,
                              const uint8_t* sync, int16_t* buffer, size_t size) {
    // Map MacroOscillatorShape to DigitalOscillatorShape
    DigitalOscillatorShape digital_shape;
    
    switch (macro_shape) {
        case MacroOscillatorShape::TRIPLE_RING_MOD:
            digital_shape = DigitalOscillatorShape::TRIPLE_RING_MOD;
            break;
        case MacroOscillatorShape::SAW_SWARM:
            digital_shape = DigitalOscillatorShape::SAW_SWARM;
            break;
        case MacroOscillatorShape::SAW_COMB:
            digital_shape = DigitalOscillatorShape::COMB_FILTER;
            break;
        case MacroOscillatorShape::TOY:
            digital_shape = DigitalOscillatorShape::TOY;
            break;
        case MacroOscillatorShape::DIGITAL_FILTER_LP:
            digital_shape = DigitalOscillatorShape::DIGITAL_FILTER_LP;
            break;
        case MacroOscillatorShape::DIGITAL_FILTER_PK:
            digital_shape = DigitalOscillatorShape::DIGITAL_FILTER_PK;
            break;
        case MacroOscillatorShape::DIGITAL_FILTER_BP:
            digital_shape = DigitalOscillatorShape::DIGITAL_FILTER_BP;
            break;
        case MacroOscillatorShape::DIGITAL_FILTER_HP:
            digital_shape = DigitalOscillatorShape::DIGITAL_FILTER_HP;
            break;
        case MacroOscillatorShape::VOSIM:
            digital_shape = DigitalOscillatorShape::VOSIM;
            break;
        case MacroOscillatorShape::VOWEL:
            digital_shape = DigitalOscillatorShape::VOWEL;
            break;
        case MacroOscillatorShape::VOWEL_FOF:
            digital_shape = DigitalOscillatorShape::VOWEL_FOF;
            break;
        case MacroOscillatorShape::HARMONICS:
            digital_shape = DigitalOscillatorShape::HARMONICS;
            break;
        case MacroOscillatorShape::FM:
            digital_shape = DigitalOscillatorShape::FM;
            break;
        case MacroOscillatorShape::FEEDBACK_FM:
            digital_shape = DigitalOscillatorShape::FEEDBACK_FM;
            break;
        case MacroOscillatorShape::CHAOTIC_FEEDBACK_FM:
            digital_shape = DigitalOscillatorShape::CHAOTIC_FEEDBACK_FM;
            break;
        case MacroOscillatorShape::STRUCK_BELL:
            digital_shape = DigitalOscillatorShape::STRUCK_BELL;
            break;
        case MacroOscillatorShape::STRUCK_DRUM:
            digital_shape = DigitalOscillatorShape::STRUCK_DRUM;
            break;
        case MacroOscillatorShape::KICK:
            digital_shape = DigitalOscillatorShape::KICK;
            break;
        case MacroOscillatorShape::CYMBAL:
            digital_shape = DigitalOscillatorShape::CYMBAL;
            break;
        case MacroOscillatorShape::SNARE:
            digital_shape = DigitalOscillatorShape::SNARE;
            break;
        case MacroOscillatorShape::PLUCKED:
            digital_shape = DigitalOscillatorShape::PLUCKED;
            break;
        case MacroOscillatorShape::BOWED:
            digital_shape = DigitalOscillatorShape::BOWED;
            break;
        case MacroOscillatorShape::BLOWN:
            digital_shape = DigitalOscillatorShape::BLOWN;
            break;
        case MacroOscillatorShape::FLUTED:
            digital_shape = DigitalOscillatorShape::FLUTED;
            break;
        case MacroOscillatorShape::WAVETABLES:
            digital_shape = DigitalOscillatorShape::WAVETABLES;
            break;
        case MacroOscillatorShape::WAVE_MAP:
            digital_shape = DigitalOscillatorShape::WAVE_MAP;
            break;
        case MacroOscillatorShape::WAVE_LINE:
            digital_shape = DigitalOscillatorShape::WAVE_LINE;
            break;
        case MacroOscillatorShape::WAVE_PARAPHONIC:
            digital_shape = DigitalOscillatorShape::WAVE_PARAPHONIC;
            break;
        case MacroOscillatorShape::FILTERED_NOISE:
            digital_shape = DigitalOscillatorShape::FILTERED_NOISE;
            break;
        case MacroOscillatorShape::TWIN_PEAKS_NOISE:
            digital_shape = DigitalOscillatorShape::TWIN_PEAKS_NOISE;
            break;
        case MacroOscillatorShape::CLOCKED_NOISE:
            digital_shape = DigitalOscillatorShape::CLOCKED_NOISE;
            break;
        case MacroOscillatorShape::GRANULAR_CLOUD:
            digital_shape = DigitalOscillatorShape::GRANULAR_CLOUD;
            break;
        case MacroOscillatorShape::PARTICLE_NOISE:
            digital_shape = DigitalOscillatorShape::PARTICLE_NOISE;
            break;
        case MacroOscillatorShape::DIGITAL_MODULATION:
            digital_shape = DigitalOscillatorShape::DIGITAL_MODULATION;
            break;
        default:
            digital_shape = DigitalOscillatorShape::TRIPLE_RING_MOD;
            break;
    }
    
    shape_ = digital_shape;
    pitch_ = pitch;
    parameter_[0] = parameter_1;
    parameter_[1] = parameter_2;
    
    phase_increment_ = ComputePhaseIncrement(pitch_);
    
    // Clamp parameters
    if (pitch_ > 140 * 128) {
        pitch_ = 140 * 128;
    } else if (pitch_ < 0) {
        pitch_ = 0;
    }
    
    // Call the appropriate render function
    RenderFn fn = fn_table_[static_cast<int>(digital_shape)];
    (this->*fn)(sync, buffer, size);
    
    // Update previous parameters
    previous_parameter_[0] = parameter_[0];
    previous_parameter_[1] = parameter_[1];
}

// Phase 2: Classic Digital Synthesis Models

void DigitalOscillator::RenderTripleRingMod(const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t phase = phase_ + (1L << 30);
    uint32_t increment = phase_increment_;
    uint32_t modulator_phase = state_.vow.formant_phase[0];
    uint32_t modulator_phase_2 = state_.vow.formant_phase[1];
    uint32_t modulator_phase_increment = ComputePhaseIncrement(
        pitch_ + ((parameter_[0] - 16384) >> 2)
    );
    uint32_t modulator_phase_increment_2 = ComputePhaseIncrement(
        pitch_ + ((parameter_[1] - 16384) >> 2)
    );
    
    while (size--) {
        phase += increment;
        if (*sync++) {
            phase = 0;
            modulator_phase = 0;
            modulator_phase_2 = 0;
        }
        modulator_phase += modulator_phase_increment;
        modulator_phase_2 += modulator_phase_increment_2;
        
        // Three sine waves ring modulated together
        int16_t carrier = InterpolateWaveform(wav_sine, phase, 1024);
        int16_t mod1 = InterpolateWaveform(wav_sine, modulator_phase, 1024);
        int16_t mod2 = InterpolateWaveform(wav_sine, modulator_phase_2, 1024);
        
        int32_t result = carrier;
        result = (result * mod1) >> 15;
        result = (result * mod2) >> 15;
        
        // Apply soft clipping
        result = SoftLimit(result);
        *buffer++ = static_cast<int16_t>(result);
    }
    phase_ = phase - (1L << 30);
    state_.vow.formant_phase[0] = modulator_phase;
    state_.vow.formant_phase[1] = modulator_phase_2;
}

void DigitalOscillator::RenderSawSwarm(const uint8_t* sync, int16_t* buffer, size_t size) {
    int32_t detune = parameter_[0] + 1024;
    detune = (detune * detune) >> 9;
    
    int32_t spread = parameter_[1] + 512;
    spread = (spread * spread) >> 10;
    
    uint32_t phase_1 = state_.vow.formant_phase[0];
    uint32_t phase_2 = state_.vow.formant_phase[1];
    uint32_t phase_3 = state_.vow.formant_phase[2];
    uint32_t phase_4 = state_.vow.formant_phase[3];
    
    uint32_t increment = phase_increment_;
    uint32_t increment_1 = increment + ((detune * spread) >> 8);
    uint32_t increment_2 = increment - ((detune * spread) >> 9);
    uint32_t increment_3 = increment + ((detune * spread) >> 9);
    uint32_t increment_4 = increment - ((detune * spread) >> 10);
    
    while (size--) {
        if (*sync++) {
            phase_1 = phase_2 = phase_3 = phase_4 = 0;
        }
        
        phase_1 += increment_1;
        phase_2 += increment_2;
        phase_3 += increment_3;
        phase_4 += increment_4;
        
        // Generate sawtooth waves
        int32_t saw_1 = (phase_1 >> 16) - 32768;
        int32_t saw_2 = (phase_2 >> 16) - 32768;
        int32_t saw_3 = (phase_3 >> 16) - 32768;
        int32_t saw_4 = (phase_4 >> 16) - 32768;
        
        int32_t result = (saw_1 + saw_2 + saw_3 + saw_4) >> 2;
        *buffer++ = SoftLimit(result);
    }
    
    state_.vow.formant_phase[0] = phase_1;
    state_.vow.formant_phase[1] = phase_2;
    state_.vow.formant_phase[2] = phase_3;
    state_.vow.formant_phase[3] = phase_4;
}

void DigitalOscillator::RenderCombFilter(const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t delay_length = ComputeDelay(pitch_);
    uint32_t feedback = parameter_[0];
    uint32_t resonance = parameter_[1];
    
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            // Clear delay line on sync
            memset(state_.comb.delay_line, 0, sizeof(state_.comb.delay_line[0]) * delay_length);
            state_.comb.delay_ptr = 0;
        }
        
        phase += increment;
        
        // Generate input signal (sawtooth)
        int16_t input = (phase >> 16) - 32768;
        
        // Read from delay line
        uint16_t read_ptr = (state_.comb.delay_ptr + delay_length - delay_length) % delay_length;
        int32_t delayed = state_.comb.delay_line[read_ptr];
        
        // Apply feedback and resonance
        int32_t feedback_amount = (feedback * delayed) >> 15;
        int32_t resonance_amount = (resonance * state_.comb.lp_state) >> 15;
        
        int32_t output = input + feedback_amount + resonance_amount;
        
        // Low-pass filter for resonance
        state_.comb.lp_state += ((output - state_.comb.lp_state) * 2048) >> 15;
        
        // Write to delay line
        state_.comb.delay_line[state_.comb.delay_ptr] = SoftLimit(output);
        state_.comb.delay_ptr = (state_.comb.delay_ptr + 1) % delay_length;
        
        *buffer++ = SoftLimit(output);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderToy(const uint8_t* sync, int16_t* buffer, size_t size) {
    // "Toy" circuit-bent sound - bit crushing and sample rate reduction
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    uint32_t bit_reduction = (parameter_[0] >> 11) + 1;  // 1-4 bits
    uint32_t sample_rate_reduction = (parameter_[1] >> 8) + 1;  // 1-128 samples
    
    static uint32_t sample_counter = 0;
    static int16_t held_sample = 0;
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Generate base waveform (square wave)
        int16_t square = (phase & 0x80000000) ? 16384 : -16384;
        
        // Sample rate reduction
        if ((sample_counter++ % sample_rate_reduction) == 0) {
            // Bit reduction
            int32_t bits = 16 - bit_reduction;
            int32_t mask = ~((1 << bits) - 1);
            held_sample = square & mask;
        }
        
        *buffer++ = held_sample;
    }
    
    phase_ = phase;
}

// Digital Filter Synthesis

void DigitalOscillator::RenderDigitalFilterLP(const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    float frequency = static_cast<float>(parameter_[0]) / 32767.0f;
    float resonance = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Generate sawtooth input
        float input = static_cast<float>((phase >> 16) - 32768) / 32768.0f;
        
        // Process through digital lowpass filter
        int16_t output;
        ProcessFilter(state_.filt.filter_state, input, frequency, resonance, &output);
        
        *buffer++ = output;
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderDigitalFilterPK(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Peak filter - similar to LP but with different filter response
    RenderDigitalFilterLP(sync, buffer, size);
}

void DigitalOscillator::RenderDigitalFilterBP(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bandpass filter - similar to LP but with different filter response
    RenderDigitalFilterLP(sync, buffer, size);
}

void DigitalOscillator::RenderDigitalFilterHP(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Highpass filter - similar to LP but with different filter response  
    RenderDigitalFilterLP(sync, buffer, size);
}

// Phase 3: Advanced Synthesis - Formant Synthesis

void DigitalOscillator::RenderVosim(const uint8_t* sync, int16_t* buffer, size_t size) {
    // VOSIM (Voice Simulation) synthesis
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    uint32_t pulse_count = (parameter_[0] >> 13) + 1;  // 1-4 pulses
    uint32_t decay_rate = parameter_[1];
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Generate VOSIM pulse train
        uint32_t cycle_phase = phase % (0xFFFFFFFF / pulse_count);
        float t = static_cast<float>(cycle_phase) / 0xFFFFFFFF;
        
        float amplitude = std::exp(-t * decay_rate / 8192.0f);
        float pulse = std::sin(2.0f * M_PI * t * pulse_count) * amplitude;
        
        *buffer++ = static_cast<int16_t>(pulse * 16384.0f);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderVowel(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Simple vowel synthesis using formant filtering
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Update formants based on parameter
    UpdateFormants(parameter_[0]);
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Generate sawtooth as excitation
        int16_t excitation = (phase >> 16) - 32768;
        
        // Apply formant filtering (simplified)
        int32_t result = excitation;
        for (int i = 0; i < 3; ++i) {
            state_.vow.formant_phase[i] += state_.vow.formant_increment[i];
            int32_t formant = InterpolateWaveform(wav_sine, state_.vow.formant_phase[i], 1024);
            result = (result * formant) >> 16;
        }
        
        *buffer++ = SoftLimit(result);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderVowelFof(const uint8_t* sync, int16_t* buffer, size_t size) {
    // FOF (Fonction d'Onde Formantique) synthesis
    RenderVowel(sync, buffer, size);  // Simplified version
}

void DigitalOscillator::RenderHarmonics(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Additive synthesis with 14 harmonics
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    UpdateHarmonics(parameter_[0]);
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            for (int i = 0; i < 14; ++i) {
                state_.harm.phase[i] = 0;
            }
        }
        
        phase += increment;
        
        int32_t result = 0;
        for (int i = 0; i < 14; ++i) {
            state_.harm.phase[i] += state_.harm.increment[i];
            int16_t harmonic = InterpolateWaveform(wav_sine, state_.harm.phase[i], 1024);
            result += (harmonic * state_.harm.harmonics[i]) >> 15;
        }
        
        *buffer++ = SoftLimit(result >> 2);  // Divide by 4 to prevent clipping
    }
    
    phase_ = phase;
}

// Phase 3: FM Synthesis

void DigitalOscillator::RenderFM(const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t carrier_phase = state_.fm.carrier_phase;
    uint32_t modulator_phase = state_.fm.modulator_phase;
    
    uint32_t carrier_increment = phase_increment_;
    float fm_ratio = static_cast<float>(parameter_[0]) / 16384.0f;
    float fm_amount = static_cast<float>(parameter_[1]) / 32767.0f;
    
    uint32_t modulator_increment = static_cast<uint32_t>(carrier_increment * fm_ratio);
    
    while (size--) {
        if (*sync++) {
            carrier_phase = 0;
            modulator_phase = 0;
        }
        
        modulator_phase += modulator_increment;
        
        // Get modulator output
        int16_t modulator = InterpolateWaveform(wav_sine, modulator_phase, 1024);
        
        // Modulate carrier frequency
        int32_t fm_offset = static_cast<int32_t>(modulator * fm_amount);
        carrier_phase += carrier_increment + fm_offset;
        
        // Get carrier output
        int16_t output = InterpolateWaveform(wav_sine, carrier_phase, 1024);
        
        *buffer++ = output;
    }
    
    state_.fm.carrier_phase = carrier_phase;
    state_.fm.modulator_phase = modulator_phase;
}

void DigitalOscillator::RenderFeedbackFM(const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t carrier_phase = state_.fm.carrier_phase;
    
    float feedback_amount = static_cast<float>(parameter_[0]) / 32767.0f;
    float fm_amount = static_cast<float>(parameter_[1]) / 32767.0f;
    
    uint32_t carrier_increment = phase_increment_;
    
    while (size--) {
        if (*sync++) {
            carrier_phase = 0;
            state_.fm.previous_sample[0] = 0;
        }
        
        // Use previous output as modulation source
        int32_t fm_offset = static_cast<int32_t>(state_.fm.previous_sample[0] * fm_amount);
        carrier_phase += carrier_increment + fm_offset;
        
        // Get carrier output
        int16_t output = InterpolateWaveform(wav_sine, carrier_phase, 1024);
        
        // Apply feedback
        state_.fm.previous_sample[0] = static_cast<int32_t>(output * feedback_amount);
        
        *buffer++ = output;
    }
    
    state_.fm.carrier_phase = carrier_phase;
}

void DigitalOscillator::RenderChaoticFeedbackFM(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Chaotic feedback FM with nonlinear processing
    uint32_t carrier_phase = state_.fm.carrier_phase;
    
    float feedback_amount = static_cast<float>(parameter_[0]) / 32767.0f;
    float chaos_amount = static_cast<float>(parameter_[1]) / 32767.0f;
    
    uint32_t carrier_increment = phase_increment_;
    
    while (size--) {
        if (*sync++) {
            carrier_phase = 0;
            state_.fm.previous_sample[0] = 0;
        }
        
        // Chaotic nonlinear feedback
        float feedback = static_cast<float>(state_.fm.previous_sample[0]) / 32767.0f;
        feedback = std::tanh(feedback * chaos_amount) * feedback_amount;
        
        int32_t fm_offset = static_cast<int32_t>(feedback * 32767.0f);
        carrier_phase += carrier_increment + fm_offset;
        
        // Get carrier output
        int16_t output = InterpolateWaveform(wav_sine, carrier_phase, 1024);
        
        state_.fm.previous_sample[0] = static_cast<int32_t>(output);
        
        *buffer++ = output;
    }
    
    state_.fm.carrier_phase = carrier_phase;
}

// Helper functions (implementations will be added progressively)

void DigitalOscillator::UpdateFormants(int16_t vowel_param) {
    // Update formant frequencies based on vowel parameter
    // Simplified version - would use proper formant tables in full implementation
    for (int i = 0; i < 3; ++i) {
        float formant_freq = 500.0f + i * 800.0f + (vowel_param * 0.1f);
        state_.vow.formant_increment[i] = static_cast<uint32_t>((formant_freq * 4294967296.0) / 48000.0);
    }
}

void DigitalOscillator::UpdateHarmonics(int16_t harmonic_param) {
    // Update harmonic levels based on parameter
    for (int i = 0; i < 14; ++i) {
        // Simple harmonic series with parameter control
        float amplitude = 1.0f / (i + 1);
        amplitude *= (1.0f - (harmonic_param * 0.00003f));  // Parameter controls rolloff
        
        state_.harm.harmonics[i] = static_cast<int16_t>(amplitude * 16384.0f);
        state_.harm.increment[i] = phase_increment_ * (i + 1);
    }
}

void DigitalOscillator::ProcessFilter(float* state, float input, float frequency, float resonance, int16_t* output) {
    // Simple biquad filter implementation
    frequency = std::max(0.001f, std::min(frequency, 0.499f));
    resonance = std::max(0.0f, std::min(resonance, 0.99f));
    
    float omega = 2.0f * M_PI * frequency;
    float sin_omega = std::sin(omega);
    float cos_omega = std::cos(omega);
    float alpha = sin_omega / (2.0f * (1.0f - resonance + 1.0f));
    
    float b0 = (1.0f - cos_omega) / 2.0f;
    float b1 = 1.0f - cos_omega;
    float b2 = (1.0f - cos_omega) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_omega;
    float a2 = 1.0f - alpha;
    
    // Process sample
    float output_f = (b0 * input + b1 * state[0] + b2 * state[1] - a1 * state[2] - a2 * state[3]) / a0;
    
    // Update state
    state[1] = state[0];
    state[0] = input;
    state[3] = state[2];
    state[2] = output_f;
    
    *output = static_cast<int16_t>(std::max(-32767.0f, std::min(32767.0f, output_f * 32767.0f)));
}

// Physical modeling stubs (Phase 4 - to be implemented fully)

void DigitalOscillator::RenderStruckBell(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bell synthesis using modal synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderStruckDrum(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Drum synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderKick(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Kick drum synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderCymbal(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Cymbal synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderSnare(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Snare synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderPlucked(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Karplus-Strong plucked string
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderBowed(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bowed string synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderBlown(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Blown pipe synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderFluted(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Flute synthesis
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

// Wavetable and noise stubs (Phase 5)

void DigitalOscillator::RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderWaveMap(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderWaveLine(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderWaveParaphonic(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderFilteredNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderTwinPeaksNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderClockedNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderGranularCloud(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderParticleNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void DigitalOscillator::RenderDigitalModulation(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder implementation
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

}  // namespace braidy