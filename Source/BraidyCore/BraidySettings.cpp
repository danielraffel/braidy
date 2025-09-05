#include "BraidySettings.h"
#include <algorithm>
#include <cstring>

namespace braidy {

BraidySettings* global_settings = nullptr;

// Algorithm/shape names
const char* const BraidySettings::shape_names_[] = {
    "CSAW", "MORPH", "SAW_SQUARE", "SINE_TRI", "BUZZ",
    "SQ_SUB", "SAW_SUB", "SQ_SYNC", "SAW_SYNC", "TRI_SAW", 
    "TRI_SQ", "TRI_TRI", "TRI_SIN", "TRI_RING", "SAW_SWARM",
    "SAW_COMB", "TOY", "LP_FILT", "PK_FILT", "BP_FILT",
    "HP_FILT", "VOSIM", "VOWEL", "VOW_FOF", "HARM",
    "FM", "FB_FM", "WTFM", "PLUCK", "BOW", "BLOW", "FLUTE",
    "BELL", "DRUM", "KICK", "CYMBAL", "SNARE", "WTBL", "WMAP",
    "WLIN", "WPAR", "NOISE", "TWLN", "CLKN", "CLOUD", "PART",
    "DIGI", "????"
};

// Parameter metadata
const ParameterInfo BraidySettings::parameter_info_[static_cast<int>(BraidyParameter::PARAMETER_COUNT)] = {
    // SHAPE
    {"Algorithm", "ALG", "Synthesis algorithm/oscillator shape", 
     0.0f, static_cast<float>(static_cast<int>(MacroOscillatorShape::LAST) - 1), 0.0f, true, shape_names_},
    
    // PITCH 
    {"Pitch", "PIT", "Fundamental pitch in semitones", 
     -48.0f, 48.0f, 0.0f, false, nullptr},
    
    // TIMBRE
    {"Timbre", "TMB", "Timbral parameter (varies by algorithm)", 
     0.0f, 1.0f, 0.5f, false, nullptr},
    
    // COLOR
    {"Color", "COL", "Color parameter (varies by algorithm)", 
     0.0f, 1.0f, 0.5f, false, nullptr},
    
    // FM_AMOUNT
    {"FM Amount", "FM", "FM modulation depth", 
     0.0f, 1.0f, 0.0f, false, nullptr},
    
    // FM_RATIO
    {"FM Ratio", "RAT", "FM frequency ratio", 
     0.125f, 8.0f, 1.0f, false, nullptr},
    
    // ATTACK
    {"Attack", "ATT", "Envelope attack time", 
     0.0f, 1.0f, 0.1f, false, nullptr},
    
    // DECAY
    {"Decay", "DEC", "Envelope decay time", 
     0.0f, 1.0f, 0.5f, false, nullptr},
    
    // VOLUME
    {"Volume", "VOL", "Output volume", 
     0.0f, 1.0f, 0.7f, false, nullptr},
    
    // BRIGHTNESS
    {"Brightness", "BRI", "High frequency content", 
     0.0f, 1.0f, 0.5f, false, nullptr},
    
    // META_MODULATION
    {"Meta Mod", "MET", "Enable meta-modulation", 
     0.0f, 1.0f, 0.0f, true, nullptr},
    
    // PITCH_OCTAVE
    {"Octave", "OCT", "Pitch octave offset", 
     -2.0f, 2.0f, 0.0f, true, nullptr},
    
    // QUANTIZER_SCALE
    {"Scale", "SCL", "Quantizer scale", 
     0.0f, 47.0f, 0.0f, true, nullptr},
    
    // QUANTIZER_ROOT
    {"Root", "ROT", "Quantizer root note", 
     0.0f, 11.0f, 0.0f, true, nullptr}
};

BraidySettings::BraidySettings() 
    : smoothed_timbre_(kParameterCenter)
    , smoothed_color_(kParameterCenter)
{
    // Initialize parameter smoothers
    timbre_smoother_.set_f(0.05f);  // 50ms smoothing
    color_smoother_.set_f(0.05f);
}

void BraidySettings::Init() {
    ResetToDefaults();
}

void BraidySettings::ResetToDefaults() {
    for (int i = 0; i < static_cast<int>(BraidyParameter::PARAMETER_COUNT); ++i) {
        parameters_[i] = parameter_info_[i].default_value;
    }
    
    smoothed_timbre_ = static_cast<int16_t>(GetParameter(BraidyParameter::TIMBRE) * kParameterMax);
    smoothed_color_ = static_cast<int16_t>(GetParameter(BraidyParameter::COLOR) * kParameterMax);
}

float BraidySettings::GetParameter(BraidyParameter param) const {
    int index = static_cast<int>(param);
    if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
        return parameters_[index];
    }
    return 0.0f;
}

void BraidySettings::SetParameter(BraidyParameter param, float value) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
        const ParameterInfo& info = parameter_info_[index];
        parameters_[index] = std::clamp(value, info.min_value, info.max_value);
    }
}

float BraidySettings::GetParameterNormalized(BraidyParameter param) const {
    int index = static_cast<int>(param);
    if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
        const ParameterInfo& info = parameter_info_[index];
        float value = parameters_[index];
        return (value - info.min_value) / (info.max_value - info.min_value);
    }
    return 0.0f;
}

void BraidySettings::SetParameterNormalized(BraidyParameter param, float normalized_value) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
        const ParameterInfo& info = parameter_info_[index];
        float value = info.min_value + normalized_value * (info.max_value - info.min_value);
        SetParameter(param, value);
    }
}

const ParameterInfo& BraidySettings::GetParameterInfo(BraidyParameter param) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
        return parameter_info_[index];
    }
    return parameter_info_[0];  // Fallback
}

const char* BraidySettings::GetParameterName(BraidyParameter param) {
    return GetParameterInfo(param).name;
}

void BraidySettings::UpdateSmoothers() {
    // Update parameter smoothing for audio-critical parameters
    float target_timbre = GetParameter(BraidyParameter::TIMBRE) * kParameterMax;
    float target_color = GetParameter(BraidyParameter::COLOR) * kParameterMax;
    
    smoothed_timbre_ = static_cast<int16_t>(timbre_smoother_.Process(target_timbre));
    smoothed_color_ = static_cast<int16_t>(color_smoother_.Process(target_color));
}

void BraidySettings::SaveState(void* data, size_t& size) const {
    size = sizeof(parameters_);
    if (data != nullptr) {
        std::memcpy(data, parameters_, size);
    }
}

void BraidySettings::LoadState(const void* data, size_t size) {
    if (data != nullptr && size == sizeof(parameters_)) {
        std::memcpy(parameters_, data, size);
        
        // Update smoothers with loaded values
        smoothed_timbre_ = static_cast<int16_t>(GetParameter(BraidyParameter::TIMBRE) * kParameterMax);
        smoothed_color_ = static_cast<int16_t>(GetParameter(BraidyParameter::COLOR) * kParameterMax);
    }
}

}  // namespace braidy