#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"

namespace braidy {

// Parameter definitions for Braidy synthesizer
enum class BraidyParameter {
    // Core synthesis parameters
    SHAPE = 0,              // Oscillator shape/algorithm
    PITCH,                  // Fundamental pitch (legacy - use COARSE + FINE instead)
    COARSE,                 // Coarse pitch control (±5 octaves)
    FINE,                   // Fine pitch control (±100 cents)
    TIMBRE,                 // Parameter 1 (varies by algorithm)
    COLOR,                  // Parameter 2 (varies by algorithm)
    
    // Modulation parameters
    FM_AMOUNT,              // FM modulation depth
    FM_RATIO,               // FM ratio for FM algorithms
    
    // Envelope parameters
    ATTACK,                 // Attack time
    DECAY,                  // Decay time
    
    // Global parameters
    VOLUME,                 // Output volume
    BRIGHTNESS,             // High frequency content
    
    // Advanced parameters
    META_MODULATION,        // Enable meta-modulation
    PITCH_OCTAVE,           // Pitch octave offset
    QUANTIZER_SCALE,        // Quantizer scale
    QUANTIZER_ROOT,         // Quantizer root note
    
    // Phase 8: Advanced features
    ENVELOPE_SUSTAIN,       // Envelope sustain level (ADSR extension)
    ENVELOPE_RELEASE,       // Envelope release time
    ENVELOPE_SHAPE,         // Envelope curve shape
    BIT_CRUSHER_BITS,       // Bit crusher resolution
    BIT_CRUSHER_RATE,       // Bit crusher sample rate
    WAVESHAPER_AMOUNT,      // Waveshaper/distortion amount
    WAVESHAPER_TYPE,        // Waveshaper algorithm type
    META_ENABLED,           // Enable meta oscillator mode
    META_SPEED,             // Meta oscillator change speed
    META_RANGE,             // Meta oscillator algorithm range
    PARAPHONY_ENABLED,      // Enable paraphonic mode
    PARAPHONY_DETUNE,       // Paraphonic detune amount
    VCA_MODE,               // VCA response (linear/exponential)
    LOW_PASS_GATE_ENABLED,  // Enable low-pass gate mode
    LOW_PASS_GATE_DECAY,    // Low-pass gate decay time
    
    PARAMETER_COUNT
};

// Parameter metadata
struct ParameterInfo {
    const char* name;
    const char* short_name;
    const char* description;
    float min_value;
    float max_value; 
    float default_value;
    bool is_discrete;
    const char* const* value_strings;  // For discrete parameters
};

class BraidySettings {
public:
    BraidySettings();
    ~BraidySettings() = default;
    
    // Initialize with default values
    void Init();
    
    // Parameter access
    float GetParameter(BraidyParameter param) const;
    void SetParameter(BraidyParameter param, float value);
    
    // Normalized parameter access (0.0 to 1.0)
    float GetParameterNormalized(BraidyParameter param) const;
    void SetParameterNormalized(BraidyParameter param, float normalized_value);
    
    // Parameter info
    static const ParameterInfo& GetParameterInfo(BraidyParameter param);
    static const char* GetParameterName(BraidyParameter param);
    static int GetParameterCount() { return static_cast<int>(BraidyParameter::PARAMETER_COUNT); }
    
    // Specific parameter getters (commonly used)
    MacroOscillatorShape GetShape() const {
        return static_cast<MacroOscillatorShape>(static_cast<int>(GetParameter(BraidyParameter::SHAPE)));
    }
    
    void SetShape(MacroOscillatorShape shape) {
        SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(shape)));
    }
    
    int16_t GetPitch() const {
        // Legacy method - combines coarse and fine pitch
        float coarse = GetParameter(BraidyParameter::COARSE);
        float fine = GetParameter(BraidyParameter::FINE);
        return static_cast<int16_t>((coarse * 12.0f + fine) * 128.0f);  // Convert to internal format
    }
    
    void SetPitch(int16_t pitch) {
        // Legacy method - distributes to coarse and fine
        float total_semitones = static_cast<float>(pitch) / 128.0f;
        float coarse = std::floor(total_semitones / 12.0f);
        float fine = total_semitones - (coarse * 12.0f);
        SetParameter(BraidyParameter::COARSE, coarse);
        SetParameter(BraidyParameter::FINE, fine);
    }
    
    // New separate pitch controls
    float GetCoarsePitch() const {
        return GetParameter(BraidyParameter::COARSE);
    }
    
    void SetCoarsePitch(float octaves) {
        SetParameter(BraidyParameter::COARSE, octaves);
    }
    
    float GetFinePitch() const {
        return GetParameter(BraidyParameter::FINE);
    }
    
    void SetFinePitch(float cents) {
        SetParameter(BraidyParameter::FINE, cents);
    }
    
    int16_t GetTimbre() const {
        return static_cast<int16_t>(GetParameter(BraidyParameter::TIMBRE));
    }
    
    void SetTimbre(int16_t timbre) {
        SetParameter(BraidyParameter::TIMBRE, static_cast<float>(timbre));
    }
    
    int16_t GetColor() const {
        return static_cast<int16_t>(GetParameter(BraidyParameter::COLOR));
    }
    
    void SetColor(int16_t color) {
        SetParameter(BraidyParameter::COLOR, static_cast<float>(color));
    }
    
    // State management
    void SaveState(void* data, size_t& size) const;
    void LoadState(const void* data, size_t size);
    void ResetToDefaults();
    
    // Parameter smoothing for audio-rate changes
    void UpdateSmoothers();
    int16_t GetSmoothedTimbre() const { return smoothed_timbre_; }
    int16_t GetSmoothedColor() const { return smoothed_color_; }
    
private:
    // Parameter storage
    float parameters_[static_cast<int>(BraidyParameter::PARAMETER_COUNT)];
    
    // Parameter smoothing for audio-critical parameters
    OnePole timbre_smoother_;
    OnePole color_smoother_;
    int16_t smoothed_timbre_;
    int16_t smoothed_color_;
    
    // Parameter metadata table
    static const ParameterInfo parameter_info_[static_cast<int>(BraidyParameter::PARAMETER_COUNT)];
    
    // Algorithm names for shape parameter
    static const char* const shape_names_[];
    
    DISALLOW_COPY_AND_ASSIGN(BraidySettings);
};

// Global settings instance
extern BraidySettings* global_settings;

}  // namespace braidy