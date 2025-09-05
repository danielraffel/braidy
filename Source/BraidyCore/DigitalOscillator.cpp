#include "DigitalOscillator.h"
#include "BraidyResources.h"
#include "BraidyMath.h"
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

// Phase 4: Physical Modeling Synthesis

void DigitalOscillator::RenderStruckBell(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bell synthesis using modal synthesis with 11 partials
    if (struck_) {
        // Initialize bell partials with inharmonic ratios
        float bell_ratios[11] = {1.0f, 2.76f, 5.40f, 8.93f, 13.34f, 18.64f, 
                                 24.83f, 31.92f, 39.89f, 48.78f, 58.57f};
        
        for (int i = 0; i < 11; ++i) {
            float partial_freq = ComputePhaseIncrement(pitch_) * bell_ratios[i];
            state_.bell.bell_phase[i] = 0;
            // Amplitude rolloff and damping based on frequency
            float damping = 1.0f - (i * 0.08f);  
            state_.bell.bell_amplitude[i] = static_cast<int16_t>(16384 * damping / (i + 1));
        }
        struck_ = false;
    }
    
    float stiffness = static_cast<float>(parameter_[0]) / 32767.0f;
    float damping = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            // Reset bell on sync
            for (int i = 0; i < 11; ++i) {
                state_.bell.bell_phase[i] = 0;
            }
        }
        
        int32_t result = 0;
        for (int i = 0; i < 11; ++i) {
            // Update each partial
            float partial_increment = ComputePhaseIncrement(pitch_) * (i + 1) * (1.0f + stiffness * i * 0.02f);
            state_.bell.bell_phase[i] += static_cast<uint32_t>(partial_increment);
            
            // Generate partial with sine wave
            int16_t partial = InterpolateWaveform(wav_sine, state_.bell.bell_phase[i], 1024);
            
            // Apply amplitude envelope with damping
            int16_t amplitude = state_.bell.bell_amplitude[i];
            amplitude = static_cast<int16_t>(amplitude * (1.0f - damping * 0.0001f));
            state_.bell.bell_amplitude[i] = amplitude;
            
            result += (partial * amplitude) >> 15;
        }
        
        *buffer++ = SoftLimit(result >> 1);
    }
}

void DigitalOscillator::RenderStruckDrum(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Drum synthesis with 6 partials and membrane modeling
    if (struck_) {
        float drum_ratios[6] = {1.0f, 1.59f, 2.14f, 2.65f, 3.11f, 3.56f};
        
        for (int i = 0; i < 6; ++i) {
            state_.drum.drum_phase[i] = 0;
            state_.drum.drum_amplitude[i] = 16384 / (i + 1);
            state_.drum.drum_decay[i] = 65000 - (i * 8000);  // Different decay rates
        }
        struck_ = false;
    }
    
    float tension = static_cast<float>(parameter_[0]) / 32767.0f;
    float brightness = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            for (int i = 0; i < 6; ++i) {
                state_.drum.drum_phase[i] = 0;
            }
        }
        
        int32_t result = 0;
        for (int i = 0; i < 6; ++i) {
            // Membrane modes with tension affecting pitch
            float partial_increment = ComputePhaseIncrement(pitch_) * (i + 1) * (0.8f + tension * 0.4f);
            state_.drum.drum_phase[i] += static_cast<uint32_t>(partial_increment);
            
            int16_t partial = InterpolateWaveform(wav_sine, state_.drum.drum_phase[i], 1024);
            
            // Exponential decay
            uint16_t decay_rate = state_.drum.drum_decay[i];
            int16_t amplitude = state_.drum.drum_amplitude[i];
            amplitude = (amplitude * decay_rate) >> 16;
            state_.drum.drum_amplitude[i] = amplitude;
            
            // Brightness affects harmonic balance
            float harmonic_gain = 1.0f - (i * 0.1f * (1.0f - brightness));
            result += static_cast<int32_t>((partial * amplitude * harmonic_gain)) >> 15;
        }
        
        *buffer++ = SoftLimit(result);
    }
}

