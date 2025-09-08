#pragma once

#include "BraidyTypes.h"
#include "BraidySettings.h"
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <memory>

namespace braidy {

// Number of macro oscillator shapes (excluding LAST which is just a marker)
constexpr int kNumMacroOscillatorShapes = static_cast<int>(MacroOscillatorShape::LAST);

// Stores the state for a single waveform
struct WaveformState {
    float timbre = 0.5f;           // Normalized 0.0-1.0
    float color = 0.5f;            // Normalized 0.0-1.0
    float fm_amount = 0.0f;        // Normalized 0.0-1.0
    float fm_ratio = 1.0f;         // FM ratio
    float brightness = 0.5f;       // High frequency content
    float attack = 0.001f;         // Attack time in seconds
    float decay = 0.1f;            // Decay time in seconds
    float sustain = 0.7f;          // Sustain level 0.0-1.0
    float release = 0.05f;         // Release time in seconds
    float volume = 1.0f;           // Output volume 0.0-1.0
    
    // Advanced parameters
    float bit_crusher_bits = 16.0f;
    float bit_crusher_rate = 1.0f;
    float waveshaper_amount = 0.0f;
    int waveshaper_type = 0;
    bool meta_enabled = false;
    float meta_speed = 0.5f;
    float meta_range = 1.0f;
    bool paraphony_enabled = false;
    float paraphony_detune = 0.0f;
    bool vca_enabled = true;
    bool low_pass_gate_enabled = false;
    float low_pass_gate_decay = 0.1f;
    
    // Initialize with default values for a specific waveform
    void InitForWaveform(MacroOscillatorShape shape);
    
    // Copy state to/from BraidySettings
    void CopyToSettings(BraidySettings& settings) const;
    void CopyFromSettings(const BraidySettings& settings);
};

// Manages per-waveform state storage and persistence
class WaveformStateManager {
public:
    WaveformStateManager();
    ~WaveformStateManager() = default;
    
    // Initialize all waveforms with their default settings
    void Init();
    
    // Reset a specific waveform to defaults
    void ResetWaveform(MacroOscillatorShape shape);
    
    // Reset all waveforms to defaults
    void ResetAllToDefaults();
    
    // Get/set state for a specific waveform
    WaveformState& GetState(MacroOscillatorShape shape);
    const WaveformState& GetState(MacroOscillatorShape shape) const;
    void SetState(MacroOscillatorShape shape, const WaveformState& state);
    
    // Save current settings to the specified waveform's state
    void SaveCurrentToWaveform(MacroOscillatorShape shape, const BraidySettings& settings);
    
    // Load a waveform's state into the current settings
    void LoadWaveformToCurrent(MacroOscillatorShape shape, BraidySettings& settings);
    
    // Switch between waveforms (saves current, loads new)
    void SwitchWaveform(MacroOscillatorShape from_shape, MacroOscillatorShape to_shape,
                       BraidySettings& settings);
    
    // State persistence (for plugin save/load)
    void SaveToMemoryBlock(juce::MemoryBlock& destData) const;
    void LoadFromMemoryBlock(const void* data, size_t sizeInBytes);
    
    // Get default values for specific waveforms (based on original Braids)
    static WaveformState GetDefaultStateForWaveform(MacroOscillatorShape shape);
    
private:
    // Store state for each waveform
    std::array<WaveformState, kNumMacroOscillatorShapes> waveform_states_;
    
    // Track if states have been modified from defaults
    std::array<bool, kNumMacroOscillatorShapes> states_modified_;
    
    // Current active waveform
    MacroOscillatorShape current_shape_;
    
    DISALLOW_COPY_AND_ASSIGN(WaveformStateManager);
};

}  // namespace braidy