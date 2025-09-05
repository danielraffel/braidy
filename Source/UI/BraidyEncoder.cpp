#include "BraidyEncoder.h"
#include <cmath>

namespace braidy {

BraidyEncoder::BraidyEncoder()
    : currentValue_(0)
    , minimumValue_(0)
    , maximumValue_(127)
    , stepSize_(1)
    , sensitivity_(1.0f)
    , isDragging_(false)
    , isPressed_(false)
    , indicatorColour_(juce::Colour(0xff00ff40))  // Bright green
    , capColour_(juce::Colour(0xff2a2a2a))       // Dark gray
    , bodyColour_(juce::Colour(0xff1a1a1a))      // Very dark gray
    , hasFocus_(false)
{
    setWantsKeyboardFocus(true);
    setRepaintsOnMouseActivity(true);
}

void BraidyEncoder::setRange(int minimum, int maximum) {
    if (minimum != minimumValue_ || maximum != maximumValue_) {
        minimumValue_ = minimum;
        maximumValue_ = maximum;
        setValue(juce::jlimit(minimum, maximum, currentValue_), true);
    }
}

void BraidyEncoder::setValue(int value, bool sendNotification) {
    setValue(value, sendNotification, false);
}

void BraidyEncoder::setValue(int value, bool sendNotification, bool allowOutOfRange) {
    int newValue = allowOutOfRange ? value : juce::jlimit(minimumValue_, maximumValue_, value);
    
    if (currentValue_ != newValue) {
        int delta = newValue - currentValue_;
        currentValue_ = newValue;
        repaint();
        
        if (sendNotification) {
            notifyValueChanged(delta);
        }
    }
}

void BraidyEncoder::addListener(Listener* listener) {
    listeners_.add(listener);
}

void BraidyEncoder::removeListener(Listener* listener) {
    listeners_.remove(listener);
}

void BraidyEncoder::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    
    // Draw encoder body
    auto encoderBounds = juce::Rectangle<float>(radius * 2, radius * 2).withCentre(center);
    drawEncoderBody(g, encoderBounds);
    
    // Draw indicator
    drawIndicator(g, encoderBounds);
    
    // Draw focus indicator if needed
    if (hasFocus_) {
        drawFocusIndicator(g, bounds);
    }
    
    // Draw press state
    if (isPressed_) {
        g.setColour(indicatorColour_.withAlpha(0.3f));
        g.fillEllipse(encoderBounds.reduced(2.0f));
    }
}

void BraidyEncoder::drawEncoderBody(juce::Graphics& g, juce::Rectangle<float> bounds) {
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.5f;
    
    // Outer ring (body)
    g.setColour(bodyColour_);
    g.fillEllipse(bounds);
    
    // Shading for 3D effect
    g.setColour(bodyColour_.brighter(0.2f));
    g.drawEllipse(bounds.reduced(1.0f), 1.0f);
    
    g.setColour(bodyColour_.darker(0.3f));
    g.drawEllipse(bounds.reduced(0.5f), 1.0f);
    
    // Inner cap
    auto capBounds = bounds.reduced(radius * 0.3f);
    g.setColour(capColour_);
    g.fillEllipse(capBounds);
    
    // Cap shading
    g.setColour(capColour_.brighter(0.1f));
    g.drawEllipse(capBounds.reduced(0.5f), 0.5f);
    
    // Knurled edge effect
    g.setColour(bodyColour_.darker(0.4f));
    for (int i = 0; i < 24; ++i) {
        float angle = i * juce::MathConstants<float>::twoPi / 24;
        float x1 = center.x + (radius - 3) * std::cos(angle);
        float y1 = center.y + (radius - 3) * std::sin(angle);
        float x2 = center.x + radius * std::cos(angle);
        float y2 = center.y + radius * std::sin(angle);
        
        g.drawLine(x1, y1, x2, y2, 0.5f);
    }
}

void BraidyEncoder::drawIndicator(juce::Graphics& g, juce::Rectangle<float> bounds) {
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.4f;
    
    float angle = getIndicatorAngle();
    
    // Calculate indicator position
    float indicatorRadius = radius * 0.7f;
    float x = center.x + indicatorRadius * std::cos(angle);
    float y = center.y + indicatorRadius * std::sin(angle);
    
    // Draw indicator dot
    g.setColour(indicatorColour_);
    g.fillEllipse(x - 3, y - 3, 6, 6);
    
    // Draw indicator line from center
    g.setColour(indicatorColour_.withAlpha(0.6f));
    g.drawLine(center.x, center.y, x, y, 2.0f);
    
    // Glow effect
    g.setColour(indicatorColour_.withAlpha(0.3f));
    g.fillEllipse(x - 5, y - 5, 10, 10);
}

