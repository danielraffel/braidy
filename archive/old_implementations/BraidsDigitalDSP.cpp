// Braids Digital DSP Algorithms Implementation
// Ported from original Mutable Instruments Braids source

#include "BraidsDigitalDSP.h"
#include "BraidsLookupTables.h"
#include "BraidsConstants.h"
#include <cmath>

namespace braidy {

// Phase increment computation matching original Braids
uint32_t BraidsDigitalDSP::ComputePhaseIncrement(int16_t midi_pitch) {
    // Handle the exact same way as original Braids
    // midi_pitch is in format: MIDI note << 7
    // Use lut_oscillator_increments[] which is for 48kHz
    
    if (midi_pitch >= kHighestNote) {
        midi_pitch = kHighestNote - 1;
    }
    
    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;
    
    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
    }
    
    uint32_t a = LUT_OSCILLATOR_INCREMENTS[ref_pitch >> 4];
    uint32_t b = LUT_OSCILLATOR_INCREMENTS[(ref_pitch >> 4) + 1];
    uint32_t phase_increment = a + 
        (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
    phase_increment >>= num_shifts;
    return phase_increment;
}

uint32_t BraidsDigitalDSP::ComputeDelay(int16_t midi_pitch) {
    // Convert pitch to delay length for Karplus-Strong using phase increment
    // This avoids runtime frequency calculations by using the LUT approach
    
    if (midi_pitch >= kHighestNote - kOctave) {
        midi_pitch = kHighestNote - kOctave;
    }
    
    // Get phase increment using existing LUT-based method
    uint32_t phase_increment = ComputePhaseIncrement(midi_pitch);
    
    // Convert phase increment to delay samples
    if (phase_increment == 0) {
        return 4095;  // Maximum delay for very low frequencies
    }
    
    // Use 64-bit arithmetic to avoid overflow
    uint64_t delay_64 = (1ULL << 32) / phase_increment;
    uint32_t delay = static_cast<uint32_t>(delay_64);
    
    // Clamp to reasonable range for string/delay models
    return std::max(4U, std::min(delay, 4095U));
}

// VOSIM synthesis - Voice Simulation technique
void BraidsDigitalDSP::RenderVosim(int16_t pitch, int16_t* parameter,
                                   uint32_t phase, uint32_t phase_increment,
                                   const uint8_t* sync, int16_t* buffer, size_t size) {
    // Formant frequencies controlled by parameters
    state_.vow.formant_increment[0] = ComputePhaseIncrement(parameter[0] >> 1);
    state_.vow.formant_increment[1] = ComputePhaseIncrement(parameter[1] >> 1);
    
    while (size--) {
        phase += phase_increment;
        if (*sync++) {
            phase = 0;
            state_.vow.formant_phase[0] = 0;
            state_.vow.formant_phase[1] = 0;
        }
        
        int32_t sample = 16384 + 8192;
        
        // Two formant oscillators
        state_.vow.formant_phase[0] += state_.vow.formant_increment[0];
        sample += static_cast<int32_t>(std::sin(state_.vow.formant_phase[0] * 2.0f * M_PI / 4294967296.0f) * 16384) >> 1;
        
        state_.vow.formant_phase[1] += state_.vow.formant_increment[1];
        sample += static_cast<int32_t>(std::sin(state_.vow.formant_phase[1] * 2.0f * M_PI / 4294967296.0f) * 16384) >> 2;
        
        // Apply bell-shaped envelope
        float envelope = std::exp(-5.0f * (phase / 4294967296.0f));
        sample = static_cast<int32_t>(sample * envelope);
        
        if (phase < phase_increment) {
            state_.vow.formant_phase[0] = 0;
            state_.vow.formant_phase[1] = 0;
        }
        
        *buffer++ = SoftClip(sample);
    }
}

// Triple Ring Modulation
void BraidsDigitalDSP::RenderTripleRingMod(int16_t pitch, int16_t* parameter,
                                           uint32_t phase, uint32_t phase_increment,
                                           const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t modulator_phase = state_.vow.formant_phase[0];
    uint32_t modulator_phase_2 = state_.vow.formant_phase[1];
    
    // Modulator frequencies based on parameter offsets from carrier
    // Parameters already scaled in DSPDispatcher, use directly
    uint32_t modulator_phase_increment = ComputePhaseIncrement(
        pitch + parameter[0]
    );
    uint32_t modulator_phase_increment_2 = ComputePhaseIncrement(
        pitch + parameter[1]
    );
    
    while (size--) {
        phase += phase_increment;
        if (*sync++) {
            phase = 0;
            modulator_phase = 0;
            modulator_phase_2 = 0;
        }
        
        modulator_phase += modulator_phase_increment;
        modulator_phase_2 += modulator_phase_increment_2;
        
        // Carrier oscillator (sine wave with 90° phase offset as per spec)
        int32_t carrier = std::sin((phase + 0x40000000) * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        // Two modulators (sine waves)
        int32_t modulator_1 = std::sin(modulator_phase * 2.0f * M_PI / 4294967296.0f) * 32767;
        int32_t modulator_2 = std::sin(modulator_phase_2 * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        // Ring modulation
        int32_t sample = (carrier * modulator_1 >> 15);
        sample = (sample * modulator_2 >> 15);
        
        *buffer++ = SoftClip(sample);
    }
    
    state_.vow.formant_phase[0] = modulator_phase;
    state_.vow.formant_phase[1] = modulator_phase_2;
}

// Saw Swarm - Multiple detuned sawtooth oscillators
void BraidsDigitalDSP::RenderSawSwarm(int16_t pitch, int16_t* parameter,
                                      uint32_t phase, uint32_t phase_increment,
                                      const uint8_t* sync, int16_t* buffer, size_t size) {
    // Number of voices and detune amount from parameters
    int num_voices = 3 + (parameter[0] >> 13);  // 3-7 voices
    float detune = parameter[1] / 32767.0f * 0.05f;  // Up to 5% detune
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            for (int i = 0; i < 4; i++) {
                state_.vow.formant_phase[i] = 0;
            }
        }
        
        int32_t sample = 0;
        
        // Generate multiple detuned saws
        for (int i = 0; i < num_voices && i < 4; i++) {
            float detune_factor = 1.0f + detune * (i - num_voices/2.0f) / num_voices;
            uint32_t voice_increment = static_cast<uint32_t>(phase_increment * detune_factor);
            
            state_.vow.formant_phase[i] += voice_increment;
            
            // Sawtooth wave
            int32_t saw = (state_.vow.formant_phase[i] >> 16) - 32768;
            sample += saw / num_voices;
        }
        
        phase += phase_increment;
        *buffer++ = SoftClip(sample);
    }
}

// Comb Filter synthesis
void BraidsDigitalDSP::RenderCombFilter(int16_t pitch, int16_t* parameter,
                                        uint32_t phase, uint32_t phase_increment,
                                        const uint8_t* sync, int16_t* buffer, size_t size) {
    // Delay time controlled by parameter[0] (4-512 samples for resonant comb)
    uint32_t delay_time = 4 + (parameter[0] >> 6);  // 4-512 samples
    int16_t feedback = parameter[1] >> 1;  // Feedback amount (-16384 to +16384)
    
    if (delay_time > 511) delay_time = 511;
    
    while (size--) {
        phase += phase_increment;
        if (*sync++) {
            phase = 0;
        }
        
        // Input signal (sawtooth)
        int32_t input = (phase >> 16) - 32768;
        
        // Read from delay line
        uint16_t read_ptr = (state_.phys.write_ptr - delay_time) & 4095;
        int32_t delayed = state_.phys.delay_line[read_ptr];
        
        // Comb filter: output = input + feedback * delayed
        int32_t output = input + ((delayed * feedback) >> 15);
        
        // Store input (not output) in delay line for proper comb filtering
        state_.phys.delay_line[state_.phys.write_ptr] = SoftClip(input);
        state_.phys.write_ptr = (state_.phys.write_ptr + 1) & 4095;
        
        *buffer++ = SoftClip(output);
    }
}

// Toy/Lo-fi synthesis with sample rate reduction and bit crushing
void BraidsDigitalDSP::RenderToy(int16_t pitch, int16_t* parameter,
                                 uint32_t phase, uint32_t phase_increment,
                                 const uint8_t* sync, int16_t* buffer, size_t size) {
    // Sample rate reduction from parameter[0] - use scaled parameters from dispatcher
    uint32_t sr_divider = 1 + parameter[0];  // 1-32x reduction
    if (sr_divider > 32) sr_divider = 32;
    
    // Bit depth from parameter[1]
    int bit_reduction = parameter[1];  // 1-8 bit reduction
    if (bit_reduction > 7) bit_reduction = 7;
    int bit_mask = 0xFFFF << bit_reduction;
    
    // Use instance variables instead of static for thread safety
    if (state_.noise.clock_phase == 0) {
        state_.noise.clock_phase = 1;
        state_.noise.sample_hold = 0;
    }
    
    while (size--) {
        phase += phase_increment;
        if (*sync++) {
            phase = 0;
            state_.noise.clock_phase = 1;
        }
        
        // Generate base waveform (square wave)
        int16_t sample = (phase < 0x80000000) ? 16384 : -16384;
        
        // Sample rate reduction
        if (state_.noise.clock_phase >= sr_divider) {
            state_.noise.clock_phase = 1;
            // Bit crushing
            state_.noise.sample_hold = sample & bit_mask;
        } else {
            state_.noise.clock_phase++;
        }
        
        *buffer++ = state_.noise.sample_hold;
    }
}

// FM Synthesis with proper carrier/modulator ratios
void BraidsDigitalDSP::RenderFM(int16_t pitch, int16_t* parameter,
                                uint32_t phase, uint32_t phase_increment,
                                const uint8_t* sync, int16_t* buffer, size_t size) {
    // Classic FM ratios
    const float fm_ratios[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 7.0f, 8.0f};
    int ratio_index = (parameter[0] >> 13) & 7;
    float ratio = fm_ratios[ratio_index];
    
    // FM amount from parameter[1]
    int32_t fm_amount = parameter[1];
    
    uint32_t modulator_increment = static_cast<uint32_t>(phase_increment * ratio);
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            state_.fm.modulator_phase = 0;
        }
        
