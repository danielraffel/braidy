#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "adapters/BraidsSynthesiser.h"
#include <memory>

/**
 * BraidyAudioProcessor - JUCE plugin wrapping Mutable Instruments Braids
 */
class BraidyAudioProcessor : public juce::AudioProcessor
{
public:
    BraidyAudioProcessor();
    ~BraidyAudioProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Braidy"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }
    
    // Get algorithm names for UI
    static std::vector<std::string> getAlgorithmNames();
    
    // Access to synthesiser for MIDI input
    BraidyAdapter::BraidsSynthesiser* getSynthesiser() { return synthesiser_.get(); }
    
    // MIDI collector for keyboard input
    juce::MidiMessageCollector& getMidiCollector() { return midiCollector_; }

private:
    // Braids synthesiser
    std::unique_ptr<BraidyAdapter::BraidsSynthesiser> synthesiser_;
    
    // MIDI collector for UI keyboard
    juce::MidiMessageCollector midiCollector_;
    
    // JUCE parameter management
    juce::AudioProcessorValueTreeState apvts_;
    
    // Parameter IDs
    static constexpr const char* ALGORITHM_ID = "algorithm";
    static constexpr const char* PARAM1_ID = "param1";
    static constexpr const char* PARAM2_ID = "param2";
    static constexpr const char* VOLUME_ID = "volume";
    
    // Parameter layout creation
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Parameter update handling
    void updateSynthesiserFromParameters();
    
    // Current state
    std::atomic<int> currentAlgorithm_{0};
    std::atomic<float> currentParam1_{0.5f};
    std::atomic<float> currentParam2_{0.5f};
    std::atomic<float> currentVolume_{0.7f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidyAudioProcessor)
};