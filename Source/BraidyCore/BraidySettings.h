#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"

namespace braidy {

// Parameter definitions for Braidy synthesizer
enum class BraidyParameter {
    // Core synthesis parameters
    SHAPE = 0,              // Oscillator shape/algorithm
    PITCH,                  // Fundamental pitch
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
        return static_cast<int16_t>(GetParameter(BraidyParameter::PITCH) * 128.0f);  // Convert to internal format
    }
    
    void SetPitch(int16_t pitch) {
        SetParameter(BraidyParameter::PITCH, static_cast<float>(pitch) / 128.0f);
    }
    
    int16_t GetTimbre() const {
        return static_cast<int16_t>(GetParameter(BraidyParameter::TIMBRE) * kParameterMax);
    }
    
    void SetTimbre(int16_t timbre) {
        SetParameter(BraidyParameter::TIMBRE, static_cast<float>(timbre) / kParameterMax);
    }
    
    int16_t GetColor() const {
        return static_cast<int16_t>(GetParameter(BraidyParameter::COLOR) * kParameterMax);
    }
    
    void SetColor(int16_t color) {
        SetParameter(BraidyParameter::COLOR, static_cast<float>(color) / kParameterMax);
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