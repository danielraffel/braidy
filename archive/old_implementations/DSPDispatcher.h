// DSP Dispatcher - Unified algorithm routing to eliminate duplicate code
// This centralizes all DSP algorithm dispatching to avoid redundancy

#ifndef BRAIDY_DSP_DISPATCHER_H_
#define BRAIDY_DSP_DISPATCHER_H_

#include "BraidyTypes.h"
#include "BraidsDigitalDSP.h"
#include "WavetableManager.h"
#include <memory>

namespace braidy {

class DSPDispatcher {
public:
    DSPDispatcher();
    ~DSPDispatcher() = default;
    
    // Main dispatch function - routes to appropriate DSP algorithm
    void Process(MacroOscillatorShape shape, 
                 int16_t pitch,
                 int16_t* parameter,
                 uint32_t& phase,
                 uint32_t phase_increment,
                 const uint8_t* sync,
                 int16_t* buffer,
                 size_t size);
    
    // Strike trigger for percussive sounds
    void Strike();
    
    // Get specific DSP engine for direct access if needed
    BraidsDigitalDSP& GetDigitalDSP() { return digital_dsp_; }
    WavetableManager& GetWavetableManager() { return wavetable_manager_; }
    
private:
    // DSP engines
    BraidsDigitalDSP digital_dsp_;
    WavetableManager wavetable_manager_;
    
    // Shared state for algorithms that need it
    struct SharedState {
        uint32_t secondary_phase;
        int32_t filter_state[4];
        int16_t delay_line[4096];
        uint16_t delay_write_ptr;
        uint32_t noise_lfsr;
        int32_t envelope;
    } state_;
    
    // Helper function to determine if shape needs digital processing
    bool IsDigitalShape(MacroOscillatorShape shape) const;
    
    // Unified parameter scaling
    void ScaleParameters(MacroOscillatorShape shape, int16_t* in_params, int16_t* out_params);
    
    // Common DSP utilities
    int16_t SoftClip(int32_t x);
    uint32_t GetNoise();
    
    DISALLOW_COPY_AND_ASSIGN(DSPDispatcher);
};

// Algorithm classification helpers
inline bool DSPDispatcher::IsDigitalShape(MacroOscillatorShape shape) const {
    return shape >= MacroOscillatorShape::TRIPLE_RING_MOD;
}

inline int16_t DSPDispatcher::SoftClip(int32_t x) {
    if (x < -32768) return -32768;
    if (x > 32767) return 32767;
    return static_cast<int16_t>(x);
}

inline uint32_t DSPDispatcher::GetNoise() {
    state_.noise_lfsr = state_.noise_lfsr * 1664525 + 1013904223;
    return state_.noise_lfsr;
}

} // namespace braidy

#endif // BRAIDY_DSP_DISPATCHER_H_