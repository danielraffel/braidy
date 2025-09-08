#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"
#include <cstdint>

namespace braidy {

// Wavetable constants
constexpr size_t kWavetableSize = 256;           // Each wavetable is 256 samples
constexpr size_t kNumWavetables = 129;           // Total number of wavetables available
constexpr size_t kWavetableIndexShift = 8;      // For converting table index to sample index

/**
 * WavetableManager handles all wavetable synthesis operations
 * Based on Mutable Instruments Braids wavetable implementation
 */
class WavetableManager {
public:
    WavetableManager();
    ~WavetableManager() = default;
    
    void Init();
    
    // Main wavetable synthesis functions
    // WTBL - Basic wavetable lookup with crossfading between adjacent tables
    int16_t RenderWavetable(uint32_t phase, uint16_t table_index);
    
    // WMAP - 2D wavetable mapping with X/Y morphing
    int16_t RenderWaveMap(uint32_t phase, uint16_t x_pos, uint16_t y_pos);
    
    // WLIN - Linear sweep through wavetables
    int16_t RenderWaveLine(uint32_t phase, uint16_t sweep_pos);
    
    // WPAR - Paraphonic wavetables (multiple tables played simultaneously)
    void RenderWaveParaphonic(uint32_t* phases, uint16_t spread, 
                             uint16_t table_base, int16_t* buffer, size_t size);
    
    // High-level render functions for different wavetable algorithms
    void RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size,
                         uint32_t& phase, uint32_t phase_increment,
                         uint16_t parameter_1, uint16_t parameter_2);
    
    void RenderWaveMap2D(const uint8_t* sync, int16_t* buffer, size_t size,
                        uint32_t& phase, uint32_t phase_increment,
                        uint16_t x_parameter, uint16_t y_parameter);
    
    void RenderWaveLineSweep(const uint8_t* sync, int16_t* buffer, size_t size,
                            uint32_t& phase, uint32_t phase_increment,
                            uint16_t sweep_parameter, uint16_t morph_parameter);
    
    void RenderWaveParaphonicMode(const uint8_t* sync, int16_t* buffer, size_t size,
                                 uint32_t* phases, uint32_t* phase_increments,
                                 uint16_t spread_parameter, uint16_t table_parameter,
                                 size_t num_voices);
    
    // Utility functions
    static uint16_t GetTableIndex(uint16_t parameter, uint16_t max_tables = kNumWavetables - 1);
    static void InterpolateLinear(const int16_t* table_a, const int16_t* table_b,
                                 int16_t* output, uint16_t blend, size_t size = kWavetableSize);
    
private:
    // Bilinear interpolation for smooth morphing
    int16_t BilinearInterpolate(const int16_t* table_tl, const int16_t* table_tr,
                               const int16_t* table_bl, const int16_t* table_br,
                               uint16_t x_frac, uint16_t y_frac, uint16_t sample_index);
    
    // Get wavetable pointer by index
    const int16_t* GetWavetable(uint16_t table_index);
    
    // Convert wavetable data format (from uint8_t to int16_t if needed)
    void ConvertWavetableData();
    
    // Check if table index is valid
    bool IsValidTableIndex(uint16_t index) const {
        return index < kNumWavetables;
    }
    
    // Internal wavetable storage (converted from original format)
    int16_t wavetables_[kNumWavetables][kWavetableSize];
    bool tables_initialized_;
    
    // Temporary buffers for blending operations
    int16_t temp_buffer_a_[kWavetableSize];
    int16_t temp_buffer_b_[kWavetableSize];
    
    DISALLOW_COPY_AND_ASSIGN(WavetableManager);
};

} // namespace braidy