void DigitalOscillator::RenderKick(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Kick drum with pitch envelope and punch
    if (struck_) {
        state_.kick.punch_phase = 0;
        state_.kick.tone_phase = 0;
        state_.kick.svf_state[0] = 0;
        state_.kick.svf_state[1] = 0;
        struck_ = false;
    }
    
    float punch_amount = static_cast<float>(parameter_[0]) / 32767.0f;
    float tone_amount = static_cast<float>(parameter_[1]) / 32767.0f;
    
    uint32_t base_increment = ComputePhaseIncrement(pitch_);
    
    while (size--) {
        if (*sync++) {
            state_.kick.punch_phase = 0;
            state_.kick.tone_phase = 0;
        }
        
        // Punch component - fast decaying click
        uint32_t punch_increment = base_increment * 8;  // High frequency click
        state_.kick.punch_phase += punch_increment;
        int16_t punch = (state_.kick.punch_phase >> 16) - 32768;  // Sawtooth
        punch = static_cast<int16_t>(punch * punch_amount * std::exp(-punch_amount * 0.1f));
        
        // Tone component - sine wave with pitch envelope
        float pitch_env = std::exp(-tone_amount * 0.05f);  // Pitch drops over time
        uint32_t tone_increment = static_cast<uint32_t>(base_increment * pitch_env);
        state_.kick.tone_phase += tone_increment;
        int16_t tone = InterpolateWaveform(wav_sine, state_.kick.tone_phase, 1024);
        
        // Mix punch and tone
        int32_t result = punch + ((tone * static_cast<int32_t>(tone_amount)) >> 8);
        
        // Low-pass filter for body
        state_.kick.svf_state[0] += ((result - state_.kick.svf_state[0]) * 8192) >> 15;
        state_.kick.svf_state[1] += ((state_.kick.svf_state[0] - state_.kick.svf_state[1]) * 4096) >> 15;
        
        *buffer++ = SoftLimit(state_.kick.svf_state[1]);
    }
}

void DigitalOscillator::RenderCymbal(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Cymbal synthesis using filtered noise and resonant modes
    if (struck_) {
        for (int i = 0; i < 6; ++i) {
            state_.cymb.hihat_phase[i] = Rng();
            state_.cymb.hihat_amplitude[i] = 16384 / (i + 1);
        }
        state_.cymb.click_phase = 0;
        state_.cymb.filter_state = 0;
        struck_ = false;
    }
    
    float brightness = static_cast<float>(parameter_[0]) / 32767.0f;
    float decay_rate = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            for (int i = 0; i < 6; ++i) {
                state_.cymb.hihat_phase[i] = Rng();
            }
            state_.cymb.click_phase = 0;
        }
        
        int32_t result = 0;
        
        // High frequency resonant modes
        for (int i = 0; i < 6; ++i) {
            // Use noise-modulated oscillators
            uint32_t noise_mod = Rng() >> 24;
            uint32_t freq_mult = 8 + i * 4;  // High frequency modes
            uint32_t increment = ComputePhaseIncrement(pitch_) * freq_mult + noise_mod;
            
            state_.cymb.hihat_phase[i] += increment;
            int16_t partial = InterpolateWaveform(wav_sine, state_.cymb.hihat_phase[i], 1024);
            
            // Exponential decay
            int16_t amplitude = state_.cymb.hihat_amplitude[i];
            amplitude = (amplitude * (65536 - static_cast<int32_t>(decay_rate * 200))) >> 16;
            state_.cymb.hihat_amplitude[i] = amplitude;
            
            result += (partial * amplitude * static_cast<int32_t>(brightness)) >> 16;
        }
        
        // Add metallic click component
        state_.cymb.click_phase += ComputePhaseIncrement(pitch_) * 16;
        int16_t click = ((state_.cymb.click_phase >> 16) ^ (state_.cymb.click_phase >> 20)) - 32768;
        result += click * static_cast<int32_t>(brightness * 0.3f);
        
        // High-pass filter for shimmer
        int32_t hp_input = result;
        state_.cymb.filter_state += ((hp_input - state_.cymb.filter_state) * 16384) >> 15;
        result = hp_input - state_.cymb.filter_state;
        
        *buffer++ = SoftLimit(result >> 2);
    }
}

void DigitalOscillator::RenderSnare(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Snare drum with noise and tonal components
    if (struck_) {
        state_.snar.noise_phase = Rng();
        state_.snar.tone_phase = 0;
        state_.snar.filter_state_1 = 0;
        state_.snar.filter_state_2 = 0;
        state_.snar.decay = 65000;
        struck_ = false;
    }
    
    float snare_tension = static_cast<float>(parameter_[0]) / 32767.0f;
    float snare_brightness = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            state_.snar.noise_phase = Rng();
            state_.snar.tone_phase = 0;
        }
        
        // Noise component (snare buzz)
        state_.snar.noise_phase = Rng();
        int16_t noise = static_cast<int16_t>(state_.snar.noise_phase >> 16) - 32768;
        
        // Band-pass filter for snare character
        state_.snar.filter_state_1 += ((noise - state_.snar.filter_state_1) * 12288) >> 15;
        state_.snar.filter_state_2 += ((state_.snar.filter_state_1 - state_.snar.filter_state_2) * 8192) >> 15;
        int16_t filtered_noise = state_.snar.filter_state_1 - state_.snar.filter_state_2;
        
        // Tonal component (drum head)
        uint32_t tone_increment = ComputePhaseIncrement(pitch_) * (1.0f + snare_tension);
        state_.snar.tone_phase += tone_increment;
        int16_t tone = InterpolateWaveform(wav_sine, state_.snar.tone_phase, 1024);
        
        // Mix noise and tone with brightness control
        int32_t result = (filtered_noise * snare_brightness) + (tone * (1.0f - snare_brightness * 0.5f));
        
        // Exponential decay
        uint16_t decay = state_.snar.decay;
        decay = (decay * 65400) >> 16;  // Slow decay
        state_.snar.decay = decay;
        
        result = (result * decay) >> 16;
        
        *buffer++ = SoftLimit(result);
    }
}

