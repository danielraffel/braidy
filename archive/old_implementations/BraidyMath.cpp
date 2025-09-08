#include "BraidyMath.h"
#include "BraidsLookupTables.h"
#include "BraidsConstants.h"

namespace braidy {

Random random_generator;

// Sine lookup table (quarter wave, 256 entries)
static const int16_t kSineTable[257] = {
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

int16_t Sin(uint32_t phase) {
    return kSineTable[(phase >> 24)];
}

int16_t Cos(uint32_t phase) {
    return Sin(phase + (1UL << 30));  // Phase shift by 90 degrees
}

uint32_t ComputePhaseIncrement(int16_t midi_pitch) {
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
    
    // CRITICAL FIX: Original Braids expects one less octave shift
    // The LUT is calibrated for higher octaves, we were over-shifting
    if (num_shifts > 0) {
        phase_increment >>= (num_shifts - 1);  // One less shift = 2x frequency
    }
    
    return phase_increment;
}

float PitchToFrequency(int16_t pitch) {
    // Convert pitch to frequency in Hz using phase increment approach
    // This avoids runtime pow() calculations
    uint32_t phase_increment = ComputePhaseIncrement(pitch);
    
    // Convert phase increment back to frequency
    // phase_increment = (2^32 * frequency) / sample_rate
    // So: frequency = (phase_increment * sample_rate) / 2^32
    uint64_t frequency_64 = (static_cast<uint64_t>(phase_increment) * kSampleRate) >> 32;
    return static_cast<float>(frequency_64);
}

// PolyBLEP antialiasing
int32_t ThisBlepSample(uint32_t t) {
    // Simplified BLEP - proper implementation would use precomputed table
    int32_t t_normalized = static_cast<int32_t>(t) >> 16;  // 16-bit fraction
    if (t_normalized < 32768) {
        int32_t t_sq = (t_normalized * t_normalized) >> 16;
        return ((t_sq - t_normalized) >> 1);
    } else {
        int32_t t_flip = 65536 - t_normalized;
        int32_t t_sq = (t_flip * t_flip) >> 16;
        return -((t_sq - t_flip) >> 1);
    }
}

int32_t NextBlepSample(uint32_t t) {
    return ThisBlepSample(t + (1UL << 31));
}

uint32_t UnWrap(uint32_t phase) {
    // Phase unwrapping utility
    return phase & 0xFFFFFFFF;
}

int16_t Interpolate824(const int16_t* table, uint32_t phase) {
    // 8-bit table with 24-bit phase
    uint32_t index = phase >> 24;
    uint32_t frac = (phase >> 8) & 0xFFFF;
    
    int16_t a = table[index];
    int16_t b = table[(index + 1) & 0xFF];
    
    return a + ((b - a) * static_cast<int32_t>(frac) >> 16);
}

int16_t Interpolate1022(const int16_t* table, uint32_t phase) {
    // 10-bit table with 22-bit phase
    uint32_t index = phase >> 22;
    uint32_t frac = (phase >> 6) & 0xFFFF;
    
    int16_t a = table[index];
    int16_t b = table[(index + 1) & 0x3FF];
    
    return a + ((b - a) * static_cast<int32_t>(frac) >> 16);
}

uint16_t Pow2(uint16_t x) {
    // Approximate 2^(x/65536) using fixed-point math
    // This avoids runtime pow() calculations
    
    // For x in range 0-65535, representing 0.0 to 0.999...
    // We need to compute 2^(x/65536)
    
    // Split into integer and fractional parts
    uint32_t int_part = x >> 12;  // Top 4 bits (0-15)
    uint32_t frac_part = x & 0x0FFF;  // Bottom 12 bits
    
    // Base value for integer part: 2^int_part
    uint32_t base = 1U << int_part;  // This is 2^int_part
    
    // For fractional part, use linear approximation: 2^frac ≈ 1 + frac*ln(2)
    // ln(2) ≈ 0.693... ≈ 2839 in 12-bit fixed point
    uint32_t frac_mult = 2839 * frac_part;  // 12-bit * 12-bit = 24-bit
    uint32_t frac_add = (frac_mult >> 12) + (1 << 12);  // Back to 12-bit + 1.0
    
    // Combine: base * (1 + frac_correction)
    uint32_t result = (base * frac_add) >> 12;
    
    return static_cast<uint16_t>(std::min(result, 65535U));
}

uint16_t Exp2(uint16_t x) {
    return Pow2(x);
}

uint16_t Sqrt(uint32_t x) {
    // Fast integer square root
    if (x == 0) return 0;
    
    uint32_t result = 0;
    uint32_t bit = 1UL << 30;  // Start with highest bit
    
    while (bit > x) {
        bit >>= 2;
    }
    
    while (bit != 0) {
        if (x >= result + bit) {
            x -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    
    return static_cast<uint16_t>(result);
}

}  // namespace braidy