#include "WavetableManager.h"
#include "BraidsWavetableData.h"
#include "BraidyResources.h"
#include "BraidyMath.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace braidy {

WavetableManager::WavetableManager() : tables_initialized_(false) {
}

void WavetableManager::Init() {
    if (!tables_initialized_) {
        ConvertWavetableData();
        tables_initialized_ = true;
    }
}

void WavetableManager::ConvertWavetableData() {
    // Convert the Braids wavetable data format to our internal format
    // The original Braids data is 16512 int16_t values (129 tables * 128 samples each, but we need 256)
    // We'll need to interpolate or repeat samples to get 256 samples per table
    
    constexpr size_t original_table_size = 128;  // Original Braids table size
    constexpr size_t original_num_tables = 129;  // 16512 / 128 = 129 tables
    
    // Ensure we don't exceed our array bounds
    const size_t num_tables_to_convert = std::min(static_cast<size_t>(kNumWavetables), 
                                                  static_cast<size_t>(original_num_tables));
    
    for (size_t table = 0; table < num_tables_to_convert; ++table) {
        const int16_t* source = &braids_wavetable_data[table * original_table_size];
        int16_t* dest = wavetables_[table];
        
        // Upsample from 128 to 256 samples using linear interpolation
        for (size_t i = 0; i < kWavetableSize; ++i) {
            float source_pos = (float(i) * original_table_size) / kWavetableSize;
            size_t index = static_cast<size_t>(source_pos);
            float frac = source_pos - index;
            
            if (index >= original_table_size - 1) {
                dest[i] = source[original_table_size - 1];
            } else {
                // Linear interpolation between samples
                dest[i] = static_cast<int16_t>(
                    source[index] * (1.0f - frac) + source[index + 1] * frac
                );
            }
        }
    }
    
    // Fill any remaining tables with zeros or sine waves
    for (size_t table = num_tables_to_convert; table < kNumWavetables; ++table) {
        for (size_t i = 0; i < kWavetableSize; ++i) {
            // Generate a sine wave for empty tables
            wavetables_[table][i] = static_cast<int16_t>(
                32767.0f * sinf(k2Pi * i / kWavetableSize)
            );
        }
    }
}

const int16_t* WavetableManager::GetWavetable(uint16_t table_index) {
    if (!IsValidTableIndex(table_index)) {
        table_index = 0;  // Fallback to first table
    }
    return wavetables_[table_index];
}

uint16_t WavetableManager::GetTableIndex(uint16_t parameter, uint16_t max_tables) {
    // Convert 16-bit parameter to table index
    return static_cast<uint16_t>((static_cast<uint32_t>(parameter) * max_tables) >> 16);
}

int16_t WavetableManager::BilinearInterpolate(const int16_t* table_tl, const int16_t* table_tr,
                                             const int16_t* table_bl, const int16_t* table_br,
                                             uint16_t x_frac, uint16_t y_frac, uint16_t sample_index) {
    // Get samples from all four tables
    int32_t tl = table_tl[sample_index];  // Top-left
    int32_t tr = table_tr[sample_index];  // Top-right
    int32_t bl = table_bl[sample_index];  // Bottom-left
    int32_t br = table_br[sample_index];  // Bottom-right
    
    // Interpolate horizontally (top row)
    int32_t top = tl + (((tr - tl) * x_frac) >> 16);
    
    // Interpolate horizontally (bottom row)
    int32_t bottom = bl + (((br - bl) * x_frac) >> 16);
    
    // Interpolate vertically
    int32_t result = top + (((bottom - top) * y_frac) >> 16);
    
    return static_cast<int16_t>(result);
}

void WavetableManager::InterpolateLinear(const int16_t* table_a, const int16_t* table_b,
                                        int16_t* output, uint16_t blend, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        int32_t a = table_a[i];
        int32_t b = table_b[i];
        output[i] = static_cast<int16_t>(a + (((b - a) * blend) >> 16));
    }
}

