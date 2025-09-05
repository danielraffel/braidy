#include "BraidyResources.h"
#include "BraidyMath.h"
#include <cmath>

namespace braidy {

// Basic waveforms - generated procedurally for now
// In a full implementation, these would be precomputed tables

// Sine wave (257 samples for interpolation)
const int16_t wav_sine[257] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 
    10278, 11039, 11793, 12539, 13279, 14010, 14732, 15446, 16150, 16846, 17530, 
    18204, 18868, 19519, 20159, 20787, 21402, 22005, 22594, 23170, 23731, 24279, 
    24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 
    29621, 29956, 30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 
    32285, 32412, 32521, 32609, 32678, 32728, 32757, 32767, 32757, 32728, 32678, 
    32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 
    30571, 30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 
    26319, 25832, 25329, 24811, 24279, 23731, 23170, 22594, 22005, 21402, 20787, 
    20159, 19519, 18868, 18204, 17530, 16846, 16150, 15446, 14732, 14010, 13279, 
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 
    3212, 2410, 1608, 804, 0, -804, -1608, -2410, -3212, -4011, -4808, -5602, 
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793, -12539, -13279, 
    -14010, -14732, -15446, -16150, -16846, -17530, -18204, -18868, -19519, -20159, 
    -20787, -21402, -22005, -22594, -23170, -23731, -24279, -24811, -25329, -25832, 
    -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956, 
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, 
    -32412, -32521, -32609, -32678, -32728, -32757, -32767, -32757, -32728, -32678, 
    -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, 
    -30852, -30571, -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, 
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731, -23170, -22594, 
    -22005, -21402, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16150, 
    -15446, -14732, -14010, -13279, -12539, -11793, -11039, -10278, -9512, -8739, 
    -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804, 0
};

// Sawtooth wave
int16_t saw_temp[257];
const int16_t* wav_sawtooth = nullptr;

// Triangle wave  
int16_t tri_temp[257];
const int16_t* wav_triangle = nullptr;

// Square wave
int16_t sq_temp[257];
const int16_t* wav_square = nullptr;

// Buzz harmonics
int16_t buzz_temp[129];
const int16_t* wav_buzz = nullptr;

// Bandlimited waveforms placeholder
const int16_t* wav_bandlimited_saw[8] = {nullptr};
const int16_t* wav_bandlimited_square[8] = {nullptr};

// Oscillator increment lookup table (pitch to phase increment)
uint32_t osc_inc_temp[128];
const uint32_t* lut_oscillator_increments = nullptr;

// Envelope exponential curve
uint16_t env_temp[257];
const uint16_t* lut_env_expo = nullptr;

// Filter cutoff lookup
uint16_t svf_temp[128];  
const uint16_t* lut_svf_cutoff = nullptr;

// VCO detune amounts
uint16_t detune_temp[128];
const uint16_t* lut_vco_detune = nullptr;

// Waveshaping tables
int16_t ws_mod_temp[257];
const int16_t* ws_moderate_overdrive = nullptr;

int16_t ws_hard_temp[257];
const int16_t* ws_violent_overdrive = nullptr;

int16_t ws_fold_temp[513];
const int16_t* ws_sine_fold = nullptr;

// Wavetables - procedurally generated for 64 different wavetables
int16_t wt_temp[64][129];
const int16_t wt_waves[64][129] = {};

// Character table for display
const uint8_t character_table[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
    // ... would contain full character set
};