        // Modulator
        state_.fm.modulator_phase += modulator_increment;
        int32_t modulator = std::sin(state_.fm.modulator_phase * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        // Apply FM to carrier phase
        uint32_t modulated_phase = phase + ((modulator * fm_amount) >> 8);
        
        // Carrier
        int32_t carrier = std::sin(modulated_phase * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        phase += phase_increment;
        *buffer++ = SoftClip(carrier);
    }
}

// Feedback FM
void BraidsDigitalDSP::RenderFeedbackFM(int16_t pitch, int16_t* parameter,
                                        uint32_t phase, uint32_t phase_increment,
                                        const uint8_t* sync, int16_t* buffer, size_t size) {
    int32_t feedback_amount = parameter[0];
    int32_t fm_amount = parameter[1];
    
    while (size--) {
        if (*sync++) {
            phase = 0;
            state_.fm.feedback = 0;
        }
        
        // Carrier with feedback modulation
        uint32_t modulated_phase = phase + ((state_.fm.feedback * feedback_amount) >> 12);
        int32_t carrier = std::sin(modulated_phase * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        // Update feedback (lowpass filtered)
        state_.fm.feedback = OnePole(carrier, state_.fm.feedback, 16384);
        
        phase += phase_increment;
        *buffer++ = SoftClip(carrier);
    }
}

// Karplus-Strong plucked string synthesis
void BraidsDigitalDSP::RenderPlucked(int16_t pitch, int16_t* parameter,
                                     const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t delay = ComputeDelay(pitch);
    int16_t damping = 32767 - (parameter[0] >> 1);  // Damping factor
    int16_t brightness = parameter[1];  // Brightness (affects filtering)
    
    while (size--) {
        if (*sync++) {
            // Excite string with noise burst
            for (uint32_t i = 0; i < delay && i < 4096; i++) {
                state_.phys.delay_line[i] = (GetNoise() >> 16) - 16384;
            }
            state_.phys.write_ptr = 0;
        }
        
        // Read from delay line
        uint16_t read_ptr = (state_.phys.write_ptr - delay) & 4095;
        int32_t delayed = state_.phys.delay_line[read_ptr];
        
        // Lowpass filter (controls brightness)
        int32_t filtered = OnePole(delayed, state_.phys.filter_state, brightness);
        state_.phys.filter_state = filtered;
        
        // Apply damping
        filtered = (filtered * damping) >> 15;
        
        // Write back to delay line
        state_.phys.delay_line[state_.phys.write_ptr] = SoftClip(filtered);
        state_.phys.write_ptr = (state_.phys.write_ptr + 1) & 4095;
        
        *buffer++ = SoftClip(filtered);
    }
}

// Struck Bell synthesis using modal synthesis approximation
void BraidsDigitalDSP::RenderStruckBell(int16_t pitch, int16_t* parameter,
                                        const uint8_t* sync, int16_t* buffer, size_t size) {
    // Bell partials (inharmonic)
    const float partials[] = {1.0f, 2.76f, 5.40f, 8.93f};
    const float amplitudes[] = {1.0f, 0.5f, 0.25f, 0.125f};
    
    int16_t mallet_hardness = parameter[0];  // Affects attack
    int16_t inharmonicity = parameter[1];  // Detunes partials
    
    while (size--) {
        if (*sync++) {
            // Reset all partial phases
            for (int i = 0; i < 4; i++) {
                state_.vow.formant_phase[i] = 0;
            }
            state_.phys.pluck_damping = 32767;
        }
        
        int32_t sample = 0;
        
        // Generate bell partials
        for (int i = 0; i < 4; i++) {
            float partial_ratio = partials[i] + (inharmonicity / 32767.0f * i * 0.1f);
            uint32_t partial_increment = ComputePhaseIncrement(
                pitch + static_cast<int16_t>(1200 * std::log2(partial_ratio))
            );
            
            state_.vow.formant_phase[i] += partial_increment;
            
            int32_t partial = std::sin(state_.vow.formant_phase[i] * 2.0f * M_PI / 4294967296.0f) 
                            * 32767 * amplitudes[i];
            sample += partial;
        }
        
        // Apply exponential decay
        sample = (sample * state_.phys.pluck_damping) >> 15;
        state_.phys.pluck_damping = state_.phys.pluck_damping - (state_.phys.pluck_damping >> 9);
        
        *buffer++ = SoftClip(sample);
    }
}

// Kick drum synthesis
void BraidsDigitalDSP::RenderKick(int16_t pitch, int16_t* parameter,
                                  const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t punch = parameter[0];  // Initial pitch bend amount
    int16_t decay = parameter[1];  // Decay time (already inverted in dispatcher)
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            // Trigger new kick
            state_.drum.drum_amplitude[0] = 32767;  // Main amplitude envelope
            state_.drum.drum_amplitude[1] = punch;  // Pitch envelope amount
            state_.drum.drum_phase[0] = 0;
        }
        
        // Calculate current pitch with exponential drop
        int32_t pitch_offset = (state_.drum.drum_amplitude[1] * punch) >> 15;
        int32_t current_pitch = pitch + (pitch_offset >> 4);  // Scale down pitch bend
        uint32_t osc_increment = ComputePhaseIncrement(static_cast<int16_t>(current_pitch));
        
        // Sine oscillator
        state_.drum.drum_phase[0] += osc_increment;
        int32_t sample = std::sin(state_.drum.drum_phase[0] * 2.0f * M_PI / 4294967296.0f) * 32767;
        
        // Apply main amplitude envelope
        sample = (sample * state_.drum.drum_amplitude[0]) >> 15;
        
        // Update envelopes - fast pitch decay, controlled amplitude decay
        state_.drum.drum_amplitude[1] = (state_.drum.drum_amplitude[1] * 30000) >> 15;  // Very fast pitch decay
        state_.drum.drum_amplitude[0] = (state_.drum.drum_amplitude[0] * decay) >> 15;   // Controlled amplitude decay
        
        buffer[i] = SoftClip(sample);
    }
}