int16_t WavetableManager::RenderWavetable(uint32_t phase, uint16_t table_index) {
    // Extract table and fractional parts
    uint16_t table_int = table_index >> 8;  // Integer part
    uint16_t table_frac = table_index & 0xFF;  // Fractional part
    
    // Get sample index from phase
    uint16_t sample_index = phase >> 24;  // Top 8 bits for sample index
    uint16_t sample_frac = (phase >> 16) & 0xFF;  // Next 8 bits for sample interpolation
    
    // Debug output for first few calls
    static int debug_count = 0;
    if (debug_count < 5) {
        printf("WavetableManager::RenderWavetable DEBUG #%d:\n", debug_count);
        printf("  Input: phase=0x%08X, table_index=%u\n", phase, table_index);
        printf("  Parsed: table_int=%u, table_frac=%u, sample_index=%u, sample_frac=%u\n", 
               table_int, table_frac, sample_index, sample_frac);
        printf("  tables_initialized_=%s\n", tables_initialized_ ? "true" : "false");
        debug_count++;
    }
    
    // Get adjacent tables
    const int16_t* table_a = GetWavetable(table_int);
    const int16_t* table_b = GetWavetable(table_int + 1);
    
    // Get adjacent samples from first table
    int16_t sample_a1 = table_a[sample_index];
    int16_t sample_a2 = table_a[(sample_index + 1) & 0xFF];
    
    // Get adjacent samples from second table
    int16_t sample_b1 = table_b[sample_index];
    int16_t sample_b2 = table_b[(sample_index + 1) & 0xFF];
    
    // Debug output for sample values
    if (debug_count <= 5) {
        printf("  Samples: a1=%d, a2=%d, b1=%d, b2=%d\n", sample_a1, sample_a2, sample_b1, sample_b2);
    }
    
    // Interpolate within samples
    int32_t interp_a = sample_a1 + (((sample_a2 - sample_a1) * sample_frac) >> 8);
    int32_t interp_b = sample_b1 + (((sample_b2 - sample_b1) * sample_frac) >> 8);
    
    // Interpolate between tables
    int32_t result = interp_a + (((interp_b - interp_a) * table_frac) >> 8);
    
    if (debug_count <= 5) {
        printf("  Result: %d\n", static_cast<int16_t>(result));
    }
    
    return static_cast<int16_t>(result);
}

int16_t WavetableManager::RenderWaveMap(uint32_t phase, uint16_t x_pos, uint16_t y_pos) {
    // Convert parameters to table coordinates
    uint16_t x_table = x_pos >> 12;  // 4 bits for X table index
    uint16_t y_table = y_pos >> 12;  // 4 bits for Y table index
    uint16_t x_frac = (x_pos >> 4) & 0xFF;  // 8 bits for X fraction
    uint16_t y_frac = (y_pos >> 4) & 0xFF;  // 8 bits for Y fraction
    
    // Clamp to valid range
    if (x_table >= 16) x_table = 15;
    if (y_table >= 8) y_table = 7;
    
    // Calculate table indices for 2D grid
    uint16_t table_tl = y_table * 16 + x_table;         // Top-left
    uint16_t table_tr = y_table * 16 + (x_table + 1);   // Top-right
    uint16_t table_bl = (y_table + 1) * 16 + x_table;   // Bottom-left
    uint16_t table_br = (y_table + 1) * 16 + (x_table + 1);  // Bottom-right
    
    // Get sample index
    uint16_t sample_index = phase >> 24;
    uint16_t sample_frac = (phase >> 16) & 0xFF;
    
    // Get tables
    const int16_t* tl_table = GetWavetable(table_tl);
    const int16_t* tr_table = GetWavetable(table_tr);
    const int16_t* bl_table = GetWavetable(table_bl);
    const int16_t* br_table = GetWavetable(table_br);
    
    // Get adjacent samples and interpolate within each table
    auto get_interpolated_sample = [](const int16_t* table, uint16_t idx, uint16_t frac) -> int16_t {
        int16_t s1 = table[idx];
        int16_t s2 = table[(idx + 1) & 0xFF];
        return static_cast<int16_t>(s1 + (((s2 - s1) * frac) >> 8));
    };
    
    int16_t tl_sample = get_interpolated_sample(tl_table, sample_index, sample_frac);
    int16_t tr_sample = get_interpolated_sample(tr_table, sample_index, sample_frac);
    int16_t bl_sample = get_interpolated_sample(bl_table, sample_index, sample_frac);
    int16_t br_sample = get_interpolated_sample(br_table, sample_index, sample_frac);
    
    // Bilinear interpolation
    int32_t top = tl_sample + (((tr_sample - tl_sample) * x_frac) >> 8);
    int32_t bottom = bl_sample + (((br_sample - bl_sample) * x_frac) >> 8);
    int32_t result = top + (((bottom - top) * y_frac) >> 8);
    
    return static_cast<int16_t>(result);
}

int16_t WavetableManager::RenderWaveLine(uint32_t phase, uint16_t sweep_pos) {
    // Linear sweep through all available wavetables
    uint16_t table_index = (static_cast<uint32_t>(sweep_pos) * (kNumWavetables - 1)) >> 16;
    uint16_t table_frac = ((static_cast<uint32_t>(sweep_pos) * (kNumWavetables - 1)) >> 8) & 0xFF;
    
    return RenderWavetable(phase, (table_index << 8) | table_frac);
}

