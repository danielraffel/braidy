#include "AnalogOscillator.h"
#include "BraidyMath.h"  // For Interpolate824
#include "BraidyResources.h"  // For wav_sine

namespace braidy {

// Function table for shape rendering
const AnalogOscillator::RenderFn AnalogOscillator::fn_table_[] = {
    &AnalogOscillator::RenderSaw,
    &AnalogOscillator::RenderVariableSaw,
    &AnalogOscillator::RenderCSaw,
    &AnalogOscillator::RenderSquare,
    &AnalogOscillator::RenderTriangle,
    &AnalogOscillator::RenderSine,
    &AnalogOscillator::RenderTriangleFold,
    &AnalogOscillator::RenderSineFold,
    &AnalogOscillator::RenderBuzz,
};

AnalogOscillator::AnalogOscillator() {
    Init();
}

void AnalogOscillator::Init() {
    shape_ = AnalogOscillatorShape::SAW;
    sync_mode_ = SyncMode::OFF;
    
    phase_ = 0;
    phase_increment_ = 1;
    
    pitch_ = kPitchC4;  // C4
    parameter_ = 0;
    previous_parameter_ = 0;
    aux_parameter_ = 0;
    
    high_ = false;
    next_sample_ = 0;
    discontinuity_depth_ = -16383;
    sync_edge_detected_ = false;
    
    ComputePhaseIncrement();
}

void AnalogOscillator::ComputePhaseIncrement() {
    phase_increment_ = ::braidy::ComputePhaseIncrement(pitch_);
}

void AnalogOscillator::Render(const uint8_t* sync_buffer, int16_t* buffer, uint8_t* aux, size_t size) {
    int shape_index = static_cast<int>(shape_);
    static int debug_render_count = 0;
    if (debug_render_count < 5) {
        printf("AnalogOscillator::Render DEBUG #%d: shape_=%d, shape_index=%d\n", 
               debug_render_count, (int)shape_, shape_index);
        debug_render_count++;
    }
    
    if (shape_index >= 0 && shape_index < 9) {
        (this->*fn_table_[shape_index])(sync_buffer, buffer, aux, size);
    } else {
        if (debug_render_count <= 5) {
            printf("  Fallback to RenderSaw (shape_index=%d out of range)\n", shape_index);
        }
        RenderSaw(sync_buffer, buffer, aux, size);  // Fallback
    }
}

void AnalogOscillator::RenderSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    // Sawtooth is generated directly from phase, not from a lookup table
    // This matches the original Braids approach
    
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Generate sawtooth directly from phase (inverted ramp)
        // phase_ ranges from 0 to 0xFFFFFFFF
        // We want output from -32768 to 32767
        *buffer = static_cast<int16_t>(~(phase_ >> 16));
        