void InitializeResources() {
    // Generate sawtooth wave
    for (int i = 0; i < 257; ++i) {
        saw_temp[i] = static_cast<int16_t>((i * 65535 / 256) - 32767);
    }
    wav_sawtooth = saw_temp;
    
    // Generate triangle wave
    for (int i = 0; i < 257; ++i) {
        if (i < 128) {
            tri_temp[i] = static_cast<int16_t>((i * 65535 / 128) - 32767);
        } else {
            tri_temp[i] = static_cast<int16_t>(32767 - ((i - 128) * 65535 / 128));
        }
    }
    wav_triangle = tri_temp;
    
    // Generate square wave
    for (int i = 0; i < 257; ++i) {
        sq_temp[i] = (i < 128) ? 32767 : -32767;
    }
    wav_square = sq_temp;
    
    // Generate buzz (harmonics)
    for (int i = 0; i < 129; ++i) {
        float phase = static_cast<float>(i) * kPi / 64.0f;
        float value = 0.0f;
        for (int h = 1; h <= 8; ++h) {  // 8 harmonics
            value += std::sin(phase * h) / h;
        }
        buzz_temp[i] = static_cast<int16_t>(value * 16384.0f);
    }
    wav_buzz = buzz_temp;
    
    // Generate oscillator increments (pitch to phase increment)
    for (int i = 0; i < 128; ++i) {
        int16_t pitch = static_cast<int16_t>((i - 64) * 128);  // ±64 semitones
        float freq = 440.0f * std::pow(2.0f, (pitch / 128.0f - 69.0f) / 12.0f);
        osc_inc_temp[i] = static_cast<uint32_t>((freq * 4294967296.0) / kSampleRate);
    }
    lut_oscillator_increments = osc_inc_temp;
    
    // Generate envelope exponential curve
    for (int i = 0; i < 257; ++i) {
        float x = static_cast<float>(i) / 256.0f;
        env_temp[i] = static_cast<uint16_t>(65535.0f * std::pow(x, 4.0f));  // Exponential
    }
    lut_env_expo = env_temp;
    
    // Generate SVF cutoff frequency table
    for (int i = 0; i < 128; ++i) {
        float freq = 20.0f * std::pow(1000.0f, static_cast<float>(i) / 127.0f);  // 20Hz to 20kHz
        float omega = 2.0f * kPi * freq / kSampleRate;
        svf_temp[i] = static_cast<uint16_t>(std::sin(omega) * 32767.0f);
    }
    lut_svf_cutoff = svf_temp;
    
    // Generate detune amounts
    for (int i = 0; i < 128; ++i) {
        float cents = (static_cast<float>(i) - 64.0f) * 50.0f / 64.0f;  // ±50 cents
        detune_temp[i] = static_cast<uint16_t>(std::pow(2.0f, cents / 1200.0f) * 32768.0f);
    }
    lut_vco_detune = detune_temp;
    
    // Generate waveshaping tables
    for (int i = 0; i < 257; ++i) {
        float x = static_cast<float>(i - 128) / 128.0f;
        
        // Moderate overdrive
        float mod = std::tanh(x * 2.0f) * 0.7f;
        ws_mod_temp[i] = static_cast<int16_t>(mod * 32767.0f);
        
        // Violent overdrive
        float hard = std::tanh(x * 8.0f) * 0.9f;
        ws_hard_temp[i] = static_cast<int16_t>(hard * 32767.0f);
    }
    ws_moderate_overdrive = ws_mod_temp;
    ws_violent_overdrive = ws_hard_temp;
    
    // Generate sine fold
    for (int i = 0; i < 513; ++i) {
        float x = static_cast<float>(i - 256) / 256.0f;
        float fold = std::sin(x * kPi);
        ws_fold_temp[i] = static_cast<int16_t>(fold * 32767.0f);
    }
    ws_sine_fold = ws_fold_temp;
    
    // Initialize bandlimited waveforms to basic waveforms for now
    for (int i = 0; i < 8; ++i) {
        wav_bandlimited_saw[i] = wav_sawtooth;
        wav_bandlimited_square[i] = wav_square;
    }
    
    // Generate 64 different wavetables
    for (int wt = 0; wt < 64; ++wt) {
        for (int i = 0; i < 129; ++i) {
            float phase = static_cast<float>(i) * k2Pi / 128.0f;
            float value = 0.0f;
            
            // Different wavetable types based on index
            switch (wt / 8) {
                case 0: {
                    // Basic waveforms with harmonics (0-7)
                    int harmonics = (wt % 8) + 1;
                    for (int h = 1; h <= harmonics; ++h) {
                        value += std::sin(phase * h) / h;
                    }
                    break;
                }
                case 1: {
                    // PWM variants (8-15)
                    float duty = 0.1f + (wt % 8) * 0.1f;  // 10-80% duty cycle
                    value = (std::fmod(phase, k2Pi) < (k2Pi * duty)) ? 1.0f : -1.0f;
                    // Anti-alias with sine blend
                    value = value * 0.7f + std::sin(phase) * 0.3f;
                    break;
                }
                case 2: {
                    // Filtered sawtooth variants (16-23)
                    float cutoff = 2.0f + (wt % 8);  // Different filter cutoffs
                    value = phase / kPi - 1.0f;  // Sawtooth
                    for (int h = 2; h <= 16; ++h) {
                        if (h <= cutoff) {
                            value += std::sin(phase * h) / (h * h);
                        }
                    }
                    break;
                }
                case 3: {
                    // FM-like wavetables (24-31)
                    float mod_index = (wt % 8) * 0.5f;
                    value = std::sin(phase + mod_index * std::sin(phase * 2.0f));
                    break;
                }
                case 4: {
                    // Resonant filter sweeps (32-39)
                    float q = 0.5f + (wt % 8) * 0.2f;
                    float freq_mod = 1.0f + (wt % 8) * 0.5f;
                    value = std::sin(phase) + q * std::sin(phase * freq_mod);
                    value = std::tanh(value);  // Clip
                    break;
                }
                case 5: {
                    // Harmonic series with gaps (40-47)
                    for (int h = 1; h <= 12; ++h) {
                        if ((h & (1 << (wt % 8))) != 0) {
                            value += std::sin(phase * h) / h;
                        }
                    }
                    break;
                }
                case 6: {
                    // Formant-like structures (48-55)
                    float f1 = 200.0f + (wt % 4) * 100.0f;
                    float f2 = 1000.0f + ((wt % 8) / 4) * 500.0f;
                    value = std::sin(phase * f1/220.0f) + 0.5f * std::sin(phase * f2/220.0f);
                    break;
                }
                case 7: {
                    // Noise-like and chaotic (56-63)
                    uint32_t chaos = static_cast<uint32_t>(i * 13 + wt * 17);
                    chaos = (chaos * 1664525L + 1013904223L) >> 16;  // Simple PRNG
                    float noise_factor = (wt % 8) * 0.05f;
                    value = std::sin(phase) + noise_factor * (static_cast<float>(chaos) / 32768.0f - 1.0f);
                    break;
                }
            }
            
            wt_temp[wt][i] = static_cast<int16_t>(std::tanh(value * 0.8f) * 32767.0f);
        }
    }
    
    // Copy the generated wavetables to the const array (const_cast for initialization)
    int16_t* wt_mutable = const_cast<int16_t*>(&wt_waves[0][0]);
    for (int wt = 0; wt < 64; ++wt) {
        for (int i = 0; i < 129; ++i) {
            wt_mutable[wt * 129 + i] = wt_temp[wt][i];
        }
    }
}

