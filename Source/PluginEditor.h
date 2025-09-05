#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Braidy Audio Processor Editor - Basic UI for Phase 1
 */
class BraidyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BraidyAudioProcessorEditor (BraidyAudioProcessor&);
    ~BraidyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BraidyAudioProcessor& processorRef;

    // UI Components
    juce::Label titleLabel;
    
    // Parameter controls
    juce::Slider shapeSlider;
    juce::Label shapeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeAttachment;
    
    juce::Slider timbreSlider;
    juce::Label timbreLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timbreAttachment;
    
    juce::Slider colorSlider;
    juce::Label colorLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;
    
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    
    juce::Slider attackSlider;
    juce::Label attackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    
    juce::Slider decaySlider;
    juce::Label decayLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    
    // Status display
    juce::Label statusLabel;
    
    // Helper methods
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void setupSliderAttachment(std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,
                              juce::Slider& slider, const juce::String& paramId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessorEditor)
};