void DigitalOscillator::RenderPlucked(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Karplus-Strong plucked string synthesis
    uint32_t delay_length = ComputeDelay(pitch_);
    if (delay_length < 4) delay_length = 4;
    if (delay_length > 1024) delay_length = 1024;
    
    if (struck_ || (sync && *sync)) {
        // Initialize delay line with noise burst for pluck excitation
        for (uint32_t i = 0; i < delay_length; ++i) {
            state_.pluk.string_1[i] = static_cast<int16_t>(Rng() >> 17) - 16384;
        }
        state_.pluk.ptr_1 = 0;
        state_.pluk.length_1 = delay_length;
        struck_ = false;
    }
    
    float damping = 1.0f - (static_cast<float>(parameter_[0]) / 65534.0f);  // 0.5 to 1.0
    float brightness = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        sync++;  // Advance sync pointer
        
        // Read from delay line
        int16_t output = state_.pluk.string_1[state_.pluk.ptr_1];
        
        // Get next sample for averaging (Karplus-Strong smoothing)
        uint32_t next_ptr = (state_.pluk.ptr_1 + 1) % state_.pluk.length_1;
        int16_t next_sample = state_.pluk.string_1[next_ptr];
        
        // Low-pass filter (averaging) with brightness control
        int32_t averaged = (output + next_sample) >> 1;
        int32_t hp_component = output - averaged;
        int32_t filtered = averaged + ((hp_component * static_cast<int32_t>(brightness)) >> 8);
        
        // Apply damping
        filtered = static_cast<int32_t>(filtered * damping);
        
        // Write back to delay line
        state_.pluk.string_1[next_ptr] = SoftLimit(filtered);
        
        // Advance pointer
        state_.pluk.ptr_1 = next_ptr;
        
        *buffer++ = output;
    }
}

void DigitalOscillator::RenderBowed(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bowed string using waveguide synthesis with bow-string interaction
    uint32_t delay_length = ComputeDelay(pitch_);
    if (delay_length < 4) delay_length = 4;
    if (delay_length > 1024) delay_length = 1024;
    
    if (struck_) {
        // Initialize string with small excitation
        for (uint32_t i = 0; i < delay_length; ++i) {
            state_.pluk.string_1[i] = static_cast<int16_t>(Rng() >> 20);  // Small initial excitation
        }
        state_.pluk.ptr_1 = 0;
        state_.pluk.length_1 = delay_length;
        state_.pluk.excitation_state = 0;
        struck_ = false;
    }
    
    float bow_pressure = static_cast<float>(parameter_[0]) / 32767.0f;
    float bow_position = static_cast<float>(parameter_[1]) / 32767.0f;
    
    // Bow position affects where we inject energy
    uint32_t bow_pos = static_cast<uint32_t>(bow_position * delay_length);
    
    while (size--) {
        if (*sync++) {
            // Reset on sync
            for (uint32_t i = 0; i < delay_length; ++i) {
                state_.pluk.string_1[i] = 0;
            }
        }
        
        // Read current string displacement
        int16_t string_velocity = state_.pluk.string_1[state_.pluk.ptr_1];
        
        // Bow-string interaction (simplified friction model)
        int16_t bow_velocity = static_cast<int16_t>(bow_pressure * 8192);  // Bow velocity
        int16_t velocity_diff = bow_velocity - string_velocity;
        
        // Friction force (simplified)
        int16_t friction_force = 0;
        if (std::abs(velocity_diff) > 1000) {
            friction_force = static_cast<int16_t>(velocity_diff * bow_pressure * 0.1f);
        } else {
            // Stick condition - string follows bow
            friction_force = static_cast<int16_t>(velocity_diff * bow_pressure * 0.5f);
        }
        
        // Inject bow energy at bow position
        uint32_t bow_ptr = (state_.pluk.ptr_1 + bow_pos) % state_.pluk.length_1;
        state_.pluk.string_1[bow_ptr] += friction_force >> 4;
        
        // String propagation with damping
        uint32_t next_ptr = (state_.pluk.ptr_1 + 1) % state_.pluk.length_1;
        int16_t next_sample = state_.pluk.string_1[next_ptr];
        
        // Simple damping and dispersion
        int32_t averaged = (string_velocity + next_sample) >> 1;
        averaged = (averaged * 32600) >> 15;  // Slight damping
        
        state_.pluk.string_1[next_ptr] = SoftLimit(averaged);
        state_.pluk.ptr_1 = next_ptr;
        
        *buffer++ = string_velocity;
    }
}

