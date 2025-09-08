// Chaotic Systems Implementation
// For WTFM (Chaotic Feedback FM) algorithm

#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

namespace braidy {

// Different chaos generators
enum class ChaosType {
    LOGISTIC_MAP,
    HENON_MAP,
    LORENZ_ATTRACTOR,
    ROSSLER_ATTRACTOR,
    CHUA_CIRCUIT
};

class ChaoticSystems {
public:
    ChaoticSystems() {
        Reset();
    }
    
    void Reset() {
        // Initialize chaos variables
        x_ = 0.1f;
        y_ = 0.1f;
        z_ = 0.1f;
        
        // Initialize FM state
        carrier_phase_ = 0;
        modulator_phase_ = 0;
        feedback_buffer_ = 0;
        
        // Initialize filter states
        lowpass_state_ = 0;
        highpass_state_ = 0;
    }
    
    // Main chaotic feedback FM synthesis
    void RenderChaoticFeedbackFM(int16_t pitch, int16_t* parameter,
                                uint32_t phase, uint32_t phase_increment,
                                const uint8_t* sync, int16_t* buffer, size_t size) {
        // Parameters:
        // parameter[0] (TIMBRE): Chaos amount / FM index
        // parameter[1] (COLOR): Feedback amount / attractor selection
        
        float chaos_amount = (parameter[0] + 32768) / 65536.0f;
        float feedback_amount = (parameter[1] + 32768) / 65536.0f;
        
        // Select chaos type based on COLOR
        ChaosType chaos_type = static_cast<ChaosType>((parameter[1] + 32768) / 13107);
        
        // FM parameters
        float fm_index = 0.1f + chaos_amount * 10.0f;  // 0.1 to 10
        float fm_ratio = 1.0f + feedback_amount * 7.0f;  // 1:1 to 1:8 ratio
        
        // Process each sample
        while (size--) {
            // Handle sync - per-sample edge detection (moved to top for correct timing)
            if (sync && *sync++) {
                carrier_phase_ = 0;
                modulator_phase_ = 0;
                // Don't reset chaos - let it continue for evolving textures
            }
            
            // Update chaos state
            UpdateChaos(chaos_type, chaos_amount);
            
            // Use chaos to modulate FM parameters
            float chaos_mod = GetChaosOutput();
            
            // Calculate modulator with feedback
            float modulator_freq = phase_increment * fm_ratio;
            modulator_phase_ += static_cast<uint32_t>(modulator_freq * (1.0f + chaos_mod));
            
            // Add feedback from previous output
            float feedback = feedback_buffer_ * feedback_amount;
            float modulator = std::sin(modulator_phase_ * 2.0f * M_PI / 4294967296.0f + feedback);
            
            // Apply chaos to modulation index
            float dynamic_index = fm_index * (1.0f + chaos_mod * 0.5f);
            
            // Calculate carrier with FM
            carrier_phase_ += phase_increment;
            float carrier = std::sin((carrier_phase_ + modulator * dynamic_index * 1073741824.0f) 
                                   * 2.0f * M_PI / 4294967296.0f);
            
            // Apply chaos-driven waveshaping
            carrier = ApplyChaoticWaveshaping(carrier, chaos_mod, chaos_amount);
            
            // Convert to 16-bit with soft limiting
            int32_t output = static_cast<int32_t>(carrier * 32767.0f);
            output = SoftLimit(output);
            
            // Store in feedback buffer
            feedback_buffer_ = output / 32768.0f;
            
            // Apply filtering for stability
            output = ApplyStabilityFilter(output, chaos_amount);
            
            *buffer++ = static_cast<int16_t>(output);
        }
    }
    
private:
    // Chaos state variables
    float x_, y_, z_;
    
    // FM state
    uint32_t carrier_phase_;
    uint32_t modulator_phase_;
    float feedback_buffer_;
    
    // Filter states
    float lowpass_state_;
    float highpass_state_;
    
