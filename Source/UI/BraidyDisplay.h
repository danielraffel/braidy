#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
// #include "../BraidyCore/BraidyTypes.h" // No longer needed

namespace braidy {

/**
 * 4-character LED display simulation matching Mutable Instruments Braids
 */
class BraidyDisplay : public juce::Component, public juce::Timer
{
public:
    BraidyDisplay();
    ~BraidyDisplay() override = default;
    
    // Display text (4 characters max)
    void setText(const juce::String& text);
    void setNumber(int number);  // Display number with leading zeros if needed
    
    // Display modes
    enum class DisplayMode {
        Normal,     // Solid text
        Blinking,   // Text blinks on/off
        Scrolling   // Text scrolls if longer than 4 chars
    };
    
    void setDisplayMode(DisplayMode mode);
    void setBrightness(float brightness);  // 0.0 to 1.0
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    juce::String currentText_;
    juce::String displayText_;  // What's actually shown (4 chars)
    DisplayMode mode_;
    float brightness_;
    
    // Animation state
    bool blinkState_;
    int scrollOffset_;
    
    // Visual properties
    juce::Colour backgroundColour_;
    juce::Colour ledColour_;
    juce::Font displayFont_;
    
    void updateDisplayText();
    void drawCharacter(juce::Graphics& g, juce::Rectangle<float> bounds, char character);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidyDisplay)
};

}  // namespace braidy