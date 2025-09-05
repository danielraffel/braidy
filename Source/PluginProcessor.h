#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "BraidyCore/BraidySettings.h"
#include "BraidyCore/PresetManager.h"
#include "BraidyVoice/VoiceManager.h"
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

private:
    // Braidy synthesizer components
    std::unique_ptr<braidy::BraidySettings> braidy_settings_;
    std::unique_ptr<braidy::VoiceManager> voice_manager_;
    std::unique_ptr<braidy::PresetManager> preset_manager_;
    
    // JUCE parameter management
    juce::AudioProcessorValueTreeState apvts_;
    
    // Parameter layout creation
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Parameter update handling
    void updateBraidyFromAPVTS();
    
    // MIDI processing helpers
    void processMidiMessage(const juce::MidiMessage& message);
    
    // Preset management helpers
    void loadPreset(size_t index);
    void saveCurrentAsPreset(const juce::String& name);
    
    // Performance optimization
    void optimizeForRealtime();
    bool parameter_update_pending_;
    int samples_since_parameter_update_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessor)
};
