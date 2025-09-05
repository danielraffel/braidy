#pragma once

#include "PluginProcessor.h"
#include "UI/BraidyDisplay.h"
#include "UI/BraidyEncoder.h"
#include "UI/BraidyVisualizer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

//==============================================================================
/**
 * Braidy Audio Processor Editor - Complete UI matching Mutable Instruments aesthetic
 */
class BraidyAudioProcessorEditor : public juce::AudioProcessorEditor, 
                                   public braidy::BraidyEncoder::Listener,
                                   public juce::Timer
{
public:
    explicit BraidyAudioProcessorEditor (BraidyAudioProcessor&);
    ~BraidyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Encoder listener callbacks
    void encoderValueChanged(braidy::BraidyEncoder* encoder, int delta) override;
    void encoderClicked(braidy::BraidyEncoder* encoder) override;
    void encoderLongPressed(braidy::BraidyEncoder* encoder) override;
    
    // Timer for display updates
    void timerCallback() override;

private:
    BraidyAudioProcessor& processorRef;
    
    // Core UI components
    std::unique_ptr<braidy::BraidyDisplay> display_;
    std::unique_ptr<braidy::BraidyEncoder> algorithmEncoder_;
    std::unique_ptr<braidy::BraidyEncoder> timbreEncoder_;
    std::unique_ptr<braidy::BraidyEncoder> colorEncoder_;
    std::unique_ptr<braidy::BraidyVisualizer> visualizer_;
    
    // Labels
    juce::Label algorithmLabel_;
    juce::Label timbreLabel_;
    juce::Label colorLabel_;
    juce::Label titleLabel_;
    
    // Settings and state
    enum class DisplayState {
        Algorithm,      // Show algorithm name
        Parameter,      // Show parameter values
        Menu,          // Settings menu
        Visualizer     // Focus on visualizer
    };
    
    DisplayState currentDisplayState_;
    int menuSelection_;
    bool settingsMode_;
    
    // Algorithm names for display
    static const juce::StringArray algorithmNames_;
    
    // Aesthetic properties
    juce::Colour panelColour_;
    juce::Colour highlightColour_;
    juce::Colour textColour_;
    juce::Colour accentColour_;
    
    // Audio monitoring for visualizer
    juce::AudioBuffer<float> visualizerBuffer_;
    int visualizerWritePos_;
    
    // Helper methods
    void setupComponents();
    void updateDisplay();
    void updateParameterValues();
    void handleMenuNavigation(int delta);
    void handleParameterChange(braidy::BraidyEncoder* encoder, int delta);
    
    // Visual helpers
    void drawModularBackground(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawConnections(juce::Graphics& g);
    void drawLEDs(juce::Graphics& g);
    
    // Algorithm name mapping
    juce::String getAlgorithmName(int algorithmIndex) const;
    juce::String getParameterDisplayText(const juce::String& paramId) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessorEditor)
};
