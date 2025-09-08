/*
  ==============================================================================

    BraidsConstants.h
    Created from Mutable Instruments Braids macro_oscillator.h and resources.h
    
    This file contains critical constants from the original Braids module,
    designed for 48kHz sample rate operation following the exact original 
    algorithm implementation.
    
    Copyright (c) 2012-2017 Emilie Gillet
    License: MIT License

  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace braidy {

//==============================================================================
// CORE BRAIDS CONSTANTS
// These match the original Braids implementation exactly
// Only constants not already defined in BraidyTypes.h
//==============================================================================

// Pitch table constants from original Braids
constexpr uint16_t kHighestNote = 128 * 128;      // 16384 - highest valid note  
constexpr uint16_t kPitchTableStart = 128 * 128;  // 16384 - start of LUT range

// Fixed-point precision
constexpr int32_t kFixedPointScale = 32768;

// Clipping helpers
inline int16_t ClipS16(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return static_cast<int16_t>(value);
}

inline uint16_t ClipU16(int32_t value) {
    if (value > 65535) return 65535;
    if (value < 0) return 0;
    return static_cast<uint16_t>(value);
}

} // namespace braidy