// Snare drum synthesis
void BraidsDigitalDSP::RenderSnare(int16_t pitch, int16_t* parameter,
                                   const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t snappy = parameter[0];  // Noise amount
    int16_t tone = parameter[1];  // Tone frequency
    
    // Initialize snare if needed
    if (state_.drum.drum_amplitude[2] == 0) {
        state_.drum.drum_amplitude[2] = 32767;  // Envelope
        state_.drum.drum_phase[2] = 0;
        state_.drum.drum_phase[3] = 0;
    }
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            state_.drum.drum_amplitude[2] = 32767;
            state_.drum.drum_phase[2] = 0;
            state_.drum.drum_phase[3] = 0;
        }
        
        // Tone component (two detuned oscillators)
        state_.drum.drum_phase[2] += ComputePhaseIncrement(pitch);
        state_.drum.drum_phase[3] += ComputePhaseIncrement(pitch + (tone >> 8));
        
        int32_t tone1 = std::sin(state_.drum.drum_phase[2] * 2.0f * M_PI / 4294967296.0f) * 16384;
        int32_t tone2 = std::sin(state_.drum.drum_phase[3] * 2.0f * M_PI / 4294967296.0f) * 16384;
        
        // Noise component
        int32_t noise = (GetNoise() >> 16) - 16384;
        noise = (noise * snappy) >> 15;
        
        // Mix and apply envelope
        int32_t sample = ((tone1 + tone2) / 2 + noise) * state_.drum.drum_amplitude[2] >> 15;
        
        // Fast envelope decay
        state_.drum.drum_amplitude[2] = (state_.drum.drum_amplitude[2] * 32000) >> 15;
        
        buffer[i] = SoftClip(sample);
    }
}

