#pragma once

#include "BraidyTypes.h"
#include <algorithm>
#include <cmath>

namespace braidy {

// Mathematical utilities adapted from Mutable Instruments stmlib

// Clipping and saturation
template<typename T>
inline T Clip(T value, T min_value, T max_value) {
    return std::clamp(value, min_value, max_value);
}

template<typename T>
inline T ClipU8(T value) {
    return Clip<T>(value, 0, 255);
}

template<typename T>
inline T ClipS16(T value) {
    return Clip<T>(value, -32768, 32767);
}

template<typename T>
inline T ClipU16(T value) {
    return Clip<T>(value, 0, 65535);
}

// Soft clipping/saturation
inline int16_t SoftClip(int32_t x) {
    if (x < -32768) {
        return -32768 + ((x + 32768) >> 4);
    } else if (x > 32767) {
        return 32767 - ((x - 32767) >> 4);
    } else {
        return static_cast<int16_t>(x);
    }
}

// Linear interpolation
template<typename T, typename U>
inline T Crossfade(T a, T b, U fade) {
    return a + ((b - a) * fade >> 16);
}

// Mix two values
template<typename T, typename U>
inline T Mix(T a, T b, U balance) {
    return (a * (65535 - balance) + b * balance) >> 16;
}

// Random number generation
class Random {
public:
    Random() : rng_state_(1) {}
    
    void Seed(uint32_t seed) {
        rng_state_ = seed;
    }
    
    uint32_t GetWord() {
        rng_state_ = rng_state_ * 1664525L + 1013904223L;
        return rng_state_;
    }
    
    int32_t GetSigned() {
        return static_cast<int32_t>(GetWord());
    }
    
    uint16_t GetU16() {
        return static_cast<uint16_t>(GetWord() >> 16);
    }
    
    int16_t GetS16() {
        return static_cast<int16_t>(GetWord() >> 16);
    }

private:
    uint32_t rng_state_;
};

extern Random random_generator;

// Lookup tables and interpolation
class LookupTableI16 {
public:
    LookupTableI16(const int16_t* data, size_t size) : data_(data), size_(size) {}
    
    inline int16_t Lookup(uint32_t index) const {
        index >>= (32 - 8);  // Use top 8 bits for indexing
        if (index >= (size_ << 8)) {
            index = (size_ << 8) - 1;
        }
        
        size_t integral = index >> 8;
        size_t fractional = index & 0xFF;
        
        int16_t a = data_[integral];
        int16_t b = (integral + 1 < size_) ? data_[integral + 1] : data_[integral];
        
        return a + ((b - a) * fractional >> 8);
    }

private:
    const int16_t* data_;
    size_t size_;
};

// Fixed point sine approximation 
int16_t Sin(uint32_t phase);
int16_t Cos(uint32_t phase);

// Phase increment calculation from pitch
uint32_t ComputePhaseIncrement(int16_t pitch);

// Pitch to frequency conversion
float PitchToFrequency(int16_t pitch);

// Envelope calculations
uint16_t Pow2(uint16_t x);
uint16_t Exp2(uint16_t x);

// DSP utilities
int32_t ThisBlepSample(uint32_t t);
int32_t NextBlepSample(uint32_t t);

// Oscillator utilities
uint32_t UnWrap(uint32_t phase);
int16_t Interpolate824(const int16_t* table, uint32_t phase);
int16_t Interpolate1022(const int16_t* table, uint32_t phase);

// Note/pitch utilities
inline int16_t NoteToFrequency(uint8_t note) {
    return static_cast<int16_t>((note << 7) + (kPitchC4 - (60 << 7)));
}

inline float Semitones(int16_t pitch_delta) {
    return static_cast<float>(pitch_delta) / 128.0f;
}

// Parameter scaling utilities
template<typename T>
inline T ScaleParameter(T value, T min_val, T max_val) {
    return min_val + ((max_val - min_val) * value >> 16);
}

// Fast square root approximation
uint16_t Sqrt(uint32_t x);

// One-pole filters for parameter smoothing
class OnePole {
public:
    OnePole() : y_(0) {}
    
    void set_f(float f) {
        g_ = f;
    }
    
    float Process(float x) {
        y_ += g_ * (x - y_);
        return y_;
    }
    
    float value() const { return y_; }
    
private:
    float g_ = 0.1f;  // Default cutoff
    float y_;
};

}  // namespace braidy