void DigitalOscillator::RenderBlown(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Blown pipe synthesis using waveguide with reed/embouchure modeling
    uint32_t delay_length = ComputeDelay(pitch_) >> 1;  // Half wavelength for open pipe
    if (delay_length < 4) delay_length = 4;
    if (delay_length > 512) delay_length = 512;
    
    if (struck_) {
        // Initialize pipe with small noise
        for (uint32_t i = 0; i < delay_length; ++i) {
            state_.pluk.string_1[i] = static_cast<int16_t>(Rng() >> 20);
        }
        state_.pluk.ptr_1 = 0;
        state_.pluk.length_1 = delay_length;
        struck_ = false;
    }
    
    float breath_pressure = static_cast<float>(parameter_[0]) / 32767.0f;
    float embouchure = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            for (uint32_t i = 0; i < delay_length; ++i) {
                state_.pluk.string_1[i] = 0;
            }
        }
        
        // Read from delay line (pipe pressure)
        int16_t pipe_pressure = state_.pluk.string_1[state_.pluk.ptr_1];
        
        // Reed/embouchure non-linearity
        float pressure_diff = breath_pressure * 16384 - pipe_pressure;
        
        // Non-linear flow (simplified reed model)
        float flow = 0;
        if (pressure_diff > 0) {
            flow = pressure_diff * embouchure;
            if (flow > 8192) flow = 8192;  // Saturation
        }
        
        // Inject flow into pipe
        int32_t new_pressure = static_cast<int32_t>(flow);
        
        // Waveguide propagation
        uint32_t next_ptr = (state_.pluk.ptr_1 + 1) % state_.pluk.length_1;
        int16_t reflected = -state_.pluk.string_1[next_ptr];  // Open end reflection (inverted)
        
        // Mix injected flow with reflected wave
        new_pressure = (new_pressure + reflected) >> 1;
        
        // Mild low-pass filtering for realism
        new_pressure = (new_pressure * 7 + state_.pluk.string_1[state_.pluk.ptr_1]) >> 3;
        
        state_.pluk.string_1[state_.pluk.ptr_1] = SoftLimit(new_pressure);
        state_.pluk.ptr_1 = next_ptr;
        
        *buffer++ = static_cast<int16_t>(new_pressure);
    }
}

void DigitalOscillator::RenderFluted(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Flute synthesis using jet-edge interaction and bore modeling
    uint32_t delay_length = ComputeDelay(pitch_) >> 1;  // Half wavelength
    if (delay_length < 8) delay_length = 8;
    if (delay_length > 512) delay_length = 512;
    
    if (struck_) {
        // Initialize flute bore
        for (uint32_t i = 0; i < delay_length; ++i) {
            state_.pluk.string_1[i] = 0;  // Clean start for flute
        }
        state_.pluk.ptr_1 = 0;
        state_.pluk.length_1 = delay_length;
        state_.pluk.excitation_state = 0;
        struck_ = false;
    }
    
    float jet_velocity = static_cast<float>(parameter_[0]) / 32767.0f;
    float jet_offset = static_cast<float>(parameter_[1]) / 32767.0f;
    
    while (size--) {
        if (*sync++) {
            for (uint32_t i = 0; i < delay_length; ++i) {
                state_.pluk.string_1[i] = 0;
            }
        }
        
        // Read bore pressure
        int16_t bore_pressure = state_.pluk.string_1[state_.pluk.ptr_1];
        
        // Jet-edge interaction (Verge's model simplified)
        float jet_displacement = jet_offset * 16384 + bore_pressure * 0.3f;  // Jet follows bore somewhat
        
        // Non-linear jet-edge interaction
        float edge_force = 0;
        if (jet_displacement > 0) {
            edge_force = jet_velocity * jet_displacement * 0.1f;
            // Add slight non-linearity
            edge_force += jet_velocity * jet_displacement * jet_displacement * 0.00001f;
        }
        
        // Limit edge force
        if (edge_force > 8192) edge_force = 8192;
        if (edge_force < -8192) edge_force = -8192;
        
        // Inject into bore
        int32_t new_pressure = static_cast<int32_t>(edge_force);
        
        // Bore propagation with open end reflection
        uint32_t reflection_ptr = (state_.pluk.ptr_1 + delay_length - 1) % state_.pluk.length_1;
        int16_t reflected = -state_.pluk.string_1[reflection_ptr] >> 2;  // Partial reflection
        
        new_pressure = (new_pressure + reflected + bore_pressure) / 3;
        
        // Gentle low-pass for breath-like quality
        state_.pluk.excitation_state = (state_.pluk.excitation_state * 3 + new_pressure) >> 2;
        
        state_.pluk.string_1[state_.pluk.ptr_1] = SoftLimit(state_.pluk.excitation_state);
        state_.pluk.ptr_1 = (state_.pluk.ptr_1 + 1) % state_.pluk.length_1;
        
        *buffer++ = static_cast<int16_t>(state_.pluk.excitation_state);
    }
}