// Filtered Noise
void BraidsDigitalDSP::RenderFilteredNoise(int16_t pitch, int16_t* parameter,
                                           const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t cutoff = parameter[0];
    int16_t resonance = parameter[1] >> 1;
    
    while (size--) {
        // White noise source
        int32_t noise = (GetNoise() >> 16) - 16384;
        
        // Simple resonant lowpass filter
        int32_t filtered = OnePole(noise, state_.noise.filter_state[0], cutoff);
        
        // Add resonance (feedback)
        filtered += (state_.noise.filter_state[0] * resonance) >> 15;
        
        state_.noise.filter_state[0] = SoftClip(filtered);
        
        *buffer++ = state_.noise.filter_state[0];
    }
}

// Clocked Noise
void BraidsDigitalDSP::RenderClockedNoise(int16_t pitch, int16_t* parameter,
                                          const uint8_t* sync, int16_t* buffer, size_t size) {
    uint32_t clock_rate = ComputePhaseIncrement(pitch + ((parameter[0] - 16384) >> 7));
    int16_t randomness = parameter[1];
    
    while (size--) {
        state_.noise.clock_phase += clock_rate;
        
        // Sample and hold on clock edge
        if (state_.noise.clock_phase < clock_rate) {
            // New random value
            int32_t new_value = (GetNoise() >> 16) - 16384;
            
            // Mix with previous value based on randomness
            state_.noise.sample_hold = state_.noise.sample_hold + 
                                       ((new_value - state_.noise.sample_hold) * randomness >> 15);
        }
        
        *buffer++ = state_.noise.sample_hold;
    }
}