    // Update chaos generators
    void UpdateChaos(ChaosType type, float amount) {
        const float dt = 0.001f;  // Time step
        
        switch (type) {
            case ChaosType::LOGISTIC_MAP: {
                // xn+1 = r * xn * (1 - xn)
                float r = 3.5f + amount * 0.5f;  // 3.5 to 4.0 (chaos region)
                x_ = r * x_ * (1.0f - x_);
                y_ = x_;  // Copy for 2D output
                break;
            }
            
            case ChaosType::HENON_MAP: {
                // Henon map equations
                float a = 1.2f + amount * 0.2f;
                float b = 0.3f;
                float x_new = 1.0f - a * x_ * x_ + y_;
                float y_new = b * x_;
                x_ = std::max(-2.0f, std::min(2.0f, x_new));
                y_ = std::max(-2.0f, std::min(2.0f, y_new));
                break;
            }
            
            case ChaosType::LORENZ_ATTRACTOR: {
                // Lorenz system
                float sigma = 10.0f;
                float rho = 28.0f;
                float beta = 8.0f / 3.0f;
                
                float dx = sigma * (y_ - x_);
                float dy = x_ * (rho - z_) - y_;
                float dz = x_ * y_ - beta * z_;
                
                x_ += dx * dt;
                y_ += dy * dt;
                z_ += dz * dt;
                
                // Keep bounded
                x_ = std::max(-50.0f, std::min(50.0f, x_));
                y_ = std::max(-50.0f, std::min(50.0f, y_));
                z_ = std::max(-50.0f, std::min(50.0f, z_));
                break;
            }
            
            case ChaosType::ROSSLER_ATTRACTOR: {
                // Rössler attractor
                float a = 0.2f;
                float b = 0.2f;
                float c = 5.7f + amount * 2.0f;
                
                float dx = -y_ - z_;
                float dy = x_ + a * y_;
                float dz = b + z_ * (x_ - c);
                
                x_ += dx * dt;
                y_ += dy * dt;
                z_ += dz * dt;
                
                // Keep bounded
                x_ = std::max(-20.0f, std::min(20.0f, x_));
                y_ = std::max(-20.0f, std::min(20.0f, y_));
                z_ = std::max(-50.0f, std::min(50.0f, z_));
                break;
            }
            
            case ChaosType::CHUA_CIRCUIT: {
                // Chua's circuit
                float alpha = 9.0f + amount * 3.0f;
                float beta = 14.286f;
                float a = -1.143f;
                float b = -0.714f;
                
                // Chua's diode (piecewise linear)
                float f_x = b * x_ + 0.5f * (a - b) * (std::abs(x_ + 1.0f) - std::abs(x_ - 1.0f));
                
                float dx = alpha * (y_ - x_ - f_x);
                float dy = x_ - y_ + z_;
                float dz = -beta * y_;
                
                x_ += dx * dt;
                y_ += dy * dt;
                z_ += dz * dt;
                
                // Keep bounded
                x_ = std::max(-3.0f, std::min(3.0f, x_));
                y_ = std::max(-3.0f, std::min(3.0f, y_));
                z_ = std::max(-5.0f, std::min(5.0f, z_));
                break;
            }
        }
    }
    
    // Get normalized chaos output
    float GetChaosOutput() {
        // Combine x and y for complex modulation
        // Normalize to roughly -1 to 1 range
        float output = (x_ + y_) * 0.1f;
        return std::max(-1.0f, std::min(1.0f, output));
    }
    
    // Apply chaotic waveshaping
    float ApplyChaoticWaveshaping(float input, float chaos, float amount) {
        // Use chaos to select between different waveshaping functions
        float shaped = input;
        
        // Folding
        if (chaos > 0.5f) {
            float fold_amount = (chaos - 0.5f) * 2.0f * amount;
            shaped = std::sin(input * (1.0f + fold_amount * 4.0f));
        }
        // Polynomial distortion
        else if (chaos > 0.0f) {
            float dist_amount = chaos * 2.0f * amount;
            shaped = input * (1.0f - dist_amount * input * input);
        }
        // Bitcrushing effect
        else {
            float crush_amount = -chaos * amount;
            int bits = 16 - static_cast<int>(crush_amount * 12);
            float scale = std::pow(2.0f, bits);
            shaped = std::round(input * scale) / scale;
        }
        
        return shaped;
    }
    
    // Soft limiting to prevent harsh clipping
    int32_t SoftLimit(int32_t sample) {
        const int32_t threshold = 28000;
        
        if (std::abs(sample) > threshold) {
            float x = static_cast<float>(sample) / 32768.0f;
            float sign = x > 0 ? 1.0f : -1.0f;
            x = std::abs(x);
            
            // Soft knee compression
            float ratio = 4.0f;
            float knee = 0.85f;
            
            if (x > knee) {
                x = knee + (x - knee) / ratio;
            }
            
            sample = static_cast<int32_t>(sign * x * 32767.0f);
        }
        
        return sample;
    }
    
    // Apply stability filter to prevent DC offset and extreme frequencies
    int32_t ApplyStabilityFilter(int32_t sample, float chaos_amount) {
        // High-pass filter to remove DC
        float hp_cutoff = 0.001f + chaos_amount * 0.01f;
        highpass_state_ += (sample - highpass_state_) * hp_cutoff;
        sample = sample - static_cast<int32_t>(highpass_state_);
        
        // Gentle low-pass to prevent aliasing
        float lp_cutoff = 0.5f - chaos_amount * 0.3f;
        lowpass_state_ += (sample - lowpass_state_) * lp_cutoff;
        
        return static_cast<int32_t>(lowpass_state_);
    }
};

} // namespace braidy