#include "AnalogOscillator.h"

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
    if (shape_index >= 0 && shape_index < 9) {
        (this->*fn_table_[shape_index])(sync_buffer, buffer, aux, size);
    } else {
        RenderSaw(sync_buffer, buffer, aux, size);  // Fallback
    }
}

void AnalogOscillator::RenderSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto saw_wave = GetSawtoothWave();
    
    for (size_t i = 0; i < size; ++i) {
        // Handle sync
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Generate sample using lookup table with interpolation
        buffer[i] = saw_wave.LookupInterpolated(phase_);
        
        if (aux) {
            aux[i] = (phase_ >> 24) > 127 ? 255 : 0;  // Square wave sync output
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderVariableSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto saw_wave = GetSawtoothWave();
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
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
        
        buffer[i] = saw_wave.LookupInterpolated(shaped_phase);
        
        if (aux) {
            aux[i] = (phase_ >> 24) > 127 ? 255 : 0;
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto saw_wave = GetSawtoothWave();
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Classic saw with parameter controlling brightness/harshness
        int16_t raw_sample = saw_wave.LookupInterpolated(phase_);
        
        // Apply parameter-controlled waveshaping
        if (parameter_ > 0) {
            int32_t shaped = raw_sample;
            shaped = (shaped * (32767 + parameter_)) >> 15;
            shaped = ClipS16(shaped);
            buffer[i] = static_cast<int16_t>(shaped);
        } else {
            buffer[i] = raw_sample;
        }
        
        if (aux) {
            aux[i] = (phase_ >> 24) > 127 ? 255 : 0;
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderSquare(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
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
        
        buffer[i] = sample;
        high_ = current_high;
        
        if (aux) {
            aux[i] = current_high ? 255 : 0;
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderTriangle(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto tri_wave = GetTriangleWave();
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Basic triangle wave
        buffer[i] = tri_wave.LookupInterpolated(phase_);
        
        if (aux) {
            aux[i] = (phase_ >> 24);  // Ramp output
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderSine(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto sine_wave = GetSineWave();
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Sine wave with parameter controlling harmonics/distortion
        int16_t sample = sine_wave.LookupInterpolated(phase_);
        
        if (parameter_ > 0) {
            // Add harmonic content based on parameter
            int16_t harmonic = sine_wave.LookupInterpolated(phase_ * 2);  // 2nd harmonic
            sample = Mix(sample, harmonic, parameter_);
        }
        
        buffer[i] = sample;
        
        if (aux) {
            aux[i] = static_cast<uint8_t>((sample >> 8) + 128);  // Bipolar to unipolar
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderTriangleFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto tri_wave = GetTriangleWave();
    auto folder = GetWaveshaper(2);  // Sine fold waveshaper
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Triangle with folding distortion
        int16_t raw = tri_wave.LookupInterpolated(phase_);
        
        if (parameter_ > 0) {
            // Scale up for folding
            int32_t scaled = (raw * (32768 + parameter_)) >> 15;
            scaled = ClipS16(scaled);
            
            // Apply wavefolder
            buffer[i] = folder.LookupInterpolated(static_cast<uint32_t>(scaled + 32768) << 15);
        } else {
            buffer[i] = raw;
        }
        
        if (aux) {
            aux[i] = (phase_ >> 24);
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderSineFold(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    auto sine_wave = GetSineWave();
    auto folder = GetWaveshaper(2);  // Sine fold waveshaper
    
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Sine with folding distortion
        int16_t raw = sine_wave.LookupInterpolated(phase_);
        
        if (parameter_ > 0) {
            // Scale and fold
            int32_t scaled = (raw * (32768 + parameter_)) >> 15;
            scaled = ClipS16(scaled);
            buffer[i] = folder.LookupInterpolated(static_cast<uint32_t>(scaled + 32768) << 15);
        } else {
            buffer[i] = raw;
        }
        
        if (aux) {
            aux[i] = static_cast<uint8_t>((raw >> 8) + 128);
        }
        
        phase_ += phase_increment_;
    }
}

void AnalogOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, uint8_t* aux, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (sync && sync[i]) {
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
        
        buffer[i] = ClipS16(sample);
        
        if (aux) {
            aux[i] = (phase_ >> 24);
        }
        
        phase_ += phase_increment_;
    }
}

}  // namespace braidy