void BraidyEncoder::drawFocusIndicator(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(indicatorColour_.withAlpha(0.5f));
    
    // Draw dashed focus ring
    juce::Path focusPath;
    focusPath.addEllipse(bounds.reduced(2.0f));
    
    // Draw dashed focus ring
    juce::PathStrokeType strokeType(1.0f);
    float dashLengths[] = {3.0f, 3.0f};
    strokeType.createDashedStroke(focusPath, focusPath, dashLengths, 2);
    g.strokePath(focusPath, strokeType);
}

float BraidyEncoder::getIndicatorAngle() const {
    // Map value to angle (-150° to +150°, 300° total range)
    float range = static_cast<float>(maximumValue_ - minimumValue_);
    float normalizedValue = static_cast<float>(currentValue_ - minimumValue_) / range;
    
    float startAngle = -150.0f * juce::MathConstants<float>::pi / 180.0f;
    float endAngle = 150.0f * juce::MathConstants<float>::pi / 180.0f;
    
    return startAngle + normalizedValue * (endAngle - startAngle);
}

void BraidyEncoder::mouseDown(const juce::MouseEvent& e) {
    if (!isEnabled()) return;
    
    grabKeyboardFocus();
    
    lastMousePos_ = e.getPosition();
    isDragging_ = false;
    isPressed_ = true;
    pressStartTime_ = juce::Time::getCurrentTime();
    
    repaint();
}

void BraidyEncoder::mouseUp(const juce::MouseEvent& e) {
    if (!isEnabled()) return;
    
    bool wasPressed = isPressed_;
    isPressed_ = false;
    isDragging_ = false;
    
    if (wasPressed) {
        auto pressTime = juce::Time::getCurrentTime() - pressStartTime_;
        
        if (pressTime.inMilliseconds() > 500) {
            // Long press
            notifyLongPressed();
        } else if (!isDragging_) {
            // Click
            notifyClicked();
        }
    }
    
    repaint();
}

void BraidyEncoder::mouseDrag(const juce::MouseEvent& e) {
    if (!isEnabled() || !isPressed_) return;
    
    auto currentPos = e.getPosition();
    auto delta = currentPos - lastMousePos_;
    
    // Calculate rotation based on mouse movement
    float mouseDelta = -(delta.y - delta.x) * sensitivity_;  // Diagonal drag
    
    if (std::abs(mouseDelta) > 0.5f) {
        isDragging_ = true;
        
        int valueDelta = static_cast<int>(mouseDelta / 5.0f);  // Scale down
        if (valueDelta != 0) {
            setValue(currentValue_ + valueDelta * stepSize_, true, false);
        }
        
        lastMousePos_ = currentPos;
    }
}

void BraidyEncoder::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (!isEnabled()) return;
    
    int delta = static_cast<int>(wheel.deltaY * 5.0f * sensitivity_);
    if (delta != 0) {
        setValue(currentValue_ + delta * stepSize_, true, false);
    }
}

bool BraidyEncoder::keyPressed(const juce::KeyPress& key) {
    if (!isEnabled()) return false;
    
    int delta = 0;
    
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::rightKey) {
        delta = stepSize_;
    } else if (key == juce::KeyPress::downKey || key == juce::KeyPress::leftKey) {
        delta = -stepSize_;
    } else if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey) {
        notifyClicked();
        return true;
    }
    
    if (delta != 0) {
        setValue(currentValue_ + delta, true, false);
        return true;
    }
    
    return false;
}

void BraidyEncoder::focusGained(juce::Component::FocusChangeType cause) {
    hasFocus_ = true;
    repaint();
}

void BraidyEncoder::focusLost(juce::Component::FocusChangeType cause) {
    hasFocus_ = false;
    repaint();
}

void BraidyEncoder::notifyValueChanged(int delta) {
    listeners_.call([this, delta](Listener& l) { l.encoderValueChanged(this, delta); });
}

void BraidyEncoder::notifyClicked() {
    listeners_.call([this](Listener& l) { l.encoderClicked(this); });
}

void BraidyEncoder::notifyLongPressed() {
    listeners_.call([this](Listener& l) { l.encoderLongPressed(this); });
}

}  // namespace braidy