// Vowel synthesis stub (simplified for now)
void BraidsDigitalDSP::RenderVowel(int16_t pitch, int16_t* parameter,
                                   uint32_t phase, uint32_t phase_increment,
                                   const uint8_t* sync, int16_t* buffer, size_t size) {
    // Simplified vowel - will be enhanced with proper formant filters
    RenderVosim(pitch, parameter, phase, phase_increment, sync, buffer, size);
}

// Bowed string physical model (waveguide)
void BraidsDigitalDSP::RenderBowed(int16_t pitch, int16_t* parameter,
                                   const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t bow_pressure = parameter[0];  // Controls roughness
    int16_t bow_position = parameter[1];  // Affects harmonics
    
    uint32_t delay = ComputeDelay(pitch);
    if (delay > 4095) delay = 4095;
    
    while (size--) {
        if (*sync++) {
            // Reset string state
            state_.phys.string_state = 0;
            state_.phys.filter_state = 0;
        }
        
        // Read from delay line
        uint16_t read_ptr = (state_.phys.write_ptr - delay) & 4095;
        int32_t delayed = state_.phys.delay_line[read_ptr];
        
        // Non-linear bow friction model
        int32_t velocity = delayed - state_.phys.string_state;
        int32_t friction = 0;
        
        if (abs(velocity) < bow_pressure) {
            // Stick mode
            friction = velocity * 32767 / bow_pressure;
        } else {
            // Slip mode
            friction = (velocity > 0 ? 16384 : -16384);
        }
        
        // Apply bow position (affects harmonic content)
        int32_t excitation = (friction * bow_position) >> 15;
        
        // Feed back into delay line with filtering
        int32_t filtered = OnePole(delayed + excitation, state_.phys.filter_state, 28000);
        state_.phys.filter_state = filtered;
        
        state_.phys.delay_line[state_.phys.write_ptr] = SoftClip(filtered);
        state_.phys.write_ptr = (state_.phys.write_ptr + 1) & 4095;
        
        state_.phys.string_state = delayed;
        *buffer++ = SoftClip(filtered);
    }
}

