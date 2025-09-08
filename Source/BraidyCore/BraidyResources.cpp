#include "BraidyResources.h"
#include "BraidyMath.h"
#include "BraidsWavetableData.h"
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
int16_t wt_waves[64][129];  // Made non-const to allow runtime initialization

// Character table for display
const uint8_t character_table[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
    // ... would contain full character set
};

// Phoneme data for VOWEL synthesis (from original Braids)
const PhonemeDefinition vowels_data[9] = {
    { { 27,  40,  89 }, { 15,  13,  1 } },
    { { 18,  51,  62 }, { 13,  12,  6 } },
    { { 15,  69,  93 }, { 14,  12,  7 } },
    { { 10,  84, 110 }, { 13,  10,  8 } },
    { { 23,  44,  87 }, { 15,  12,  1 } },
    { { 13,  29,  80 }, { 13,   8,  0 } },
    { {  6,  46,  81 }, { 12,   3,  0 } },
    { {  9,  51,  95 }, { 15,   3,  0 } },
    { {  6,  73,  99 }, {  7,   3,  14 } }
};

const PhonemeDefinition consonant_data[8] = {
    { { 6, 54, 121 }, { 9,  9,  0 } },
    { { 18, 50, 51 }, { 12,  10,  5 } },
    { { 11, 24, 70 }, { 13,  8,  0 } },
    { { 15, 69, 74 }, { 14,  12,  7 } },
    { { 16, 37, 111 }, { 14,  8,  1 } },
    { { 18, 51, 62 }, { 14,  12,  6 } },
    { { 6, 26, 81 }, { 5,  5,  5 } },
    { { 6, 73, 99 }, { 7,  10,  14 } },
};

// Formant frequency and amplitude tables for FOF synthesis
const int16_t formant_f_data[kNumFormants][kNumFormants][kNumFormants] = {
    // bass
    {
        { 9519, 10738, 12448, 12636, 12892 }, // a
        { 8620, 11720, 12591, 12932, 13158 }, // e
        { 7579, 11891, 12768, 13122, 13323 }, // i
        { 8620, 10013, 12591, 12768, 13010 }, // o
        { 8324, 9519, 12591, 12831, 13048 }   // u
    },
    // tenor
    {
        { 9696, 10821, 12810, 13010, 13263 }, // a
        { 8620, 11827, 12768, 13228, 13477 }, // e
        { 7908, 12038, 12932, 13263, 13452 }, // i
        { 8620, 10156, 12768, 12932, 13085 }, // o
        { 8324, 9519, 12852, 13010, 13296 }   // u
    },
    // countertenor
    {
        { 9730, 10902, 12892, 13085, 13330 }, // a
        { 8832, 11953, 12852, 13085, 13296 }, // e
        { 7749, 12014, 13010, 13330, 13483 }, // i
        { 8781, 10211, 12852, 13085, 13296 }, // o
        { 8448, 9627, 12892, 13085, 13363 }   // u
    },
    // alto
    {
        { 10156, 10960, 12932, 13427, 14195 }, // a
        { 8620, 11692, 12852, 13296, 14195 },  // e
        { 8324, 11827, 12852, 13550, 14195 },  // i
        { 8881, 10156, 12956, 13427, 14195 },  // o
        { 8160, 9860, 12708, 13427, 14195 }    // u
    },
    // soprano
    {
        { 10156, 10960, 13010, 13667, 14195 }, // a
        { 8324, 12187, 12932, 13489, 14195 },  // e
        { 7749, 12337, 13048, 13667, 14195 },  // i
        { 8881, 10156, 12956, 13609, 14195 },  // o
        { 8160, 9860, 12852, 13609, 14195 }    // u
    }
};

