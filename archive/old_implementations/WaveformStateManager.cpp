#include "WaveformStateManager.h"
#include <juce_data_structures/juce_data_structures.h>

namespace braidy {

// Initialize with default values for a specific waveform based on original Braids
void WaveformState::InitForWaveform(MacroOscillatorShape shape) {
    // Set common defaults
    volume = 1.0f;
    vca_enabled = true;
    attack = 0.001f;
    decay = 0.1f;
    sustain = 0.7f;
    release = 0.05f;
    
    // Set waveform-specific defaults based on original Braids characteristics
    switch (shape) {
        // Classic analog waveforms
        case MacroOscillatorShape::CSAW:
            timbre = 0.0f;     // Start with pure saw
            color = 0.5f;      // Balanced detuning
            brightness = 0.8f; // Bright character
            break;
            
        case MacroOscillatorShape::SAW_SQUARE:
            timbre = 0.5f;     // 50% morph
            color = 0.0f;      // No modulation
            brightness = 0.7f;
            break;
            
        case MacroOscillatorShape::SINE_TRIANGLE:
            timbre = 0.5f;     // Balanced shape
            color = 0.0f;      // Pure morph
            brightness = 0.4f; // Softer tone
            break;
            
        case MacroOscillatorShape::TRIPLE_SINE:
            timbre = 0.0f;     // Pure triple sine
            color = 0.0f;      // No harmonics
            brightness = 0.3f; // Very soft
            break;
            
        // FM synthesis
        case MacroOscillatorShape::FM:
            timbre = 0.3f;     // Moderate modulation index
            color = 0.25f;     // 2:1 ratio region
            fm_amount = 0.5f;
            fm_ratio = 2.0f;
            brightness = 0.6f;
            break;
            
        // Wavetable modes
        case MacroOscillatorShape::WAVETABLES:
        case MacroOscillatorShape::WAVE_MAP:
        case MacroOscillatorShape::WAVE_LINE:
        case MacroOscillatorShape::WAVE_PARAPHONIC:
            timbre = 0.0f;     // Start at first wavetable
            color = 0.5f;      // Middle interpolation
            brightness = 0.7f;
            break;
            
        // Vocal/formant modes
        case MacroOscillatorShape::VOSIM:
        case MacroOscillatorShape::VOWEL:
        case MacroOscillatorShape::VOWEL_FOF:
            timbre = 0.4f;     // Balanced formant
            color = 0.5f;      // Middle vowel
            brightness = 0.6f;
            break;
            
        // Harmonic modes
        case MacroOscillatorShape::HARMONICS:
        case MacroOscillatorShape::FEEDBACK_FM:
            timbre = 0.25f;    // Few harmonics
            color = 0.0f;      // Start simple
            brightness = 0.7f;
            break;
            
        // Drum modes
        case MacroOscillatorShape::KICK:
        case MacroOscillatorShape::SNARE:
        case MacroOscillatorShape::CYMBAL:
            timbre = 0.5f;     // Balanced tone
            color = 0.3f;      // Some variation
            brightness = 0.8f;
            attack = 0.0001f;  // Very fast attack for drums
            decay = 0.05f;     // Quick decay
            sustain = 0.0f;    // No sustain
            release = 0.01f;   // Fast release
            break;
            
        // Bell/metallic modes
        case MacroOscillatorShape::STRUCK_BELL:
        case MacroOscillatorShape::STRUCK_DRUM:
        case MacroOscillatorShape::PLUCKED:
        case MacroOscillatorShape::BOWED:
        case MacroOscillatorShape::BLOWN:
        case MacroOscillatorShape::FLUTED:
            timbre = 0.5f;
            color = 0.3f;
            brightness = 0.7f;
            attack = 0.001f;   // Fast attack
            decay = 0.3f;      // Longer decay for resonance
            sustain = 0.2f;
            release = 0.2f;
            break;
            
        // Noise modes
        case MacroOscillatorShape::FILTERED_NOISE:
        case MacroOscillatorShape::TWIN_PEAKS_NOISE:
        case MacroOscillatorShape::CLOCKED_NOISE:
        case MacroOscillatorShape::GRANULAR_CLOUD:
        case MacroOscillatorShape::PARTICLE_NOISE:
            timbre = 0.5f;
            color = 0.5f;
            brightness = 0.5f;
            break;
            
        // Chord modes
        case MacroOscillatorShape::TRIPLE_SAW:
        case MacroOscillatorShape::TRIPLE_SQUARE:
        case MacroOscillatorShape::TRIPLE_TRIANGLE:
        case MacroOscillatorShape::SAW_SWARM:
            timbre = 0.3f;     // Moderate chord density
            color = 0.5f;      // Balanced voicing
            brightness = 0.7f;
            break;
            
        // Special modes
        case MacroOscillatorShape::DIGITAL_MODULATION:
        case MacroOscillatorShape::CHAOTIC_FEEDBACK_FM:
        case MacroOscillatorShape::TOY:
        case MacroOscillatorShape::DIGITAL_FILTER_LP:
        case MacroOscillatorShape::DIGITAL_FILTER_BP:
            timbre = 0.5f;
            color = 0.5f;
            brightness = 0.6f;
            break;
            
        default:
            // Safe defaults for any unknown shapes
            timbre = 0.5f;
            color = 0.5f;
            brightness = 0.5f;
            break;
    }
    
    // Common advanced parameter defaults
    bit_crusher_bits = 16.0f;
    bit_crusher_rate = 1.0f;
    waveshaper_amount = 0.0f;
    waveshaper_type = 0;
    meta_enabled = false;
    meta_speed = 0.5f;
    meta_range = 1.0f;
    paraphony_enabled = false;
    paraphony_detune = 0.0f;
    low_pass_gate_enabled = false;
    low_pass_gate_decay = 0.1f;
}

void WaveformState::CopyToSettings(BraidySettings& settings) const {
    settings.SetParameterNormalized(BraidyParameter::TIMBRE, timbre);
    settings.SetParameterNormalized(BraidyParameter::COLOR, color);
    settings.SetParameterNormalized(BraidyParameter::FM_AMOUNT, fm_amount);
    settings.SetParameter(BraidyParameter::FM_RATIO, fm_ratio);
    settings.SetParameterNormalized(BraidyParameter::BRIGHTNESS, brightness);
    settings.SetParameter(BraidyParameter::ATTACK, attack);
    settings.SetParameter(BraidyParameter::DECAY, decay);
    settings.SetParameterNormalized(BraidyParameter::ENVELOPE_SUSTAIN, sustain);
    settings.SetParameter(BraidyParameter::ENVELOPE_RELEASE, release);
    settings.SetParameterNormalized(BraidyParameter::VOLUME, volume);
    
    // Advanced parameters
    settings.SetParameter(BraidyParameter::BIT_CRUSHER_BITS, bit_crusher_bits);
    settings.SetParameter(BraidyParameter::BIT_CRUSHER_RATE, bit_crusher_rate);
    settings.SetParameterNormalized(BraidyParameter::WAVESHAPER_AMOUNT, waveshaper_amount);
    settings.SetParameter(BraidyParameter::WAVESHAPER_TYPE, static_cast<float>(waveshaper_type));
    settings.SetParameter(BraidyParameter::META_ENABLED, meta_enabled ? 1.0f : 0.0f);
    settings.SetParameterNormalized(BraidyParameter::META_SPEED, meta_speed);
    settings.SetParameterNormalized(BraidyParameter::META_RANGE, meta_range);
    settings.SetParameter(BraidyParameter::PARAPHONY_ENABLED, paraphony_enabled ? 1.0f : 0.0f);
    settings.SetParameterNormalized(BraidyParameter::PARAPHONY_DETUNE, paraphony_detune);
    settings.SetParameter(BraidyParameter::VCA_MODE, vca_enabled ? 1.0f : 0.0f);
    settings.SetParameter(BraidyParameter::LOW_PASS_GATE_ENABLED, low_pass_gate_enabled ? 1.0f : 0.0f);
    settings.SetParameter(BraidyParameter::LOW_PASS_GATE_DECAY, low_pass_gate_decay);
}

void WaveformState::CopyFromSettings(const BraidySettings& settings) {
    timbre = settings.GetParameterNormalized(BraidyParameter::TIMBRE);
    color = settings.GetParameterNormalized(BraidyParameter::COLOR);
    fm_amount = settings.GetParameterNormalized(BraidyParameter::FM_AMOUNT);
    fm_ratio = settings.GetParameter(BraidyParameter::FM_RATIO);
    brightness = settings.GetParameterNormalized(BraidyParameter::BRIGHTNESS);
    attack = settings.GetParameter(BraidyParameter::ATTACK);
    decay = settings.GetParameter(BraidyParameter::DECAY);
    sustain = settings.GetParameterNormalized(BraidyParameter::ENVELOPE_SUSTAIN);
    release = settings.GetParameter(BraidyParameter::ENVELOPE_RELEASE);
    volume = settings.GetParameterNormalized(BraidyParameter::VOLUME);
    
    // Advanced parameters
    bit_crusher_bits = settings.GetParameter(BraidyParameter::BIT_CRUSHER_BITS);
    bit_crusher_rate = settings.GetParameter(BraidyParameter::BIT_CRUSHER_RATE);
    waveshaper_amount = settings.GetParameterNormalized(BraidyParameter::WAVESHAPER_AMOUNT);
    waveshaper_type = static_cast<int>(settings.GetParameter(BraidyParameter::WAVESHAPER_TYPE));
    meta_enabled = settings.GetParameter(BraidyParameter::META_ENABLED) > 0.5f;
    meta_speed = settings.GetParameterNormalized(BraidyParameter::META_SPEED);
    meta_range = settings.GetParameterNormalized(BraidyParameter::META_RANGE);
    paraphony_enabled = settings.GetParameter(BraidyParameter::PARAPHONY_ENABLED) > 0.5f;
    paraphony_detune = settings.GetParameterNormalized(BraidyParameter::PARAPHONY_DETUNE);
    vca_enabled = settings.GetParameter(BraidyParameter::VCA_MODE) > 0.5f;
    low_pass_gate_enabled = settings.GetParameter(BraidyParameter::LOW_PASS_GATE_ENABLED) > 0.5f;
    low_pass_gate_decay = settings.GetParameter(BraidyParameter::LOW_PASS_GATE_DECAY);
}

// WaveformStateManager implementation

WaveformStateManager::WaveformStateManager()
    : current_shape_(MacroOscillatorShape::CSAW) {
    Init();
}

void WaveformStateManager::Init() {
    // Initialize all waveforms with their default settings
    for (int i = 0; i < kNumMacroOscillatorShapes; ++i) {
        MacroOscillatorShape shape = static_cast<MacroOscillatorShape>(i);
        waveform_states_[i].InitForWaveform(shape);
        states_modified_[i] = false;
    }
}

void WaveformStateManager::ResetWaveform(MacroOscillatorShape shape) {
    int index = static_cast<int>(shape);
    if (index >= 0 && index < kNumMacroOscillatorShapes) {
        waveform_states_[index].InitForWaveform(shape);
        states_modified_[index] = false;
    }
}

void WaveformStateManager::ResetAllToDefaults() {
    Init();
}

WaveformState& WaveformStateManager::GetState(MacroOscillatorShape shape) {
    int index = static_cast<int>(shape);
    if (index < 0 || index >= kNumMacroOscillatorShapes) {
        index = 0; // Fallback to CSAW
    }
    return waveform_states_[index];
}

const WaveformState& WaveformStateManager::GetState(MacroOscillatorShape shape) const {
    int index = static_cast<int>(shape);
    if (index < 0 || index >= kNumMacroOscillatorShapes) {
        index = 0; // Fallback to CSAW
    }
    return waveform_states_[index];
}

void WaveformStateManager::SetState(MacroOscillatorShape shape, const WaveformState& state) {
    int index = static_cast<int>(shape);
    if (index >= 0 && index < kNumMacroOscillatorShapes) {
        waveform_states_[index] = state;
        states_modified_[index] = true;
    }
}

void WaveformStateManager::SaveCurrentToWaveform(MacroOscillatorShape shape, const BraidySettings& settings) {
    int index = static_cast<int>(shape);
    if (index >= 0 && index < kNumMacroOscillatorShapes) {
        waveform_states_[index].CopyFromSettings(settings);
        states_modified_[index] = true;
    }
}

void WaveformStateManager::LoadWaveformToCurrent(MacroOscillatorShape shape, BraidySettings& settings) {
    int index = static_cast<int>(shape);
    if (index >= 0 && index < kNumMacroOscillatorShapes) {
        waveform_states_[index].CopyToSettings(settings);
        current_shape_ = shape;
    }
}

void WaveformStateManager::SwitchWaveform(MacroOscillatorShape from_shape, MacroOscillatorShape to_shape,
                                         BraidySettings& settings) {
    // Save current waveform's state
    SaveCurrentToWaveform(from_shape, settings);
    
    // Load new waveform's state
    LoadWaveformToCurrent(to_shape, settings);
}

void WaveformStateManager::SaveToMemoryBlock(juce::MemoryBlock& destData) const {
    destData.ensureSize(sizeof(WaveformState) * kNumMacroOscillatorShapes + sizeof(bool) * kNumMacroOscillatorShapes + sizeof(int), false);
    
    char* data = static_cast<char*>(destData.getData());
    size_t offset = 0;
    
    // Save current shape
    int current = static_cast<int>(current_shape_);
    memcpy(data + offset, &current, sizeof(int));
    offset += sizeof(int);
    
    // Save all waveform states
    memcpy(data + offset, waveform_states_.data(), sizeof(WaveformState) * kNumMacroOscillatorShapes);
    offset += sizeof(WaveformState) * kNumMacroOscillatorShapes;
    
    // Save modified flags
    memcpy(data + offset, states_modified_.data(), sizeof(bool) * kNumMacroOscillatorShapes);
}

void WaveformStateManager::LoadFromMemoryBlock(const void* data, size_t sizeInBytes) {
    if (sizeInBytes < sizeof(WaveformState) * kNumMacroOscillatorShapes + sizeof(bool) * kNumMacroOscillatorShapes + sizeof(int)) {
        return; // Invalid data
    }
    
    const char* dataPtr = static_cast<const char*>(data);
    size_t offset = 0;
    
    // Load current shape
    int current;
    memcpy(&current, dataPtr + offset, sizeof(int));
    current_shape_ = static_cast<MacroOscillatorShape>(current);
    offset += sizeof(int);
    
    // Load all waveform states
    memcpy(waveform_states_.data(), dataPtr + offset, sizeof(WaveformState) * kNumMacroOscillatorShapes);
    offset += sizeof(WaveformState) * kNumMacroOscillatorShapes;
    
    // Load modified flags
    memcpy(states_modified_.data(), dataPtr + offset, sizeof(bool) * kNumMacroOscillatorShapes);
}

WaveformState WaveformStateManager::GetDefaultStateForWaveform(MacroOscillatorShape shape) {
    WaveformState state;
    state.InitForWaveform(shape);
    return state;
}

}  // namespace braidy