// Blown instrument physical model (waveguide with turbulence)
void BraidsDigitalDSP::RenderBlown(int16_t pitch, int16_t* parameter,
                                   const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t breath_pressure = parameter[0];
    int16_t embouchure = parameter[1];  // Lip tension/brightness
    
    uint32_t delay = ComputeDelay(pitch);
    if (delay > 4095) delay = 4095;
    
    while (size--) {
        if (*sync++) {
            memset(state_.phys.delay_line, 0, sizeof(state_.phys.delay_line));
            state_.phys.write_ptr = 0;
        }
        
        // Generate turbulence noise
        int32_t turbulence = (GetNoise() >> 18) - 2048;
        turbulence = (turbulence * breath_pressure) >> 15;
        
        // Read from delay line
        uint16_t read_ptr = (state_.phys.write_ptr - delay) & 4095;
        int32_t delayed = state_.phys.delay_line[read_ptr];
        
        // Reed/jet interaction (non-linear)
        int32_t pressure_diff = breath_pressure - (delayed >> 1);
        int32_t flow = 0;
        
        if (pressure_diff > 0) {
            // Flow through reed opening
            flow = (pressure_diff * embouchure) >> 15;
            flow += turbulence;  // Add turbulence
        }
        
        // Feed back with filtering
        int32_t output = delayed + flow;
        output = OnePole(output, state_.phys.filter_state, embouchure);
        state_.phys.filter_state = output;
        
        state_.phys.delay_line[state_.phys.write_ptr] = SoftClip(output);
        state_.phys.write_ptr = (state_.phys.write_ptr + 1) & 4095;
        
        *buffer++ = SoftClip(output);
    }
}

// Flute physical model
void BraidsDigitalDSP::RenderFluted(int16_t pitch, int16_t* parameter,
                                    const uint8_t* sync, int16_t* buffer, size_t size) {
    // Similar to blown but with different embouchure model
    int16_t breath = parameter[0];
    int16_t tone = parameter[1];
    
    uint32_t delay = ComputeDelay(pitch);
    if (delay > 4095) delay = 4095;
    
    while (size--) {
        if (*sync++) {
            memset(state_.phys.delay_line, 0, sizeof(state_.phys.delay_line));
            state_.phys.write_ptr = 0;
        }
        
        // Jet turbulence (edge tone)
        int32_t jet_noise = (GetNoise() >> 17) - 4096;
        jet_noise = (jet_noise * breath) >> 15;
        
        // Read from bore delay
        uint16_t read_ptr = (state_.phys.write_ptr - delay) & 4095;
        int32_t delayed = state_.phys.delay_line[read_ptr];
        
        // Jet deflection by acoustic flow
        int32_t deflection = (delayed * tone) >> 16;
        int32_t excitation = jet_noise + deflection;
        
        // Low-pass filter (tone hole effects)
        int32_t output = OnePole(delayed + excitation, state_.phys.filter_state, tone);
        state_.phys.filter_state = output;
        
        state_.phys.delay_line[state_.phys.write_ptr] = SoftClip(output);
        state_.phys.write_ptr = (state_.phys.write_ptr + 1) & 4095;
        
        *buffer++ = SoftClip(output);
    }
}

// Struck drum synthesis
void BraidsDigitalDSP::RenderStruckDrum(int16_t pitch, int16_t* parameter,
                                        const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t decay = 32767 - (parameter[0] >> 2);  // Decay time
    int16_t tone = parameter[1];  // Tone/pitch
    
    // Drum membrane modes (more drum-like ratios)
    const float modes[] = {1.0f, 1.593f, 2.136f, 2.296f, 2.653f};
    const float mode_amps[] = {1.0f, 0.4f, 0.25f, 0.15f, 0.1f};
    const float mode_decays[] = {1.0f, 0.8f, 0.6f, 0.4f, 0.3f}; // Different decay rates
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            // Strike drum - set mode amplitudes and add initial noise burst
            for (int j = 0; j < 5; j++) {
                state_.drum.drum_amplitude[j] = static_cast<int16_t>(32767 * mode_amps[j]);
                state_.drum.drum_phase[j] = GetNoise() >> 16;  // Random initial phase
            }
        }
        
        int32_t sample = 0;
        
        // Sum all drum modes
        for (int j = 0; j < 5; j++) {
            // Calculate mode frequency
            float mode_ratio = modes[j] + (tone / 32767.0f) * 0.1f;  // Slight detuning with tone
            int16_t mode_pitch = pitch + static_cast<int16_t>(1200 * std::log2(mode_ratio));
            uint32_t mode_increment = ComputePhaseIncrement(mode_pitch);
            
            state_.drum.drum_phase[j] += mode_increment;
            
            // Use sine wave but with some harmonic distortion for drum character
            int32_t mode_sin = std::sin(state_.drum.drum_phase[j] * 2.0f * M_PI / 4294967296.0f) * 32767;
            
            // Add slight distortion/harmonics for more drum-like character
            int32_t mode_sample = mode_sin + (mode_sin * mode_sin >> 16) / 8;
            mode_sample = (mode_sample * state_.drum.drum_amplitude[j]) >> 15;
            
            // Different decay rates for each mode
            int16_t mode_decay = decay + static_cast<int16_t>((32767 - decay) * (1.0f - mode_decays[j]));
            state_.drum.drum_amplitude[j] = (state_.drum.drum_amplitude[j] * mode_decay) >> 15;
            
            sample += mode_sample >> 3;  // Mix modes
        }
        
        buffer[i] = SoftClip(sample);
    }
}