// Wavetable and noise stubs (Phase 5)

void DigitalOscillator::RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Standard wavetable synthesis with morphing
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls wavetable selection (0-63)
    uint32_t wavetable_index = (parameter_[0] * 63) >> 15;
    uint32_t next_wavetable = (wavetable_index + 1) & 63;
    uint32_t morph_amount = (parameter_[0] * 63) & 0x7FFF;  // Fractional part
    
    // Parameter 1 controls wavetable scanning position
    uint32_t scan_position = parameter_[1];
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Get sample from current wavetable
        auto current_wt = GetWavetable(wavetable_index);
        int16_t sample_a = current_wt.LookupInterpolated(phase + scan_position);
        
        // Get sample from next wavetable for morphing
        auto next_wt = GetWavetable(next_wavetable);
        int16_t sample_b = next_wt.LookupInterpolated(phase + scan_position);
        
        // Morph between wavetables
        int32_t result = sample_a + (((sample_b - sample_a) * morph_amount) >> 15);
        
        *buffer++ = static_cast<int16_t>(result);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderWaveMap(const uint8_t* sync, int16_t* buffer, size_t size) {
    // 2D wavetable morphing - both X and Y axis control
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls X-axis (wavetable bank)
    uint32_t bank_x = (parameter_[0] * 7) >> 15;  // 8 banks of 8
    uint32_t bank_x_frac = (parameter_[0] * 7) & 0x7FFF;
    
    // Parameter 1 controls Y-axis (wavetable within bank)
    uint32_t wave_y = (parameter_[1] * 7) >> 15;  // 8 waves per bank
    uint32_t wave_y_frac = (parameter_[1] * 7) & 0x7FFF;
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Sample from 4 corner wavetables for bilinear interpolation
        uint32_t wt_00 = (bank_x * 8 + wave_y) & 63;
        uint32_t wt_01 = (bank_x * 8 + ((wave_y + 1) & 7)) & 63;
        uint32_t wt_10 = (((bank_x + 1) & 7) * 8 + wave_y) & 63;
        uint32_t wt_11 = (((bank_x + 1) & 7) * 8 + ((wave_y + 1) & 7)) & 63;
        
        // Get samples from all 4 corners
        int16_t s00 = GetWavetable(wt_00).LookupInterpolated(phase);
        int16_t s01 = GetWavetable(wt_01).LookupInterpolated(phase);
        int16_t s10 = GetWavetable(wt_10).LookupInterpolated(phase);
        int16_t s11 = GetWavetable(wt_11).LookupInterpolated(phase);
        
        // Bilinear interpolation
        int32_t s0 = s00 + (((s01 - s00) * wave_y_frac) >> 15);
        int32_t s1 = s10 + (((s11 - s10) * wave_y_frac) >> 15);
        int32_t result = s0 + (((s1 - s0) * bank_x_frac) >> 15);
        
        *buffer++ = static_cast<int16_t>(result);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderWaveLine(const uint8_t* sync, int16_t* buffer, size_t size) {
    // 1D wavetable sweep with automatic scanning
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls sweep speed
    uint32_t sweep_speed = (parameter_[0] >> 8) + 1;  // 1-128
    
    // Parameter 1 controls sweep range
    uint32_t sweep_range = (parameter_[1] >> 9) + 8;  // 8-72 wavetables
    
    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        
        phase += increment;
        
        // Auto-sweep through wavetables based on time
        state_.wtbl.phase[0] += sweep_speed << 8;
        uint32_t sweep_pos = (state_.wtbl.phase[0] >> 16) % sweep_range;
        uint32_t sweep_frac = (state_.wtbl.phase[0] >> 8) & 0xFF;
        
        // Get samples from current and next wavetable
        int16_t sample_a = GetWavetable(sweep_pos).LookupInterpolated(phase);
        int16_t sample_b = GetWavetable((sweep_pos + 1) % sweep_range).LookupInterpolated(phase);
        
        // Interpolate between wavetables
        int32_t result = sample_a + (((sample_b - sample_a) * sweep_frac) >> 8);
        
        *buffer++ = static_cast<int16_t>(result);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderWaveParaphonic(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Paraphonic wavetable synthesis - 4 oscillators with different wavetables
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls detuning amount
    int32_t detune_amount = (parameter_[0] - 16384) >> 6;  // ±256 detune range
    
    // Parameter 1 controls wavetable spread
    uint32_t wt_spread = parameter_[1] >> 10;  // 0-31 wavetable offset
    
    while (size--) {
        if (*sync++) {
            for (int i = 0; i < 4; ++i) {
                state_.wtbl.phase[i] = 0;
            }
        }
        
        int32_t result = 0;
        
        // 4 wavetable oscillators with different detune and wavetables
        for (int i = 0; i < 4; ++i) {
            // Different detune for each oscillator
            int32_t detune = detune_amount * (i - 2);  // -2, -1, 1, 2 detune factors
            uint32_t osc_increment = increment + detune;
            
            state_.wtbl.phase[i] += osc_increment;
            
            // Different wavetable for each oscillator
            uint32_t wavetable_idx = (i * wt_spread) & 63;
            
            // Get sample from wavetable
            int16_t sample = GetWavetable(wavetable_idx).LookupInterpolated(state_.wtbl.phase[i]);
            
            // Mix with level control
            result += (sample * state_.wtbl.level[i]) >> 15;
        }
        
        // Scale down to prevent clipping
        result >>= 2;
        
        *buffer++ = static_cast<int16_t>(result);
    }
    
    // Update oscillator levels (simple envelope)
    for (int i = 0; i < 4; ++i) {
        if (state_.wtbl.level[i] < 16384) {
            state_.wtbl.level[i] += 32;  // Slow attack
        }
    }
}

void DigitalOscillator::RenderFilteredNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Filtered noise synthesis with resonant filtering
    float frequency = static_cast<float>(parameter_[0]) / 32767.0f;  // 0-1 filter frequency
    float resonance = static_cast<float>(parameter_[1]) / 32767.0f;  // 0-1 resonance
    
    // Map frequency to useful range (20Hz - 20kHz)
    frequency = 20.0f + frequency * frequency * 19980.0f;
    frequency /= 48000.0f;  // Normalize to sample rate
    
    while (size--) {
        sync++;  // Advance sync pointer
        
        // Generate white noise
        int16_t noise = static_cast<int16_t>(Rng() >> 17) - 16384;
        
        // Process through resonant filter
        int16_t output;
        ProcessFilter(state_.filt.filter_state, static_cast<float>(noise) / 32768.0f, frequency, resonance, &output);
        
        *buffer++ = output;
    }
}

void DigitalOscillator::RenderTwinPeaksNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Twin peaks noise - dual resonant filters on noise
    float freq1 = static_cast<float>(parameter_[0]) / 32767.0f;
    float freq2 = static_cast<float>(parameter_[1]) / 32767.0f;
    
    // Map to different frequency ranges
    freq1 = 100.0f + freq1 * freq1 * 5000.0f;  // 100Hz - 5.1kHz
    freq2 = 200.0f + freq2 * freq2 * 8000.0f;  // 200Hz - 8.2kHz
    
    freq1 /= 48000.0f;
    freq2 /= 48000.0f;
    
    while (size--) {
        sync++;  // Advance sync pointer
        
        // Generate white noise
        int16_t noise = static_cast<int16_t>(Rng() >> 17) - 16384;
        float noise_f = static_cast<float>(noise) / 32768.0f;
        
        // Process through two resonant filters
        int16_t output1, output2;
        ProcessFilter(&state_.filt.filter_state[0], noise_f, freq1, 0.8f, &output1);
        ProcessFilter(&state_.filt.filter_state[2], noise_f, freq2, 0.8f, &output2);
        
        // Mix the two filtered signals
        int32_t result = (static_cast<int32_t>(output1) + output2) >> 1;
        
        *buffer++ = static_cast<int16_t>(result);
    }
}

