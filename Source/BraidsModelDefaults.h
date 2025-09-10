/**
 * BraidsModelDefaults.h
 * Default parameter settings for each Braids algorithm/model
 * These ensure each model sounds authentic when selected
 */

#pragma once
#include <array>
#include <cstdint>

namespace BraidsDefaults {

// Parameter indices
enum ParamIndex {
    PARAM_TIMBRE = 0,
    PARAM_COLOR = 1,
    PARAM_ENVELOPE_ATTACK = 2,
    PARAM_ENVELOPE_DECAY = 3,
    PARAM_VCA_LEVEL = 4,
    PARAM_FM_AMOUNT = 5,
    PARAM_MODULATION = 6
};

// Default envelope settings for different model categories
struct EnvelopeDefaults {
    float attack;   // 0.0 - 1.0
    float decay;    // 0.0 - 1.0
    float sustain;  // 0.0 - 1.0
    float release;  // 0.0 - 1.0
    bool useVCA;    // Whether to use VCA envelope
};

// Default settings for each model
struct ModelDefaults {
    const char* name;        // 4-char display name
    float timbre;           // Default timbre (0.0-1.0)
    float color;            // Default color (0.0-1.0)
    float fm;               // Default FM amount (0.0-1.0)
    float modulation;       // Default modulation amount (0.0-1.0)
    EnvelopeDefaults env;   // Envelope settings
    float amplitude;        // Default amplitude (0.0-1.0)
    bool isPercussive;     // True for percussion models
    bool needsTrigger;     // True if needs trigger/strike
};

// Default settings for all 47 Braids algorithms
const std::array<ModelDefaults, 47> MODEL_DEFAULTS = {{
    // Basic Analog (0-4)
    {"CSAW", 0.0f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"MRPH", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"S/SQ", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"S/TR", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"BUZZ", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    
    // Sub variants (5-9)
    {"+SUB", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"SAW+", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"+SYN", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"SAW*", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"TRI3", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    
    // Triple variants (10-14)
    {"SQ3", 0.3f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"TR3", 0.3f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"SI3", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"RI3", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"SWRM", 0.7f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    
    // Digital filters (15-19)
    {"COMB", 0.5f, 0.7f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"TOY", 0.5f, 0.8f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"FLTR", 0.5f, 0.7f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"PEAK", 0.5f, 0.3f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"BAND", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    
    // Formants (20-24)
    {"HIGH", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"VOSM", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"VOWL", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"VOW2", 0.3f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"HARM", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    
    // FM & Physical (25-29)
    {"FM", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"FBFM", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"WTFM", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"PLUK", 0.7f, 0.5f, 0.0f, 0.0f, {0.01f, 0.5f, 0.0f, 0.5f, false}, 0.9f, false, true},
    {"BOWD", 0.5f, 0.5f, 0.0f, 0.0f, {0.1f, 0.0f, 1.0f, 0.1f, false}, 0.8f, false, false},
    {"BLOW", 0.5f, 0.5f, 0.0f, 0.0f, {0.05f, 0.0f, 1.0f, 0.2f, false}, 0.8f, false, false},
    {"FLUT", 0.5f, 0.5f, 0.0f, 0.0f, {0.05f, 0.0f, 1.0f, 0.2f, false}, 0.8f, false, false},
    {"BELL", 0.5f, 0.8f, 0.0f, 0.0f, {0.001f, 2.0f, 0.0f, 2.0f, false}, 0.9f, true, true},
    {"DRUM", 0.5f, 0.5f, 0.0f, 0.0f, {0.001f, 0.3f, 0.0f, 0.3f, false}, 0.9f, true, true},
    {"KICK", 0.5f, 0.7f, 0.0f, 0.0f, {0.001f, 0.5f, 0.0f, 0.5f, false}, 1.0f, true, true},
    
    // Percussion & Wavetable (35-39)
    {"CYMB", 0.5f, 0.7f, 0.0f, 0.0f, {0.001f, 1.5f, 0.0f, 1.5f, false}, 0.8f, true, true},
    {"SNAR", 0.5f, 0.5f, 0.0f, 0.0f, {0.001f, 0.2f, 0.0f, 0.2f, false}, 0.9f, true, true},
    {"WTBL", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"WMAP", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    {"WLIN", 0.5f, 0.0f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false},
    
    // Wavetable & Noise (40-44)
    {"WPAR", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"NOIS", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"TWLN", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"CLKN", 0.8f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"CLDS", 0.5f, 0.7f, 0.0f, 0.0f, {0.0f, 0.0f, 1.0f, 0.5f, true}, 0.7f, false, false},
    
    // Final algorithms (45-46)
    {"PART", 0.5f, 0.5f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.7f, false, false},
    {"DIGI", 0.7f, 0.3f, 0.0f, 0.0f, {0.0f, 0.3f, 1.0f, 0.3f, true}, 0.8f, false, false}
}};

// Menu settings that persist per model
struct MenuSettings {
    int8_t meta_mode;       // META - algorithm variation
    int8_t bits;           // BITS - bit depth reduction
    int8_t rate;           // RATE - sample rate reduction  
    int8_t brightness;     // BRIG - overall brightness
    int8_t trig_source;    // TSRC - trigger source
    int8_t trig_delay;     // TDLY - trigger delay
    int8_t env_attack;     // |\ATT - envelope attack
    int8_t env_decay;      // |\DEC - envelope decay
    int8_t fm_cv;          // |\FM - FM CV amount
    int8_t timbre_cv;      // |\TIM - Timbre CV amount
    int8_t color_cv;       // |\COL - Color CV amount
    int8_t vca_level;      // |\VCA - VCA level
    int8_t pitch_range;    // RANG - pitch bend range
    int8_t pitch_octave;   // OCTV - octave transpose
    int8_t quantizer;      // QNTZ - pitch quantizer
    int8_t root_note;      // ROOT - root note for quantizer
    bool flatten;          // FLAT - flatten to equal temperament
    int8_t drift;          // DRFT - oscillator drift
    int8_t signature;      // SIGN - waveshaper signature
};

// Get default menu settings for a model
inline MenuSettings getDefaultMenuSettings(int modelIndex) {
    MenuSettings settings = {};
    
    // Set defaults based on model type
    if (modelIndex >= 32 && modelIndex <= 38) {
        // Percussion models
        settings.env_attack = 0;
        settings.env_decay = 64;
        settings.vca_level = 127;
    } else {
        // Melodic models
        settings.env_attack = 0;
        settings.env_decay = 64;
        settings.vca_level = 100;
    }
    
    settings.meta_mode = 0;
    settings.bits = 16;
    settings.rate = 96;  // 96kHz default
    settings.brightness = 0;
    settings.pitch_range = 2;  // +/- 2 semitones
    settings.pitch_octave = 0;
    settings.quantizer = 0;  // Off
    settings.root_note = 0;  // C
    
    return settings;
}

} // namespace BraidsDefaults