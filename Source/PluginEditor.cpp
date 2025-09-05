#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BraidyAudioProcessorEditor::BraidyAudioProcessorEditor (BraidyAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (400, 300);
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor()
{
}

void BraidyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void BraidyAudioProcessorEditor::resized()
{
    // This is where you'd lay out your UI components
}