void DigitalOscillator::RenderClockedNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Clocked noise - sample and hold noise at a specific rate
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls clock rate (divider)
    uint32_t clock_divider = (parameter_[0] >> 8) + 1;  // 1-128
    
    // Parameter 1 controls noise character
    uint32_t noise_type = parameter_[1] >> 13;  // 0-3 different noise types
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            state_.noise.next_sample = 0;
        }
        
        phase += increment;
        
        // Clock generation based on phase
        uint32_t current_clock = phase / (0xFFFFFFFF / clock_divider);
        uint32_t previous_clock = (phase - increment) / (0xFFFFFFFF / clock_divider);
        
        // Generate new noise sample on clock edge
        if (current_clock != previous_clock) {
            switch (noise_type) {
                case 0:  // White noise
                    state_.noise.next_sample = Rng() >> 17;
                    break;
                case 1:  // Pink-ish noise (simple 1-pole filter)
                    state_.noise.next_sample = (state_.noise.next_sample * 7 + (Rng() >> 17)) >> 3;
                    break;
                case 2:  // Stepped random walk
                    state_.noise.next_sample += ((Rng() >> 18) - 8192);
                    if (state_.noise.next_sample > 16384) state_.noise.next_sample = 16384;
                    if (state_.noise.next_sample < -16384) state_.noise.next_sample = -16384;
                    break;
                case 3:  // Quantized noise (bit-crushed)
                    state_.noise.next_sample = (Rng() >> 19) << 2;  // 4-bit quantized
                    break;
            }
        }
        
        *buffer++ = static_cast<int16_t>(state_.noise.next_sample);
    }
    
    phase_ = phase;
}

