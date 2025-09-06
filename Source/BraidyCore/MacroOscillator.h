#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"
#include "BraidySettings.h"
#include "AnalogOscillator.h"
#include "ParameterInterpolation.h"
#include <cstdio>

namespace braidy {

// Forward declaration for DigitalOscillator (will be implemented later)
class DigitalOscillator;

class MacroOscillator {
public:
    using RenderFn = void (MacroOscillator::*)(const uint8_t*, int16_t*, size_t);
    
    MacroOscillator();
    ~MacroOscillator();
    
    void Init();
    
    // Main parameter setters
    inline void set_shape(MacroOscillatorShape shape) {
        if (shape != shape_) {
            printf("=== MACRO OSCILLATOR SET_SHAPE DEBUG ===\n");
            printf("Shape changing from %d to %d\n", static_cast<int>(shape_), static_cast<int>(shape));
            Strike();  // Trigger any percussion/strike elements
        }
        shape_ = shape;
        printf("MacroOscillator shape set to: %d\n", static_cast<int>(shape_));
    }
    
    inline void set_pitch(int16_t pitch) { pitch_ = pitch; }
    inline void set_parameters(int16_t parameter_1, int16_t parameter_2) {
        parameter_[0] = parameter_1;
        parameter_[1] = parameter_2;
    }
    
    // Accessors
    inline MacroOscillatorShape shape() const { return shape_; }
    inline int16_t pitch() const { return pitch_; }
    inline int16_t parameter(int index) const { 
        return (index >= 0 && index < 2) ? parameter_[index] : 0; 
    }
    
    // Trigger percussion/strike elements
    void Strike();
    
    // Main rendering function
    void Render(const uint8_t* sync_buffer, int16_t* buffer, size_t size);
    
private:
    // Rendering functions for each synthesis model
    void RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderMorph(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSawSquare(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSineTriangle(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Sub-oscillator variants
    void RenderSub(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderDualSync(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderTriple(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Digital synthesis (placeholder for now)
    void RenderDigital(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Utility functions
    void ConfigureTriple(AnalogOscillatorShape shape);
    void UpdateParameters();
    
    // State
    MacroOscillatorShape shape_;
    int16_t pitch_;
    int16_t parameter_[2];
    int16_t previous_parameter_[2];
    
    // Oscillators
    AnalogOscillator analog_oscillator_[3];  // Up to 3 analog oscillators for complex algorithms
    DigitalOscillator* digital_oscillator_;   // Digital synthesis engine
    
    // Parameter interpolation for smooth changes
    MacroParameterInterpolation parameter_interpolation_;
    
    // Temporary buffers
    uint8_t sync_buffer_[kBlockSize];
    int16_t temp_buffer_[kBlockSize];
    
    // Low-pass filter state for some algorithms
    int32_t lp_state_;
    
    // Function table for algorithm dispatch
    static const RenderFn fn_table_[];
    
    DISALLOW_COPY_AND_ASSIGN(MacroOscillator);
};

}  // namespace braidy