// Cymbal synthesis (metallic noise)
void BraidsDigitalDSP::RenderCymbal(int16_t pitch, int16_t* parameter,
                                    const uint8_t* sync, int16_t* buffer, size_t size) {
    int16_t decay = 32767 - (parameter[0] >> 2);
    int16_t tone = parameter[1];  // Brightness/tone
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            state_.drum.drum_amplitude[4] = 32767;  // Main envelope
            // Reset metallic resonators with random phases
            for (int j = 0; j < 6; j++) {
                state_.cymb.hihat_phase[j] = GetNoise();
                state_.cymb.hihat_amplitude[j] = 8192;  // Reduced initial amplitude
            }
        }
        
        int32_t sample = 0;
        
        // Generate metallic sound with multiple inharmonic oscillators
        const float metallic_ratios[] = {1.0f, 1.34f, 1.8f, 2.67f, 3.73f, 5.11f};
        
        for (int j = 0; j < 6; j++) {
            // Inharmonic frequencies based on metallic ratios
            float ratio = metallic_ratios[j] + (tone / 32767.0f) * 0.2f;
            int16_t harmonic_pitch = pitch + static_cast<int16_t>(1200 * std::log2(ratio));
            uint32_t freq = ComputePhaseIncrement(harmonic_pitch);
            
            state_.cymb.hihat_phase[j] += freq;
            
            // Mix sine wave with controlled noise for metallic texture
            int32_t osc = std::sin(state_.cymb.hihat_phase[j] * 2.0f * M_PI / 4294967296.0f) * 16384;
            int32_t noise = (GetNoise() >> 20) - 1024;  // Reduced noise level
            
            int32_t metallic_osc = ((osc * 3 + noise) >> 2) * state_.cymb.hihat_amplitude[j] >> 15;
            sample += metallic_osc >> 3;  // Mix down
            
            // Individual oscillator decay
            state_.cymb.hihat_amplitude[j] = (state_.cymb.hihat_amplitude[j] * (decay + 1000)) >> 15;
        }
        
        // High-pass filter for shimmer
        int32_t hp_sample = sample - state_.cymb.filter_state;
        state_.cymb.filter_state = OnePole(sample, state_.cymb.filter_state, tone >> 1);
        
        // Apply main envelope
        int32_t final_sample = (hp_sample * state_.drum.drum_amplitude[4]) >> 15;
        state_.drum.drum_amplitude[4] = (state_.drum.drum_amplitude[4] * decay) >> 15;
        
        buffer[i] = SoftClip(final_sample);
    }
}

// Granular cloud synthesis

// Twin peaks noise (dual resonant filters)
void BraidsDigitalDSP::RenderTwinPeaksNoise(int16_t pitch, int16_t* parameter,
                                            const uint8_t* sync, int16_t* buffer, size_t size) {
    // Use parameters to control peak frequencies and resonance
    int16_t peak1_freq = 1000 + (parameter[0] >> 3);  // 1000-9000 Hz range
    int16_t peak2_freq = 2000 + (parameter[1] >> 3);  // 2000-10000 Hz range
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            // Reset filter states on sync
            for (int j = 0; j < 4; j++) {
                state_.noise.filter_state[j] = 0;
            }
        }
        
        // White noise source
        int32_t noise = (GetNoise() >> 16) - 16384;
        
        // First resonant peak - proper biquad filter approximation
        int32_t peak1_input = noise - state_.noise.filter_state[0];
        state_.noise.filter_state[0] += (peak1_input * peak1_freq) >> 15;
        state_.noise.filter_state[1] += (state_.noise.filter_state[0] * peak1_freq) >> 15;
        int32_t out1 = state_.noise.filter_state[1];
        
        // Second resonant peak
        int32_t peak2_input = noise - state_.noise.filter_state[2];
        state_.noise.filter_state[2] += (peak2_input * peak2_freq) >> 15;
        state_.noise.filter_state[3] += (state_.noise.filter_state[2] * peak2_freq) >> 15;
        int32_t out2 = state_.noise.filter_state[3];
        
        // Mix and amplify the two peaks
        int32_t mixed = (out1 + out2);
        buffer[i] = SoftClip(mixed);
    }
}