void WavetableManager::RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size,
                                       uint32_t& phase, uint32_t phase_increment,
                                       uint16_t parameter_1, uint16_t parameter_2) {
    for (size_t i = 0; i < size; ++i) {
        // Handle sync
        if (sync && sync[i]) {
            phase = 0;
        }
        
        // Render sample using basic wavetable lookup
        uint16_t table_index = (static_cast<uint32_t>(parameter_1) * (kNumWavetables - 1)) >> 16;
        uint16_t morph_amount = parameter_2 >> 8;  // Use parameter_2 for table morphing
        
        buffer[i] = RenderWavetable(phase, (table_index << 8) | morph_amount);
        
        // Advance phase
        phase += phase_increment;
    }
}

void WavetableManager::RenderWaveMap2D(const uint8_t* sync, int16_t* buffer, size_t size,
                                      uint32_t& phase, uint32_t phase_increment,
                                      uint16_t x_parameter, uint16_t y_parameter) {
    for (size_t i = 0; i < size; ++i) {
        // Handle sync
        if (sync && sync[i]) {
            phase = 0;
        }
        
        buffer[i] = RenderWaveMap(phase, x_parameter, y_parameter);
        
        // Advance phase
        phase += phase_increment;
    }
}

void WavetableManager::RenderWaveLineSweep(const uint8_t* sync, int16_t* buffer, size_t size,
                                          uint32_t& phase, uint32_t phase_increment,
                                          uint16_t sweep_parameter, uint16_t morph_parameter) {
    for (size_t i = 0; i < size; ++i) {
        // Handle sync
        if (sync && sync[i]) {
            phase = 0;
        }
        
        // Use sweep_parameter for position along the line
        // morph_parameter can add variation or crossfading
        uint16_t modified_sweep = sweep_parameter + ((morph_parameter >> 4) & 0x0FFF);
        buffer[i] = RenderWaveLine(phase, modified_sweep);
        
        // Advance phase
        phase += phase_increment;
    }
}

void WavetableManager::RenderWaveParaphonicMode(const uint8_t* sync, int16_t* buffer, size_t size,
                                               uint32_t* phases, uint32_t* phase_increments,
                                               uint16_t spread_parameter, uint16_t table_parameter,
                                               size_t num_voices) {
    // Clear buffer first
    std::memset(buffer, 0, size * sizeof(int16_t));
    
    // Limit number of voices
    const size_t max_voices = std::min(num_voices, static_cast<size_t>(8));
    
    // Calculate table spread
    uint16_t table_spread = spread_parameter >> 8;
    uint16_t base_table = GetTableIndex(table_parameter);
    
    for (size_t voice = 0; voice < max_voices; ++voice) {
        uint16_t voice_table = base_table + (voice * table_spread / max_voices);
        if (voice_table >= kNumWavetables) {
            voice_table = kNumWavetables - 1;
        }
        
        for (size_t i = 0; i < size; ++i) {
            // Handle sync for this voice
            if (sync && sync[i]) {
                phases[voice] = 0;
            }
            
            // Render this voice
            int16_t sample = RenderWavetable(phases[voice], voice_table << 8);
            
            // Mix into buffer with volume scaling
            int32_t mixed = buffer[i] + (sample / static_cast<int32_t>(max_voices));
            buffer[i] = ClipS16(mixed);
            
            // Advance phase for this voice
            phases[voice] += phase_increments[voice];
        }
    }
}

void WavetableManager::RenderWaveParaphonic(uint32_t* phases, uint16_t spread, 
                                           uint16_t table_base, int16_t* buffer, size_t size) {
    // Clear buffer first
    std::memset(buffer, 0, size * sizeof(int16_t));
    
    const size_t num_voices = 4;
    
    // Calculate detune for each voice based on spread
    for (size_t voice = 0; voice < num_voices; ++voice) {
        // Calculate table for this voice
        uint16_t voice_table = table_base + (voice * spread >> 8);
        if (voice_table >= kNumWavetables) {
            voice_table = kNumWavetables - 1;
        }
        
        for (size_t i = 0; i < size; ++i) {
            // Render this voice
            int16_t sample = RenderWavetable(phases[voice], voice_table << 8);
            
            // Mix into buffer with volume scaling
            int32_t mixed = buffer[i] + (sample / static_cast<int32_t>(num_voices));
            buffer[i] = ClipS16(mixed);
        }
    }
}

} // namespace braidy