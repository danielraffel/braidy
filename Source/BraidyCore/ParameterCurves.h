// Parameter Calibration Curves
// Matches original Braids hardware response curves

#pragma once

#include <cmath>
#include <cstdint>

namespace braidy {

class ParameterCurves {
public:
    // Exponential pitch tracking curve
    // Maps linear MIDI note (0-127) to exponential frequency multiplier
    static float PitchCurve(int16_t pitch) {
        // Convert from 16-bit pitch to MIDI note (0-127 range)
        float midi_note = 60.0f + (pitch / 512.0f);  // Center at C4
        
        // A4 = 440Hz, using equal temperament
        // f = 440 * 2^((n-69)/12) where n is MIDI note number
        return 440.0f * std::pow(2.0f, (midi_note - 69.0f) / 12.0f);
    }
    
    // S-curve for smooth modulation response
    // Used for TIMBRE and COLOR parameters
    static int16_t SCurve(int16_t input) {
        // Normalize to -1 to 1
        float x = input / 32768.0f;
        
        // Apply S-curve: f(x) = x / (1 + |x|^2)^0.5
        // This provides smooth saturation at extremes
        float abs_x = std::abs(x);
        float result = x / std::sqrt(1.0f + abs_x * abs_x);
        
        // Scale back to 16-bit
        return static_cast<int16_t>(result * 32767.0f);
    }
    
    // Exponential curve for filter cutoff
    // Maps linear control to exponential frequency response
    static int16_t FilterCurve(int16_t input) {
        // Map 0-65535 to 20Hz-20kHz exponentially
        float normalized = (input + 32768) / 65536.0f;  // 0 to 1
        
        // Exponential mapping: 20 * 1000^normalized
        // This gives us 20Hz at 0 and 20kHz at 1
        float freq = 20.0f * std::pow(1000.0f, normalized);
        
        // Convert frequency back to control value
        // Assuming sample rate of 48kHz, normalize to Nyquist
        float nyquist = 24000.0f;
        float ratio = freq / nyquist;
        
        // Clamp and scale
        ratio = std::min(1.0f, std::max(0.0f, ratio));
        return static_cast<int16_t>((ratio - 0.5f) * 65535.0f);
    }
    
    // Envelope curve for natural attack/decay
    static int16_t EnvelopeCurve(int16_t input, bool attack) {
        float normalized = (input + 32768) / 65536.0f;
        
        if (attack) {
            // Fast exponential attack
            float curve = 1.0f - std::exp(-5.0f * normalized);
            return static_cast<int16_t>((curve - 0.5f) * 65535.0f);
        } else {
            // Slower exponential decay
            float curve = std::exp(-3.0f * (1.0f - normalized));
            return static_cast<int16_t>((curve - 0.5f) * 65535.0f);
        }
    }
    
    // LFO curve for smoother modulation
    static int16_t LFOCurve(int16_t input) {
        // Apply slight S-curve to make LFO smoother at extremes
        float x = input / 32768.0f;
        
        // Softer S-curve for LFO
        float result = x * (1.5f - 0.5f * x * x);
        
        // Clamp to valid range
        result = std::min(1.0f, std::max(-1.0f, result));
        
        return static_cast<int16_t>(result * 32767.0f);
    }
    
    // Detune curve for oscillator spreading
    static int32_t DetuneCurve(int16_t spread, int voice_index, int num_voices) {
        // Calculate voice position (-1 to 1)
        float position = 0.0f;
        if (num_voices > 1) {
            position = (2.0f * voice_index / (num_voices - 1)) - 1.0f;
        }
        
        // Apply exponential spreading
        float spread_normalized = spread / 32768.0f;
        float detune_cents = position * spread_normalized * 50.0f;  // ±50 cents max
        
        // Convert cents to frequency ratio
        float ratio = std::pow(2.0f, detune_cents / 1200.0f);
        
        // Convert to phase increment multiplier (fixed point)
        return static_cast<int32_t>(ratio * 65536.0f);
    }
    
    // Resonance curve for filter Q
    static int16_t ResonanceCurve(int16_t input) {
        // Map linear control to exponential Q response
        float normalized = (input + 32768) / 65536.0f;
        
        // Q ranges from 0.5 to 20 exponentially
        float q = 0.5f * std::pow(40.0f, normalized);
        
        // Convert Q to feedback amount (approximation)
        float feedback = 1.0f - (1.0f / q);
        feedback = std::min(0.99f, std::max(0.0f, feedback));
        
        return static_cast<int16_t>((feedback - 0.5f) * 65535.0f);
    }
    
    // PWM curve for pulse width modulation
    static uint16_t PWMCurve(int16_t input) {
        // Map to 5%-95% duty cycle to avoid silence
        float normalized = (input + 32768) / 65536.0f;
        float pwm = 0.05f + 0.9f * normalized;
        
        return static_cast<uint16_t>(pwm * 65535.0f);
    }
};

} // namespace braidy