// Additional stub implementations for remaining algorithms
void BraidsDigitalDSP::RenderVowelFOF(int16_t pitch, int16_t* parameter,
                                      uint32_t phase, uint32_t phase_increment,
                                      const uint8_t* sync, int16_t* buffer, size_t size) {
    RenderVowel(pitch, parameter, phase, phase_increment, sync, buffer, size);
}

void BraidsDigitalDSP::RenderHarmonics(int16_t pitch, int16_t* parameter,
                                       uint32_t phase, uint32_t phase_increment,
                                       const uint8_t* sync, int16_t* buffer, size_t size) {
    // Generate harmonics based on parameter[0] for harmonic content
    int num_harmonics = 1 + (parameter[0] >> 12);  // 1-16 harmonics
    if (num_harmonics > 16) num_harmonics = 16;
    int16_t brightness = parameter[1];
    
    // Use proper phase tracking for each harmonic
    for (size_t i = 0; i < size; ++i) {
        phase += phase_increment;
        if (sync && sync[i]) {
            phase = 0;
        }
        
        int32_t sample = 0;
        
        // Generate each harmonic with proper amplitude rolloff
        for (int h = 1; h <= num_harmonics; h++) {
            // Calculate harmonic phase directly from fundamental phase
            uint32_t harmonic_phase = phase * h;
            
            // Sine wave with 1/h amplitude rolloff
            int32_t harmonic = std::sin(harmonic_phase * 2.0f * M_PI / 4294967296.0f) * (32767 / h);
            sample += harmonic;
        }
        
        // Apply brightness/rolloff control
        sample = (sample * brightness) >> 15;
        
        buffer[i] = SoftClip(sample);
    }
}

void BraidsDigitalDSP::RenderDigitalFilter(int filter_type, int16_t pitch, int16_t* parameter,
                                          uint32_t phase, uint32_t phase_increment,
                                          const uint8_t* sync, int16_t* buffer, size_t size) {
    // Digital filter simulation (placeholder for now)
    // Will implement multimode filter with resonance
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

void BraidsDigitalDSP::RenderChaoticFeedbackFM(int16_t pitch, int16_t* parameter,
                                              uint32_t phase, uint32_t phase_increment,
                                              const uint8_t* sync, int16_t* buffer, size_t size) {
    // Use the chaotic systems implementation
    chaotic_.RenderChaoticFeedbackFM(pitch, parameter, phase, phase_increment, sync, buffer, size);
}

void BraidsDigitalDSP::RenderGranularCloud(int16_t pitch, int16_t* parameter,
                                          const uint8_t* sync, int16_t* buffer, size_t size) {
    // Use the granular synthesis implementation
    granular_.RenderGranularCloud(pitch, parameter, sync, buffer, size);
}

void BraidsDigitalDSP::RenderParticleNoise(int16_t pitch, int16_t* parameter,
                                          const uint8_t* sync, int16_t* buffer, size_t size) {
    // Use the granular synthesis implementation for particle noise
    granular_.RenderParticleNoise(pitch, parameter, sync, buffer, size);
}

void BraidsDigitalDSP::RenderDigitalModulation(int16_t pitch, int16_t* parameter,
                                              const uint8_t* sync, int16_t* buffer, size_t size) {
    // Digital modulation: QPSK/bit-crushed modulation effect
    uint32_t phase_increment = ComputePhaseIncrement(pitch);
    uint32_t phase = 0;
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase = 0;
        }
        
        // Generate carrier
        int32_t carrier = Sin(phase);
        
        // Apply digital modulation based on parameters
        uint32_t modulation_rate = phase_increment >> (parameter[0] >> 11);
        uint32_t bit_mask = 0xFFFFFFFF << (16 - (parameter[1] >> 11));
        
        // Apply phase quantization for QPSK-like effect
        uint32_t quantized_phase = phase & bit_mask;
        int32_t modulated = Sin(quantized_phase);
        
        // Mix carrier and modulated signal
        int32_t output = (carrier * (32767 - parameter[0]) + modulated * parameter[0]) >> 15;
        
        buffer[i] = ClipS16(output);
        phase += modulation_rate;
    }
}

} // namespace braidy