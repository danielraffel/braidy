#include "BraidyDisplay.h"

namespace braidy {

BraidyDisplay::BraidyDisplay()
    : mode_(DisplayMode::Normal)
    , brightness_(1.0f)
    , blinkState_(true)
    , scrollOffset_(0)
    , backgroundColour_(juce::Colour(0xff0a0a0a))
    , ledColour_(juce::Colour(0xff00ff40))  // Bright green like original Braids
    , displayFont_(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold))
{
    setText("----");
    startTimer(500);  // 2Hz blink rate
}

void BraidyDisplay::setText(const juce::String& text) {
    if (currentText_ != text) {
        currentText_ = text;
        scrollOffset_ = 0;
        updateDisplayText();
        repaint();
    }
}

void BraidyDisplay::setNumber(int number) {
    juce::String numStr = juce::String(number);
    if (numStr.length() < 4) {
        numStr = juce::String::repeatedString("0", 4 - numStr.length()) + numStr;
    }
    setText(numStr);
}

void BraidyDisplay::setDisplayMode(DisplayMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        
        // Adjust timer frequency based on mode
        if (mode == DisplayMode::Blinking) {
            startTimer(500);  // 1Hz blink
        } else if (mode == DisplayMode::Scrolling) {
            startTimer(250);  // 4Hz scroll
        } else {
            startTimer(1000); // Slow update for normal mode
        }
        
        repaint();
    }
}

void BraidyDisplay::setBrightness(float brightness) {
    brightness_ = juce::jlimit(0.0f, 1.0f, brightness);
    repaint();
}

void BraidyDisplay::updateDisplayText() {
    if (currentText_.length() <= 4) {
        displayText_ = currentText_.paddedRight(' ', 4);
    } else if (mode_ == DisplayMode::Scrolling) {
        // Create scrolling display
        juce::String extendedText = currentText_ + "    " + currentText_;  // Add spacing and repeat
        int startPos = scrollOffset_ % (currentText_.length() + 4);
        displayText_ = extendedText.substring(startPos, startPos + 4);
    } else {
        displayText_ = currentText_.substring(0, 4);
    }
}

void BraidyDisplay::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background (dark)
    g.setColour(backgroundColour_);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Inner bezel
    g.setColour(backgroundColour_.brighter(0.1f));
    g.drawRoundedRectangle(bounds.reduced(2.0f), 4.0f, 1.0f);
    
    // Segment grid background (subtle)
    auto displayArea = bounds.reduced(8.0f);
    g.setColour(backgroundColour_.brighter(0.05f));
    
    float charWidth = displayArea.getWidth() / 4.0f;
    for (int i = 0; i < 4; ++i) {
        auto charBounds = displayArea.removeFromLeft(charWidth);
        g.drawRect(charBounds.reduced(2.0f), 0.5f);
    }
    
    // Display characters
    if (mode_ != DisplayMode::Blinking || blinkState_) {
        displayArea = bounds.reduced(8.0f);
        charWidth = displayArea.getWidth() / 4.0f;
        
        g.setFont(displayFont_);
        
        for (int i = 0; i < 4; ++i) {
            auto charBounds = displayArea.removeFromLeft(charWidth);
            char c = (i < displayText_.length()) ? displayText_[i] : ' ';
            drawCharacter(g, charBounds, c);
        }
    }
}

void BraidyDisplay::drawCharacter(juce::Graphics& g, juce::Rectangle<float> bounds, char character) {
    if (character == ' ' || character == 0) return;
    
    // Calculate LED brightness
    auto currentColour = ledColour_.withAlpha(brightness_);
    
    // Add glow effect
    g.setColour(currentColour.withAlpha(brightness_ * 0.3f));
    g.fillRoundedRectangle(bounds.reduced(1.0f), 2.0f);
    
    // Main character
    g.setColour(currentColour);
    g.setFont(displayFont_);
    g.drawText(juce::String::charToString(character), bounds, 
               juce::Justification::centred, false);
}

void BraidyDisplay::resized() {
    // Adjust font size based on component size
    float fontSize = jmin(getHeight() * 0.6f, getWidth() * 0.15f);
    displayFont_ = juce::Font(juce::Font::getDefaultMonospacedFontName(), fontSize, juce::Font::bold);
}

void BraidyDisplay::timerCallback() {
    switch (mode_) {
        case DisplayMode::Blinking:
            blinkState_ = !blinkState_;
            repaint();
            break;
            
        case DisplayMode::Scrolling:
            if (currentText_.length() > 4) {
                scrollOffset_++;
                updateDisplayText();
                repaint();
            }
            break;
            
        case DisplayMode::Normal:
        default:
            // Just update occasionally in case external state changed
            break;
    }
}

}  // namespace braidy