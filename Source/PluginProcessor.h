#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BraidyCore/BraidySettings.h"
#include "BraidyCore/BraidyTypes.h"
#include "BraidyCore/PresetManager.h"
#include "BraidyCore/WaveformStateManager.h"
#include "BraidyVoice/VoiceManager.h"
#include "Modulation/ModulationMatrix.h"
#include <memory>

//==============================================================================
/**
 * Braidy Audio Processor - Mutable Instruments Braids port for JUCE
 */
class BraidyAudioProcessor : public juce::AudioProcessor
{
public:
    BraidyAudioProcessor();
    ~BraidyAudioProcessor() override;

    // AudioProcessor interface
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter management
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }
    braidy::BraidySettings& getBraidySettings() { return *braidy_settings_; }
    
    // Voice management
    braidy::VoiceManager& getVoiceManager() { return *voice_manager_; }
    
    // Preset management
    braidy::PresetManager& getPresetManager() { return *preset_manager_; }
    
    // Waveform state management
    braidy::WaveformStateManager& getWaveformStateManager() { return *waveform_state_manager_; }
    
    // Modulation system
    braidy::ModulationMatrix& getModulationMatrix() { return *modulation_matrix_; }
    void setMetaModeEnabled(bool enabled);
    void setQuantizerEnabled(bool enabled);
    void setBitCrusherEnabled(bool enabled);
    
    // MIDI processing (public for editor keyboard input)
    void processMidiMessage(const juce::MidiMessage& message);

private:
    // Braidy synthesizer components
    std::unique_ptr<braidy::BraidySettings> braidy_settings_;
    std::unique_ptr<braidy::VoiceManager> voice_manager_;
    std::unique_ptr<braidy::PresetManager> preset_manager_;
    std::unique_ptr<braidy::WaveformStateManager> waveform_state_manager_;
    std::unique_ptr<braidy::ModulationMatrix> modulation_matrix_;
    
    // JUCE parameter management
    juce::AudioProcessorValueTreeState apvts_;
    
    // Parameter layout creation
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Parameter update handling
    void updateBraidyFromAPVTS();
    
    // 48kHz core processing
    void setupResamplerChain(double hostSampleRate);
    void processWithFixedCore(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // Preset management helpers
    void loadPreset(size_t index);
    void saveCurrentAsPreset(const juce::String& name);
    
    // Performance optimization
    void optimizeForRealtime();
    bool parameter_update_pending_;
    int samples_since_parameter_update_;
    
    // Fixed 48kHz core processing
    static constexpr double kCoreSampleRate = 48000.0;
    double host_sample_rate_;
    bool needs_resampling_;
    
    // Resampler for non-48kHz hosts
    std::unique_ptr<juce::dsp::Oversampling<float>> resampler_;
    
    // Internal 48kHz buffers
    juce::AudioBuffer<float> core_input_buffer_;
    juce::AudioBuffer<float> core_output_buffer_;
    
    // 24-sample micro-block processing (using kBlockSize from BraidyTypes.h)
    std::vector<float> micro_block_buffer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessor)
};