LookupTable<int16_t> GetSineWave() {
    return LookupTable<int16_t>(wav_sine, 257);
}

LookupTable<int16_t> GetTriangleWave() {
    return LookupTable<int16_t>(wav_triangle, 257);
}

LookupTable<int16_t> GetSawtoothWave() {
    return LookupTable<int16_t>(wav_sawtooth, 257);
}

LookupTable<int16_t> GetSquareWave() {
    return LookupTable<int16_t>(wav_square, 257);
}

LookupTable<uint32_t> GetOscillatorIncrements() {
    return LookupTable<uint32_t>(lut_oscillator_increments, 128);
}

LookupTable<uint16_t> GetEnvelopeExpo() {
    return LookupTable<uint16_t>(lut_env_expo, 257);
}

LookupTable<uint16_t> GetSvfCutoff() {
    return LookupTable<uint16_t>(lut_svf_cutoff, 128);
}

LookupTable<int16_t> GetWaveshaper(int type) {
    switch (type) {
        case 0: return LookupTable<int16_t>(ws_moderate_overdrive, 257);
        case 1: return LookupTable<int16_t>(ws_violent_overdrive, 257);
        case 2: return LookupTable<int16_t>(ws_sine_fold, 513);
        default: return LookupTable<int16_t>(ws_moderate_overdrive, 257);
    }
}

LookupTable<int16_t> GetWavetable(int index) {
    if (index >= 0 && index < 64) {
        return LookupTable<int16_t>(wt_waves[index], 129);
    }
    return LookupTable<int16_t>(wav_sine, 257);
}

const int16_t* GetBandlimitedWaveform(MacroOscillatorShape shape, int frequency_bin) {
    frequency_bin = Clip(frequency_bin, 0, 7);
    
    switch (shape) {
        case MacroOscillatorShape::CSAW:
        case MacroOscillatorShape::SAW_SQUARE:
            return wav_bandlimited_saw[frequency_bin];
        default:
            return wav_bandlimited_square[frequency_bin];
    }
}

int GetFrequencyBin(uint32_t phase_increment) {
    // Determine which bandlimited table to use based on frequency
    // Higher frequencies need more bandlimiting
    if (phase_increment < (1UL << 20)) return 0;      // Very low
    else if (phase_increment < (1UL << 21)) return 1;
    else if (phase_increment < (1UL << 22)) return 2;
    else if (phase_increment < (1UL << 23)) return 3;
    else if (phase_increment < (1UL << 24)) return 4;
    else if (phase_increment < (1UL << 25)) return 5;
    else if (phase_increment < (1UL << 26)) return 6;
    else return 7;  // Very high frequency
}

}  // namespace braidy