void DigitalOscillator::RenderGranularCloud(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Granular cloud synthesis - multiple overlapping grains
    float grain_density = static_cast<float>(parameter_[0]) / 32767.0f;  // 0-1
    float grain_size = static_cast<float>(parameter_[1]) / 32767.0f;     // 0-1
    
    // Convert to useful ranges
    uint32_t grain_length = static_cast<uint32_t>(48 + grain_size * 1000);  // 1-21ms grains
    uint32_t grain_rate = static_cast<uint32_t>(1 + grain_density * 100);   // 1-100 grains per second
    
    while (size--) {
        sync++;  // Advance sync pointer
        
        int32_t result = 0;
        
        // Update grain triggers
        for (int g = 0; g < 8; ++g) {  // 8 concurrent grains max
            // Check if grain should start
            if (state_.wtbl.phase[g] == 0) {
                // Random grain triggering based on density
                if ((Rng() & 0xFFFF) < (grain_rate * 655)) {
                    state_.wtbl.phase[g] = 1;  // Start grain
                    state_.wtbl.level[g] = grain_length;  // Grain length counter
                }
            }
            
            // Process active grains
            if (state_.wtbl.phase[g] > 0) {
                // Grain position within length
                float grain_pos = static_cast<float>(state_.wtbl.phase[g]) / grain_length;
                
                // Simple grain envelope (Hann window)
                float envelope = 0.5f * (1.0f - std::cos(2.0f * M_PI * grain_pos));
                
                // Generate grain content (filtered noise)
                int16_t grain_noise = static_cast<int16_t>(Rng() >> 17) - 16384;
                
                // Apply different grain characteristics per grain
                float grain_freq = 0.1f + (g * 0.1f);  // Different filter freq per grain
                
                // Simple one-pole low-pass per grain
                state_.noise.integrator_state = static_cast<int32_t>(
                    state_.noise.integrator_state * (1.0f - grain_freq) + grain_noise * grain_freq
                );
                
                // Apply envelope and add to result
                int32_t grain_out = static_cast<int32_t>(state_.noise.integrator_state * envelope);
                result += grain_out >> 3;  // Scale down
                
                // Advance grain
                state_.wtbl.phase[g]++;
                if (state_.wtbl.phase[g] > grain_length) {
                    state_.wtbl.phase[g] = 0;  // End grain
                }
            }
        }
        
        *buffer++ = SoftLimit(result);
    }
}

