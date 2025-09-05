#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"
#include "BraidyResources.h"

namespace braidy {

// Analog-style oscillator shapes
enum class AnalogOscillatorShape {
    SAW = 0,
    VARIABLE_SAW,
    CSAW,
    SQUARE,
    TRIANGLE,
    SINE,
    TRIANGLE_FOLD,
    SINE_FOLD,
    BUZZ
};

enum class SyncMode {
    OFF = 0,
    MASTER,
    SLAVE
};

class AnalogOscillator {
public:
    using RenderFn = void (AnalogOscillator::*)(const uint8_t*, int16_t*, uint8_t*, size_t);
    
    AnalogOscillator();
    ~AnalogOscillator() = default;
    
    void Init();
    
    // Parameter setters
    inline void set_shape(AnalogOscillatorShape shape) { shape_ = shape; }
    inline void set_parameter(int16_t parameter) { parameter_ = parameter; }
    inline void set_aux_parameter(int16_t aux_parameter) { aux_parameter_ = aux_parameter; }
    inline void set_pitch(int16_t pitch) { 
        pitch_ = pitch;
        ComputePhaseIncrement();
    }
    
    // Sync configuration
    inline void set_sync_mode(SyncMode sync_mode) { sync_mode_ = sync_mode; }
    
    // Main render function
    void Render(const uint8_t* sync_buffer, int16_t* buffer, uint8_t* aux, size_t size);
    
    // Get current phase for sync
    inline uint32_t phase() const { return phase_; }
    inline bool sync_edge_detected() const { return sync_edge_detected_; }
    
private:
    // Rendering functions for different shapes
    void RenderSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderVariableSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderCSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderSquare(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderTriangle(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderSine(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderTriangleFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderSineFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    void RenderBuzz(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size);
    
    // Utility functions
    void ComputePhaseIncrement();
    int16_t ComputeSample();
    void ProcessSync(const uint8_t* sync_buffer, size_t size);
    
    // State variables
    AnalogOscillatorShape shape_;
    SyncMode sync_mode_;
    
    uint32_t phase_;
    uint32_t phase_increment_;
    
    int16_t pitch_;
    int16_t parameter_;
    int16_t previous_parameter_;
    int16_t aux_parameter_;
    
    bool high_;
    int16_t next_sample_;
    int16_t discontinuity_depth_;
    bool sync_edge_detected_;
    
    // Function table for shape rendering
    static const RenderFn fn_table_[];
    
    DISALLOW_COPY_AND_ASSIGN(AnalogOscillator);
};

}  // namespace braidy