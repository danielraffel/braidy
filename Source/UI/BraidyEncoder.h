#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace braidy {

/**
 * Encoder control with click/turn behavior matching Mutable Instruments Braids
 */
class BraidyEncoder : public juce::Component
{
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void encoderValueChanged(BraidyEncoder* encoder, int delta) {}
        virtual void encoderClicked(BraidyEncoder* encoder) {}
        virtual void encoderLongPressed(BraidyEncoder* encoder) {}
    };
    
    BraidyEncoder();
    ~BraidyEncoder() override = default;
    
    // Settings
    void setRange(int minimum, int maximum);
    void setValue(int value, bool sendNotification = true);
    int getValue() const { return currentValue_; }
    
    void setStepSize(int stepSize) { stepSize_ = stepSize; }
    void setSensitivity(float sensitivity) { sensitivity_ = sensitivity; }  // 0.1 to 10.0
    
    // Visual settings
    void setIndicatorColour(juce::Colour colour) { indicatorColour_ = colour; repaint(); }
    void setCapColour(juce::Colour colour) { capColour_ = colour; repaint(); }
    void setBodyColour(juce::Colour colour) { bodyColour_ = colour; repaint(); }
    
    // Listeners
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
private:
    // Value management
    int currentValue_;
    int minimumValue_;
    int maximumValue_;
    int stepSize_;
    float sensitivity_;
    
    // Mouse tracking
    juce::Point<int> lastMousePos_;
    bool isDragging_;
    bool isPressed_;
    juce::Time pressStartTime_;
    
    // Visual properties
    juce::Colour indicatorColour_;
    juce::Colour capColour_;
    juce::Colour bodyColour_;
    bool hasFocus_;
    
    // Listeners
    juce::ListenerList<Listener> listeners_;
    
    // Helper methods
    void setValue(int value, bool sendNotification, bool allowOutOfRange);
    void notifyValueChanged(int delta);
    void notifyClicked();
    void notifyLongPressed();
    
    float getIndicatorAngle() const;
    void drawEncoderBody(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawIndicator(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawFocusIndicator(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidyEncoder)
};

}  // namespace braidy