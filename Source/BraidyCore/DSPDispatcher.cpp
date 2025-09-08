// DSP Dispatcher Implementation
// Centralizes all algorithm routing to eliminate duplicate code

#include "DSPDispatcher.h"
#include "BraidyMath.h"
#include <cstring>

namespace braidy {

DSPDispatcher::DSPDispatcher() {
    // Initialize shared state
    memset(&state_, 0, sizeof(state_));
    state_.noise_lfsr = 1;
    
    // Initialize wavetable manager
    wavetable_manager_.Init();
}

void DSPDispatcher::Strike() {
    digital_dsp_.Strike();
    // WavetableManager doesn't have Strike() - it's just a lookup table
    state_.envelope = 32767;
}

void DSPDispatcher::Process(MacroOscillatorShape shape,
                           int16_t pitch,
                           int16_t* parameter,
                           uint32_t& phase,
                           uint32_t phase_increment,
                           const uint8_t* sync,
                           int16_t* buffer,
                           size_t size) {
    
    // Scale parameters appropriately for each algorithm
    int16_t scaled_params[2];
    ScaleParameters(shape, parameter, scaled_params);
    
    // Route to appropriate DSP implementation
    switch (shape) {
        // Ring modulation
        case MacroOscillatorShape::TRIPLE_RING_MOD:
            digital_dsp_.RenderTripleRingMod(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Saw swarm
        case MacroOscillatorShape::SAW_SWARM:
            digital_dsp_.RenderSawSwarm(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Comb filter
        case MacroOscillatorShape::SAW_COMB:
            digital_dsp_.RenderCombFilter(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Toy/lo-fi
        case MacroOscillatorShape::TOY:
            digital_dsp_.RenderToy(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Digital filters
        case MacroOscillatorShape::DIGITAL_FILTER_LP:
        case MacroOscillatorShape::DIGITAL_FILTER_PK:
        case MacroOscillatorShape::DIGITAL_FILTER_BP:
        case MacroOscillatorShape::DIGITAL_FILTER_HP:
            {
                int filter_type = static_cast<int>(shape) - static_cast<int>(MacroOscillatorShape::DIGITAL_FILTER_LP);
                digital_dsp_.RenderDigitalFilter(filter_type, pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            }
            break;
            
        // Formant synthesis
        case MacroOscillatorShape::VOSIM:
            digital_dsp_.RenderVosim(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::VOWEL:
            digital_dsp_.RenderVowel(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::VOWEL_FOF:
            digital_dsp_.RenderVowelFOF(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Harmonics
        case MacroOscillatorShape::HARMONICS:
            digital_dsp_.RenderHarmonics(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // FM synthesis
        case MacroOscillatorShape::FM:
            digital_dsp_.RenderFM(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::FEEDBACK_FM:
            digital_dsp_.RenderFeedbackFM(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::CHAOTIC_FEEDBACK_FM:
            digital_dsp_.RenderChaoticFeedbackFM(pitch, scaled_params, phase, phase_increment, sync, buffer, size);
            break;
            
        // Physical models - strings
        case MacroOscillatorShape::PLUCKED:
            digital_dsp_.RenderPlucked(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::BOWED:
            digital_dsp_.RenderBowed(pitch, scaled_params, sync, buffer, size);
            break;
            
        // Physical models - winds
        case MacroOscillatorShape::BLOWN:
            digital_dsp_.RenderBlown(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::FLUTED:
            digital_dsp_.RenderFluted(pitch, scaled_params, sync, buffer, size);
            break;
            
        // Physical models - percussion
        case MacroOscillatorShape::STRUCK_BELL:
            digital_dsp_.RenderStruckBell(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::STRUCK_DRUM:
            digital_dsp_.RenderStruckDrum(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::KICK:
            digital_dsp_.RenderKick(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::CYMBAL:
            digital_dsp_.RenderCymbal(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::SNARE:
            digital_dsp_.RenderSnare(pitch, scaled_params, sync, buffer, size);
            break;
            
        // Wavetables
        case MacroOscillatorShape::WAVETABLES: {
            // Standard wavetable synthesis - morph through wavetables
            uint16_t table_index = scaled_params[0];  // TIMBRE selects table
            static int debug_wtbl_count = 0;
            if (debug_wtbl_count < 3) {
                printf("DSPDispatcher::WAVETABLES DEBUG #%d:\n", debug_wtbl_count);
                printf("  scaled_params[0]=%d, scaled_params[1]=%d\n", scaled_params[0], scaled_params[1]);
                printf("  table_index=%u, phase=0x%08X, phase_increment=0x%08X\n", table_index, phase, phase_increment);
                debug_wtbl_count++;
            }
            
            while (size--) {
                // Handle sync - per-sample edge detection
                if (sync && *sync++) {
                    phase = 0;
                }
                *buffer = wavetable_manager_.RenderWavetable(phase, table_index);
                phase += phase_increment;
                buffer++;
            }
            
            if (debug_wtbl_count <= 3) {
                printf("  First few samples: %d, %d, %d, %d\n", 
                       buffer[0], buffer[1], buffer[2], buffer[3]);
            }
            break;
        }
            
        case MacroOscillatorShape::WAVE_MAP: {
            // 2D wavetable navigation  
            uint16_t x_pos = scaled_params[0];  // TIMBRE = X position
            uint16_t y_pos = scaled_params[1];  // COLOR = Y position
            static int debug_wmap_count = 0;
            if (debug_wmap_count < 3) {
                printf("DSPDispatcher::WAVE_MAP DEBUG #%d:\n", debug_wmap_count);
                printf("  x_pos=%u, y_pos=%u, phase=0x%08X, phase_increment=0x%08X\n", x_pos, y_pos, phase, phase_increment);
                debug_wmap_count++;
            }
            
            // Store original buffer for debug output
            int16_t* orig_buffer = buffer;
            size_t orig_size = size;
            
            while (size--) {
                // Handle sync - per-sample edge detection
                if (sync && *sync++) {
                    phase = 0;
                }
                *buffer = wavetable_manager_.RenderWaveMap(phase, x_pos, y_pos);
                phase += phase_increment;
                buffer++;
            }
            
            // Restore buffer pointer for debug output
            buffer = orig_buffer;
            size = orig_size;
            
            if (debug_wmap_count <= 3) {
                printf("  WMAP samples: %d, %d, %d, %d\n", 
                       buffer[0], buffer[1], buffer[2], buffer[3]);
            }
            break;
        }
            
        case MacroOscillatorShape::WAVE_LINE: {
            // Wavetable sweeping with a line
            uint16_t sweep_pos = scaled_params[0];  // TIMBRE = sweep position
            static int debug_wlin_count = 0;
            if (debug_wlin_count < 3) {
                printf("DSPDispatcher::WAVE_LINE DEBUG #%d:\n", debug_wlin_count);
                printf("  sweep_pos=%u, phase=0x%08X, phase_increment=0x%08X\n", sweep_pos, phase, phase_increment);
                debug_wlin_count++;
            }
            
            // Store original buffer for debug output
            int16_t* orig_buffer = buffer;
            size_t orig_size = size;
            
            while (size--) {
                // Handle sync - per-sample edge detection
                if (sync && *sync++) {
                    phase = 0;
                }
                *buffer = wavetable_manager_.RenderWaveLine(phase, sweep_pos);
                phase += phase_increment;
                buffer++;
            }
            
            // Restore buffer pointer for debug output
            buffer = orig_buffer;
            size = orig_size;
            
            if (debug_wlin_count <= 3) {
                printf("  WLIN samples: %d, %d, %d, %d\n", 
                       buffer[0], buffer[1], buffer[2], buffer[3]);
            }
            break;
        }
            
        case MacroOscillatorShape::WAVE_PARAPHONIC: {
            // Paraphonic wavetable synthesis - multiple detuned voices
            uint16_t spread = scaled_params[0];  // TIMBRE = detune spread
            uint16_t table_base = scaled_params[1];  // COLOR = base wavetable
            
            // Initialize phase array for voices
            uint32_t phases[4];
            for (int i = 0; i < 4; ++i) {
                phases[i] = phase + (i * 0x10000000);  // Different starting phases
            }
            
            // Clear buffer first
            std::memset(buffer, 0, size * sizeof(int16_t));
            
            // Render each voice
            for (int voice = 0; voice < 4; ++voice) {
                uint32_t voice_phase = phases[voice];
                uint16_t voice_table = table_base + (voice * spread >> 8);
                if (voice_table >= 128) voice_table = 127;  // Clamp to valid range
                
                // Calculate detune for this voice
                int32_t detune_factor = (voice - 2) * spread;  // -2, -1, 0, +1 relative to center
                int32_t voice_increment = phase_increment + (detune_factor >> 12);
                
                // Process each sample for this voice
                for (size_t i = 0; i < size; ++i) {
                    // Handle sync - per-sample edge detection
                    if (sync && sync[i]) {
                        voice_phase = 0;
                    }
                    
                    // Render sample from wavetable
                    int16_t sample = wavetable_manager_.RenderWavetable(voice_phase, voice_table << 8);
                    
                    // Mix into buffer (divide by 4 for 4 voices)
                    int32_t mixed = buffer[i] + (sample >> 2);
                    buffer[i] = ClipS16(mixed);
                    
                    voice_phase += voice_increment;
                }
            }
            
            // Update main phase for sync
            phase += phase_increment * size;
            break;
        }
            
        // Noise generators
        case MacroOscillatorShape::FILTERED_NOISE:
            digital_dsp_.RenderFilteredNoise(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::TWIN_PEAKS_NOISE:
            digital_dsp_.RenderTwinPeaksNoise(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::CLOCKED_NOISE:
            digital_dsp_.RenderClockedNoise(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::GRANULAR_CLOUD:
            digital_dsp_.RenderGranularCloud(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::PARTICLE_NOISE:
            digital_dsp_.RenderParticleNoise(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::DIGITAL_MODULATION:
            // Digital modulation/glitch effects - route to proper implementation
            digital_dsp_.RenderDigitalModulation(pitch, scaled_params, sync, buffer, size);
            break;
            
        case MacroOscillatorShape::QUESTION_MARK:
            // Easter egg - random algorithm each time
            {
                uint32_t random_algo = (GetNoise() >> 24) % 47;
                auto random_shape = static_cast<MacroOscillatorShape>(random_algo);
                Process(random_shape, pitch, parameter, phase, phase_increment, sync, buffer, size);
            }
            break;
            
        default:
            // Fallback - generate silence for unimplemented algorithms
            printf("DSPDispatcher: Unimplemented algorithm %d, generating silence\n", static_cast<int>(shape));
            memset(buffer, 0, size * sizeof(int16_t));
            break;
    }
}

void DSPDispatcher::ScaleParameters(MacroOscillatorShape shape, int16_t* in_params, int16_t* out_params) {
    // Default: pass through unchanged
    out_params[0] = in_params[0];
    out_params[1] = in_params[1];
    
    // Algorithm-specific parameter scaling
    switch (shape) {
        case MacroOscillatorShape::VOSIM:
        case MacroOscillatorShape::VOWEL:
        case MacroOscillatorShape::VOWEL_FOF:
            // Formant frequencies need scaling
            out_params[0] = in_params[0] >> 1;
            out_params[1] = in_params[1] >> 1;
            break;
            
        case MacroOscillatorShape::TRIPLE_RING_MOD:
            // Ring mod frequencies centered around 0
            out_params[0] = (in_params[0] - 16384) >> 2;
            out_params[1] = (in_params[1] - 16384) >> 2;
            break;
            
        case MacroOscillatorShape::FM:
        case MacroOscillatorShape::FEEDBACK_FM:
        case MacroOscillatorShape::CHAOTIC_FEEDBACK_FM:
            // FM ratio quantization happens in the DSP
            break;
            
        case MacroOscillatorShape::TOY:
        case MacroOscillatorShape::DIGITAL_MODULATION:
            // Sample rate and bit depth reduction
            out_params[0] = in_params[0] >> 11;  // SR reduction factor
            out_params[1] = in_params[1] >> 12;  // Bit reduction amount
            break;
            
        case MacroOscillatorShape::KICK:
            // Kick needs inverted decay parameter
            out_params[0] = in_params[0];  // Punch amount
            out_params[1] = 32767 - (in_params[1] >> 1);  // Decay (inverted)
            break;
            
        case MacroOscillatorShape::PLUCKED:
            // Damping needs inversion
            out_params[0] = 32767 - (in_params[0] >> 1);  // Damping
            out_params[1] = in_params[1];  // Brightness
            break;
            
        default:
            // Use default pass-through
            break;
    }
}

} // namespace braidy