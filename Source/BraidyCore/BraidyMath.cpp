#include "BraidyMath.h"

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

uint32_t ComputePhaseIncrement(int16_t pitch) {
    // Convert pitch to phase increment
    // pitch is in semitones * 128, where 60*128 = C4
    
    // Base frequency calculation: f = 440 * 2^((pitch/128 - 69)/12)
    // Phase increment = (f * 2^32) / sample_rate
    
    int32_t pitch_offset = pitch - (69 << 7);  // A4 = 69 semitones
    
    // Approximate 2^(x/12) using lookup table and interpolation
    // For now, use simple linear approximation
    if (pitch_offset < 0) {
        return 1;  // Very low frequency
    }
    
    // Simple approximation - will need proper exponential calculation
    uint32_t base_increment = 0x1000000;  // ~440Hz at 48kHz
    int32_t octaves = pitch_offset >> (7 + 2);  // Divide by 12*128
    
    return base_increment << (octaves > 0 ? octaves : 0);
}

float PitchToFrequency(int16_t pitch) {
    // Convert pitch to frequency in Hz
    // pitch is in semitones * 128
    float semitones = static_cast<float>(pitch) / 128.0f;
    return 440.0f * std::pow(2.0f, (semitones - 69.0f) / 12.0f);
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
    // Approximate 2^(x/65536)
    // Simple implementation - would benefit from lookup table
    return static_cast<uint16_t>(65536.0f * std::pow(2.0f, static_cast<float>(x) / 65536.0f));
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