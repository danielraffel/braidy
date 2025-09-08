#pragma once

#include "BraidyTypes.h"

namespace braidy {

// Resource management for Braidy synthesizer
// Simplified version of Mutable Instruments Braids resources
// Contains lookup tables and waveforms for synthesis

// Lookup table wrapper for safe access
template<typename T>
class LookupTable {
public:
    LookupTable(const T* data, size_t size) : data_(data), size_(size) {}
    
    inline T Lookup(size_t index) const {
        if (index >= size_) index = size_ - 1;
        return data_[index];
    }
    
    inline T LookupInterpolated(uint32_t phase) const {
        // Use top bits for index, lower bits for interpolation
        uint32_t index = phase >> (32 - 8);  // 8-bit index
        if (index >= (size_ << 8)) {
            index = (size_ << 8) - 1;
        }
        
        size_t integral = index >> 8;
        size_t fractional = index & 0xFF;
        
        T a = data_[integral];
        T b = (integral + 1 < size_) ? data_[integral + 1] : data_[integral];
        
        return a + (((b - a) * static_cast<int32_t>(fractional)) >> 8);
    }
    
    size_t size() const { return size_; }

private:
    const T* data_;
    size_t size_;
};

// Waveform data - essential tables
extern const int16_t wav_sine[257];
extern const int16_t* wav_triangle;
extern const int16_t* wav_sawtooth;
extern const int16_t* wav_square;
extern const int16_t* wav_buzz;

// Bandlimited waveforms for different frequency ranges
extern const int16_t* wav_bandlimited_saw[8];
extern const int16_t* wav_bandlimited_square[8];

// Lookup tables for parameter conversion
extern const uint32_t* lut_oscillator_increments;
extern const uint16_t* lut_env_expo;
extern const uint16_t* lut_svf_cutoff;
extern const uint16_t* lut_vco_detune;

// Waveshaping tables
extern const int16_t* ws_moderate_overdrive;
extern const int16_t* ws_violent_overdrive;
extern const int16_t* ws_sine_fold;

// Wavetables for wavetable synthesis
extern int16_t wt_waves[64][129];  // Non-const for runtime initialization

// Original Braids wavetable data
extern const int16_t braids_wavetable_data[16512];

// Character set for display (if UI needs it)
extern const uint8_t character_table[95][5];

// Formant synthesis data structures
struct PhonemeDefinition {
    uint8_t formant_frequency[3];
    uint8_t formant_amplitude[3];
};

// Phoneme data for vowel and consonant synthesis
extern const PhonemeDefinition vowels_data[9];
extern const PhonemeDefinition consonant_data[8];

// Formant frequency and amplitude tables for different voice types
static const int kNumFormants = 5;
extern const int16_t formant_f_data[kNumFormants][kNumFormants][kNumFormants];
extern const int16_t formant_a_data[kNumFormants][kNumFormants][kNumFormants];

// Formant waveform tables
extern const int16_t* wav_formant_sine;
extern const int16_t* wav_formant_square;

// Bell envelope for VOSIM synthesis
extern const uint16_t* lut_bell;

// Additive synthesis constants
static const int kNumAdditiveHarmonics = 14;

// Physical modeling constants
static const int kNumDrumPartials = 6;
extern const int16_t kDrumPartialAmplitude[kNumDrumPartials];

// Resource access functions
LookupTable<int16_t> GetSineWave();
LookupTable<int16_t> GetTriangleWave();
LookupTable<int16_t> GetSawtoothWave();
LookupTable<int16_t> GetSquareWave();

LookupTable<uint32_t> GetOscillatorIncrements();
LookupTable<uint16_t> GetEnvelopeExpo();
LookupTable<uint16_t> GetSvfCutoff();

LookupTable<int16_t> GetWaveshaper(int type);
LookupTable<int16_t> GetWavetable(int index);

// Utility functions for resource selection
const int16_t* GetBandlimitedWaveform(MacroOscillatorShape shape, int frequency_bin);
int GetFrequencyBin(uint32_t phase_increment);

// Initialize resources (called at startup)
void InitializeResources();

}  // namespace braidy