void DigitalOscillator::RenderParticleNoise(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Particle noise - dust/crackle synthesis
    float particle_rate = static_cast<float>(parameter_[0]) / 32767.0f;    // 0-1 particle density
    float particle_energy = static_cast<float>(parameter_[1]) / 32767.0f;  // 0-1 particle energy
    
    // Convert to useful ranges
    uint32_t trigger_threshold = static_cast<uint32_t>((1.0f - particle_rate) * 65536);
    
    while (size--) {
        sync++;  // Advance sync pointer
        
        int32_t result = 0;
        
        // Random particle generation
        if ((Rng() & 0xFFFF) > trigger_threshold) {
            // Generate particle burst
            uint32_t particle_length = 4 + (Rng() & 15);  // 4-19 samples
            int16_t particle_amplitude = static_cast<int16_t>(particle_energy * 16384);
            
            // Particle characteristics
            uint32_t particle_type = Rng() & 3;
            
            switch (particle_type) {
                case 0:  // Sharp click
                    result = particle_amplitude * ((Rng() & 1) ? 1 : -1);
                    break;
                    
                case 1:  // Decaying burst
                    state_.noise.band_limited_impulse = particle_amplitude;
                    result = state_.noise.band_limited_impulse;
                    state_.noise.band_limited_impulse = (state_.noise.band_limited_impulse * 28000) >> 15;
                    break;
                    
                case 2:  // Filtered pop
                    {
                        int16_t noise_burst = static_cast<int16_t>(Rng() >> 17) - 16384;
                        state_.noise.integrator_state += ((noise_burst * particle_amplitude - state_.noise.integrator_state) * 8192) >> 15;
                        result = state_.noise.integrator_state;
                    }
                    break;
                    
                case 3:  // Resonant ping
                    {
                        // Simple resonator
                        float freq = 0.01f + particle_energy * 0.2f;
                        int32_t excitation = particle_amplitude * ((Rng() & 1) ? 1 : -1);
                        
                        // Add excitation to resonator state
                        state_.noise.integrator_state += excitation >> 4;
                        
                        // Resonator update
                        state_.noise.integrator_state = (state_.noise.integrator_state * (32768 - static_cast<int32_t>(freq * 1024))) >> 15;
                        result = state_.noise.integrator_state;
                    }
                    break;
            }
        }
        
        // Add background ambiance (very quiet)
        if (particle_rate > 0.1f) {
            int16_t ambient = static_cast<int16_t>(Rng() >> 22);  // Very quiet noise floor
            result += ambient;
        }
        
        *buffer++ = SoftLimit(result);
    }
}

void DigitalOscillator::RenderDigitalModulation(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Digital modulation synthesis - QPSK-like modulation
    uint32_t phase = phase_;
    uint32_t increment = phase_increment_;
    
    // Parameter 0 controls modulation rate
    uint32_t mod_rate = (parameter_[0] >> 10) + 1;  // 1-32 symbol rate divider
    
    // Parameter 1 controls modulation depth/type
    uint32_t mod_type = parameter_[1] >> 13;  // 0-3 modulation types
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            state_.noise.next_sample = 0;
        }
        
        phase += increment;
        
        // Symbol clock generation
        uint32_t symbol_clock = (phase >> 16) / mod_rate;
        
        // Generate new symbol on clock transition
        if (symbol_clock != ((phase - increment) >> 16) / mod_rate) {
            // Generate new modulation symbol
            switch (mod_type) {
                case 0:  // BPSK (Binary Phase Shift Keying)
                    state_.noise.next_sample = (Rng() & 0x8000) ? 16384 : -16384;
                    break;
                    
                case 1:  // QPSK (Quadrature Phase Shift Keying)
                    {
                        uint32_t symbol = Rng() & 3;
                        switch (symbol) {
                            case 0: state_.noise.next_sample = 11585;  break;  // 45 degrees
                            case 1: state_.noise.next_sample = -11585; break;  // 135 degrees
                            case 2: state_.noise.next_sample = -11585; break;  // 225 degrees
                            case 3: state_.noise.next_sample = 11585;  break;  // 315 degrees
                        }
                    }
                    break;
                    
                case 2:  // 8-PSK (8-level Phase Shift Keying)
                    {
                        uint32_t symbol = Rng() & 7;
                        float angle = symbol * M_PI / 4.0f;
                        state_.noise.next_sample = static_cast<int32_t>(16384 * std::cos(angle));
                    }
                    break;
                    
                case 3:  // QAM-like (amplitude + phase)
                    {
                        uint32_t amplitude = 8192 + ((Rng() >> 17) & 8191);  // Variable amplitude
                        uint32_t phase_offset = Rng() & 7;
                        float angle = phase_offset * M_PI / 4.0f;
                        state_.noise.next_sample = static_cast<int32_t>(amplitude * std::cos(angle));
                    }
                    break;
            }
        }
        
        // Generate carrier with modulation
        int16_t carrier = InterpolateWaveform(wav_sine, phase, 1024);
        int32_t result = (carrier * state_.noise.next_sample) >> 15;
        
        // Add some filtering to make it more musical
        state_.noise.integrator_state += ((result - state_.noise.integrator_state) * 16384) >> 15;
        
        *buffer++ = static_cast<int16_t>(state_.noise.integrator_state);
    }
    
    phase_ = phase;
}

}  // namespace braidy