const int16_t formant_a_data[kNumFormants][kNumFormants][kNumFormants] = {
    // bass
    {
        { 16384, 7318, 5813, 5813, 1638 }, // a
        { 16384, 4115, 5813, 4115, 2062 }, // e
        { 16384, 518, 2596, 1301, 652 },   // i
        { 16384, 4617, 1460, 1638, 163 },  // o
        { 16384, 1638, 411, 652, 259 }     // u
    },
    // tenor
    {
        { 16384, 8211, 7318, 6522, 1301 }, // a
        { 16384, 3269, 4115, 3269, 1638 }, // e
        { 16384, 2913, 2062, 1638, 518 },  // i
        { 16384, 5181, 4115, 4115, 821 },  // o
        { 16384, 1638, 2314, 3269, 821 }   // u
    },
    // countertenor
    {
        { 16384, 8211, 1159, 1033, 206 },  // a
        { 16384, 3269, 2062, 1638, 1638 }, // e
        { 16384, 1033, 1033, 259, 259 },   // i
        { 16384, 5181, 821, 1301, 326 },   // o
        { 16384, 1638, 518, 1160, 411 }    // u
    },
    // alto
    {
        { 16384, 4115, 2062, 1033, 411 },  // a
        { 16384, 2913, 2596, 2062, 1301 }, // e
        { 16384, 518, 1638, 1301, 652 },   // i
        { 16384, 2596, 1460, 1301, 518 },  // o
        { 16384, 821, 1033, 1638, 652 }    // u
    },
    // soprano
    {
        { 16384, 4115, 2596, 1301, 518 },  // a
        { 16384, 1160, 2596, 1638, 1160 }, // e
        { 16384, 326, 1301, 518, 259 },    // i
        { 16384, 2596, 1460, 1301, 652 },  // o
        { 16384, 821, 1301, 1301, 821 }    // u
    }
};

// Bell envelope for VOSIM synthesis
uint16_t bell_temp[256];
const uint16_t* lut_bell = nullptr;

// Formant waveform tables
int16_t formant_sine_temp[256];
const int16_t* wav_formant_sine = nullptr;

int16_t formant_square_temp[256];
const int16_t* wav_formant_square = nullptr;

// Physical modeling drum partials
const int16_t kDrumPartialAmplitude[kNumDrumPartials] = {
    16384, 8192, 4096, 2048, 1024, 512
};

void InitializeResources() {
    // Generate sawtooth wave - FIXED: proper range for int16_t
    // Sawtooth ramps from -32768 to +32767 over 256 samples
    for (int i = 0; i < 257; ++i) {
        // Map 0-256 to -32768 to +32767
        int32_t value = (i * 65536 / 256) - 32768;
        // Ensure we don't overflow int16_t
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        saw_temp[i] = static_cast<int16_t>(value);
    }
    wav_sawtooth = saw_temp;
    
    // Generate triangle wave - FIXED: proper range for int16_t
    for (int i = 0; i < 257; ++i) {
        int32_t value;
        if (i < 128) {
            // Rising edge: -32768 to +32767
            value = (i * 65536 / 128) - 32768;
        } else {
            // Falling edge: +32767 to -32768
            value = 32767 - ((i - 128) * 65536 / 128);
        }
        // Clamp to int16_t range
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        tri_temp[i] = static_cast<int16_t>(value);
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
    
    // Initialize wavetables using real Braids wavetable data
    // The Braids data contains 16512 samples = 128 wavetables * 129 samples each
    // We'll use the first 64 wavetables
    for (int wt = 0; wt < 64; ++wt) {
        for (int i = 0; i < 129; ++i) {
            // Direct copy from Braids wavetable data
            int source_index = wt * 129 + i;
            if (source_index < 16512) {
                wt_waves[wt][i] = braids_wavetable_data[source_index];
            } else {
                wt_waves[wt][i] = 0;  // Safety fallback
            }
        }
    }
    
    // Generate bell envelope for VOSIM synthesis
    for (int i = 0; i < 256; ++i) {
        float t = static_cast<float>(i) / 255.0f;
        // Bell-shaped envelope: e^(-5*t) * sin(pi*t)
        float bell = std::exp(-5.0f * t) * std::sin(kPi * t);
        bell_temp[i] = static_cast<uint16_t>(bell * 65535.0f);
    }
    lut_bell = bell_temp;
    
    // Generate formant waveform tables
    for (int i = 0; i < 256; ++i) {
        float phase = static_cast<float>(i) * k2Pi / 256.0f;
        
        // Formant sine with amplitude modulation capability
        float sine_val = std::sin(phase);
        formant_sine_temp[i] = static_cast<int16_t>(sine_val * 32767.0f);
        
        // Formant square for breathiness
        float square_val = (phase < kPi) ? 1.0f : -1.0f;
        formant_square_temp[i] = static_cast<int16_t>(square_val * 16384.0f);
    }
    wav_formant_sine = formant_sine_temp;
    wav_formant_square = formant_square_temp;
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