        if (aux) {
            *aux = (phase_ >> 24) > 127 ? 255 : 0;  // Square wave sync output
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderVariableSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Variable saw uses parameter to control the shape
        uint32_t shaped_phase = phase_;
        if (parameter_ > 0) {
            // Compress first part of the wave
            uint32_t threshold = static_cast<uint32_t>(parameter_) << 16;
            if (shaped_phase < threshold) {
                shaped_phase = (shaped_phase * threshold) >> 16;
            } else {
                uint32_t remaining = 0xFFFFFFFF - threshold;
                shaped_phase = threshold + (((shaped_phase - threshold) * remaining) >> 16);
            }
        }
        
        // Generate sawtooth from shaped phase
        *buffer = static_cast<int16_t>(~(shaped_phase >> 16));
        
        if (aux) {
            *aux = (phase_ >> 24) > 127 ? 255 : 0;
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    // CSAW: Bandlimited sawtooth with phase randomization
    // parameter_ (TIMBRE) controls the phase randomization amount (0-32767)
    // aux_parameter_ (COLOR) controls filtering/brightness (not used for DC offset)
    
    // Debug output for first few samples
    static int debug_count = 0;
    if (debug_count < 10) {
        printf("CSAW DEBUG #%d: phase_increment_=%u, parameter_=%d, aux_parameter_=%d\n", 
               debug_count, phase_increment_, parameter_, aux_parameter_);
    }
    
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Apply phase randomization based on parameter_
        uint32_t working_phase = phase_;
        if (parameter_ > 0) {
            // Create pseudo-random phase jitter
            uint32_t jitter = (phase_ >> 8) ^ (phase_ >> 16); // Simple PRNG from phase
            jitter = (jitter * 1103515245U + 12345U) & 0xFFFF; // Linear congruential generator
            
            // Scale jitter by parameter strength
            int32_t jitter_amount = (jitter * parameter_) >> 16;
            working_phase += jitter_amount;
        }
        
        // Generate clean sawtooth directly from phase (no DC offset)
        int16_t raw_sample = static_cast<int16_t>(~(working_phase >> 16));
        
        *buffer = raw_sample;
        
        if (debug_count < 10) {
            printf("  Sample #%d: phase_=0x%08X, working_phase=0x%08X, raw_sample=%d\n",
                   debug_count, phase_, working_phase, raw_sample);
            debug_count++;
        }
        
        if (aux) {
            *aux = (phase_ >> 24) > 127 ? 255 : 0;
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderSquare(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Square wave with parameter controlling pulse width
        uint32_t threshold = (static_cast<uint32_t>(parameter_) + 32768) << 15;  // 0 to 0xFFFFFFFF
        
        bool current_high = phase_ < threshold;
        
        // Use PolyBLEP antialiasing at transitions
        int16_t sample = current_high ? 16383 : -16384;
        
        // Simple anti-aliasing: detect edge and apply BLEP
        if (current_high != high_) {
            uint32_t t = phase_ < threshold ? phase_ : (phase_ - threshold);
            int32_t blep = ThisBlepSample(t);
            sample += static_cast<int16_t>(blep >> 4);
        }
        
        *buffer = sample;
        high_ = current_high;
        
        if (aux) {
            *aux = current_high ? 255 : 0;
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderTriangle(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Generate triangle directly from phase
        // Triangle formula from original Braids
        uint16_t triangle = (phase_ >> 15) ^ (phase_ & 0x80000000 ? 0xffff : 0x0000);
        *buffer = static_cast<int16_t>(triangle);
        
        if (aux) {
            *aux = (phase_ >> 24);  // Ramp output
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderSine(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Sine wave using Interpolate824 like original Braids
        int16_t sample = Interpolate824(wav_sine, phase_);
        
        if (parameter_ > 0) {
            // Add harmonic content based on parameter
            int16_t harmonic = Interpolate824(wav_sine, phase_ * 2);  // 2nd harmonic
            sample = Mix(sample, harmonic, parameter_);
        }
        
        *buffer = sample;
        
        if (aux) {
            *aux = static_cast<uint8_t>((sample >> 8) + 128);  // Bipolar to unipolar
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderTriangleFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto folder = GetWaveshaper(2);  // Sine fold waveshaper
    
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Triangle with folding distortion
        uint16_t triangle = (phase_ >> 15) ^ (phase_ & 0x80000000 ? 0xffff : 0x0000);
        int16_t raw = static_cast<int16_t>(triangle);
        
        if (parameter_ > 0) {
            // Scale up for folding
            int32_t scaled = (raw * (32768 + parameter_)) >> 15;
            scaled = ClipS16(scaled);
            
            // Apply wavefolder
            *buffer = folder.LookupInterpolated(static_cast<uint32_t>(scaled + 32768) << 15);
        } else {
            *buffer = raw;
        }
        
        if (aux) {
            *aux = (phase_ >> 24);
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderSineFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto folder = GetWaveshaper(2);  // Sine fold waveshaper
    
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Sine with folding distortion
        int16_t raw = Interpolate824(wav_sine, phase_);
        
        if (parameter_ > 0) {
            // Scale and fold
            int32_t scaled = (raw * (32768 + parameter_)) >> 15;
            scaled = ClipS16(scaled);
            *buffer = folder.LookupInterpolated(static_cast<uint32_t>(scaled + 32768) << 15);
        } else {
            *buffer = raw;
        }
        
        if (aux) {
            *aux = static_cast<uint8_t>((raw >> 8) + 128);
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

void AnalogOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    while (size--) {
        // Handle sync - per-sample edge detection
        if (sync && *sync++) {
            phase_ = 0;
        }
        
        // Buzz harmonics - additive synthesis
        int32_t sample = 0;
        uint32_t harmonic_phase = phase_;
        
        int num_harmonics = 1 + (parameter_ >> 10);  // 1 to 32 harmonics based on parameter
        num_harmonics = Clip(num_harmonics, 1, 16);
        
        for (int h = 1; h <= num_harmonics; ++h) {
            int16_t harmonic = Sin(harmonic_phase);
            sample += harmonic / h;  // 1/n amplitude scaling
            harmonic_phase += phase_increment_;
        }
        
        *buffer = ClipS16(sample);
        
        if (aux) {
            *aux = (phase_ >> 24);
            aux++;
        }
        
        phase_ += phase_increment_;
        buffer++;
    }
}

}  // namespace braidy