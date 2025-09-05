#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessorEditor)
};
