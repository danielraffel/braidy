#include "PluginProcessor.h"
#include "PluginEditor.h"

// Exact algorithm names from Mutable Instruments Braids (4-character display)
const std::array<const char*, 48> BraidyAudioProcessorEditor::algorithmNames_ = {
    "CSAW", "MRPH", "S/SQ", "S/TR", "BUZZ",          // Basic analog (0-4)
    "+SUB", "SAW+", "+SYN", "SAW*", "TRI3",          // Sub variants (5-9)
    "SQ3",  "TR3",  "SI3",  "RI3",  "SWRM",          // Triple variants (10-14)
    "COMB", "TOY",  "FLTR", "PEAK", "BAND",          // Digital filters (15-19)
    "HIGH", "VOSM", "VOWL", "VOW2", "HARM",          // Formants (20-24)
    "FM",   "FBFM", "WTFM", "PLUK", "BOWD",          // FM & Physical (25-29)
    "BLOW", "FLUT", "BELL", "DRUM", "KICK",          // Physical modeling (30-34)
    "CYMB", "SNAR", "WTBL", "WMAP", "WLIN",          // Percussion & Wavetable (35-39)
    "WPAR", "NOIS", "TWLN", "CLKN", "CLDS",          // Wavetable & Noise (40-44)
    "PART", "DIGI", "????"                           // Final algorithms (45-47)
};

// Exact menu page names from Braids (4-character display)
const std::array<const char*, 20> BraidyAudioProcessorEditor::menuPageNames_ = {
    "META", "BITS", "RATE", "TSRC", "TDLY",          // 0-4
    "|\\ATT", "|\\DEC", "|\\FM", "|\\TIM", "|\\COL",  // 5-9 (envelope settings)
    "|\\VCA", "RANG", "OCTV", "QNTZ", "ROOT",        // 10-14
    "FLAT", "DRFT", "SIGN", "CAL>", "VERS"           // 15-19
};

//==============================================================================
// BraidsEncoder Implementation
//==============================================================================
BraidyAudioProcessorEditor::BraidsEncoder::BraidsEncoder() {
    setSize(60, 60);  // Large encoder like on Braids
    setInterceptsMouseClicks(true, false);  // Accept mouse clicks but don't intercept children
    setWantsKeyboardFocus(false);
    DBG("BraidsEncoder constructed");
}

void BraidyAudioProcessorEditor::BraidsEncoder::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();
    auto radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    
    // Outer ring (encoder body) - dark gray
    g.setColour(juce::Colour(BraidsColors::encoder));
    g.fillEllipse(bounds.reduced(2));
    
    // Inner ring highlight
    g.setColour(juce::Colour(BraidsColors::encoder).brighter(0.1f));
    g.drawEllipse(bounds.reduced(4), 1.0f);
    
    // Encoder cap - slightly lighter
    auto capBounds = bounds.reduced(8);
    g.setColour(juce::Colour(BraidsColors::encoder).brighter(0.2f));
    g.fillEllipse(capBounds);
    
    // Position indicator (white dot)
    g.setColour(juce::Colours::white);
    auto indicatorRadius = 2.0f;
    auto indicatorDistance = radius * 0.7f;
    auto indicatorX = center.x + std::cos(angle_) * indicatorDistance;
    auto indicatorY = center.y + std::sin(angle_) * indicatorDistance;
    g.fillEllipse(indicatorX - indicatorRadius, indicatorY - indicatorRadius, 
                  indicatorRadius * 2, indicatorRadius * 2);
    
    // Pressed state
    if (isPressed_) {
        g.setColour(juce::Colour(BraidsColors::encoder).darker(0.3f));
        g.fillEllipse(capBounds.reduced(2));
    }
}

void BraidyAudioProcessorEditor::BraidsEncoder::mouseDown(const juce::MouseEvent& e) {
    DBG("=== ENCODER MOUSE DOWN ===");
    isPressed_ = true;
    longPressTriggered_ = false;
    pressStartTime_ = juce::Time::currentTimeMillis();
    
    // Calculate initial angle from mouse position
    auto center = getLocalBounds().getCentre().toFloat();
    lastAngle_ = std::atan2(e.position.y - center.y, e.position.x - center.x);
    DBG("Mouse down at angle: " << lastAngle_);
    
    repaint();
}

void BraidyAudioProcessorEditor::BraidsEncoder::mouseDrag(const juce::MouseEvent& e) {
    if (!isPressed_) return;
    
    auto center = getLocalBounds().getCentre().toFloat();
    auto mousePos = e.position;
    auto currentAngle = std::atan2(mousePos.y - center.y, mousePos.x - center.x);
    
    auto angleDiff = currentAngle - lastAngle_;
    
    // Handle wrap-around
    if (angleDiff > juce::MathConstants<float>::pi)
        angleDiff -= juce::MathConstants<float>::twoPi;
    else if (angleDiff < -juce::MathConstants<float>::pi)
        angleDiff += juce::MathConstants<float>::twoPi;
    
    // Update visual angle
    angle_ += angleDiff;
    
    // Convert angle change to encoder steps with higher sensitivity
    int steps = static_cast<int>(angleDiff * 20.0f);  // Increased sensitivity
    if (steps != 0 && onValueChange) {
        DBG("=== ENCODER DRAG: steps = " << steps);
        onValueChange(steps);
    }
    
    lastAngle_ = currentAngle;
    repaint();
}

void BraidyAudioProcessorEditor::BraidsEncoder::mouseUp(const juce::MouseEvent& e) {
    if (!isPressed_) return;
    
    auto pressDuration = juce::Time::currentTimeMillis() - pressStartTime_;
    
    if (pressDuration > 500 && !longPressTriggered_) {
        // Long press
        longPressTriggered_ = true;
        if (onLongPress) onLongPress();
    } else if (pressDuration < 500) {
        // Short click
        if (onClick) onClick();
    }
    
    isPressed_ = false;
    repaint();
}

//==============================================================================
// BraidsKnob Implementation  
//==============================================================================
BraidyAudioProcessorEditor::BraidsKnob::BraidsKnob(bool isBipolar, uint32_t indicatorColor) 
    : isBipolar_(isBipolar), indicatorColor_(indicatorColor) {
    setOpaque(true);  // Make component opaque for proper rendering
    if (isBipolar_) {
        value_ = 0.5f;  // Center position for bipolar
    }
}

void BraidyAudioProcessorEditor::BraidsKnob::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();
    auto radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    
    // Knob body - lighter for better visibility
    juce::Colour knobColor(0xFF6A6A6A);  // Much lighter gray
    g.setColour(knobColor);
    g.fillEllipse(bounds.reduced(2));
    
    // Outer ring highlight - more prominent
    g.setColour(knobColor.brighter(0.5f));
    g.drawEllipse(bounds.reduced(2), 2.0f);
    
    // Inner ring (knob cap) - more visible
    g.setColour(knobColor.brighter(0.3f));
    g.fillEllipse(bounds.reduced(6));
    
    // Add center detail for depth
    g.setColour(knobColor.darker(0.2f));
    g.drawEllipse(bounds.reduced(8), 1.0f);
    
    // Position indicator - use custom color if specified, otherwise white
    juce::Colour indicatorColor = (indicatorColor_ != 0) ? juce::Colour(indicatorColor_) : juce::Colours::white;
    
    float angle;
    if (isBipolar_) {
        // Bipolar: -135° to +135° (270° total range)
        angle = (value_ - 0.5f) * juce::MathConstants<float>::pi * 1.5f;
    } else {
        // Unipolar: -135° to +135° (270° total range)  
        angle = (value_ * 1.5f - 0.75f) * juce::MathConstants<float>::pi;
    }
    
    g.setColour(indicatorColor);
    auto indicatorRadius = 2.0f;
    auto indicatorDistance = radius * 0.7f;
    auto indicatorX = center.x + std::cos(angle) * indicatorDistance;
    auto indicatorY = center.y + std::sin(angle) * indicatorDistance;
    g.fillEllipse(indicatorX - indicatorRadius, indicatorY - indicatorRadius, 
                  indicatorRadius * 2, indicatorRadius * 2);
    
    // Add colored ring around knob for special parameters
    if (indicatorColor_ != 0) {
        g.setColour(indicatorColor.withAlpha(0.4f));
        g.drawEllipse(bounds.reduced(1), 2.0f);
    }
    
    // Center dot for bipolar knobs
    if (isBipolar_) {
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillEllipse(center.x - 1, center.y - 1, 2, 2);
    }
}

void BraidyAudioProcessorEditor::BraidsKnob::mouseDown(const juce::MouseEvent& e) {
    dragStartY_ = e.position.y;
    dragStartValue_ = value_;
}

void BraidyAudioProcessorEditor::BraidsKnob::mouseDrag(const juce::MouseEvent& e) {
    auto dragDistance = dragStartY_ - e.position.y;
    auto sensitivity = 0.005f;  // Fine control
    
    value_ = juce::jlimit(0.0f, 1.0f, dragStartValue_ + dragDistance * sensitivity);
    
    if (onValueChange) {
        onValueChange(value_);
    }
    
    repaint();
}

void BraidyAudioProcessorEditor::BraidsKnob::setValue(float value) {
    value_ = juce::jlimit(0.0f, 1.0f, value);
    repaint();
}

//==============================================================================
// CvJack Implementation
//==============================================================================
BraidyAudioProcessorEditor::CvJack::CvJack(const juce::String& label, bool isOutput) 
    : label_(label), isOutput_(isOutput) {
    setOpaque(true);  // Make component opaque for proper rendering
    setSize(20, 30);  // Width for jack + label space
}

void BraidyAudioProcessorEditor::CvJack::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    auto jackBounds = bounds.removeFromTop(15);
    
    // Jack socket - more visible
    g.setColour(juce::Colour(0xFF3A3A3A));  // Lighter than pure black
    g.fillEllipse(jackBounds.reduced(2).toFloat());
    
    // Outer ring for visibility
    g.setColour(juce::Colour(0xFF7A7A7A));
    g.drawEllipse(jackBounds.reduced(2).toFloat(), 2.0f);
    
    // Inner ring
    g.setColour(juce::Colour(0xFF5A5A5A));
    g.drawEllipse(jackBounds.reduced(4).toFloat(), 1.5f);
    
    // Center hole with highlight
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillEllipse(jackBounds.reduced(jackBounds.getWidth() * 0.35f).toFloat());
    g.setColour(juce::Colour(0xFF4A4A4A));
    g.drawEllipse(jackBounds.reduced(jackBounds.getWidth() * 0.35f).toFloat(), 0.5f);
    
    // Label
    g.setColour(juce::Colour(BraidsColors::text));
    g.setFont(juce::Font("Arial", 8.0f, juce::Font::plain));
    g.drawText(label_, bounds, juce::Justification::centred);
}

//==============================================================================
// Simple OLED Display Implementation (inline to avoid dependencies)
//==============================================================================
class SimpleOLEDDisplay : public juce::Component {
public:
    SimpleOLEDDisplay() {
        setOpaque(true);  // Make component opaque for proper rendering
        setText("VOWL");
    }
    
    void setText(const juce::String& text) {
        if (displayText_ != text) {
            displayText_ = text.substring(0, 4).paddedRight(' ', 4);
            repaint();
        }
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Black OLED background with visible border
        g.setColour(juce::Colour(0xFF1A1A1A));  // Slightly lighter for visibility
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Outer border for visibility
        g.setColour(juce::Colour(0xFF5A5A5A));
        g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
        
        // Inner bezel
        g.setColour(juce::Colour(0xFF333333));
        g.drawRoundedRectangle(bounds.reduced(2.0f), 4.0f, 1.0f);
        
        // Bright green LED text
        g.setColour(juce::Colour(0xFF00FF40));  // Brighter green for better visibility
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), bounds.getHeight() * 0.65f, juce::Font::bold));
        g.drawText(displayText_, bounds, juce::Justification::centred);
        
        // Enhanced glow effect
        g.setColour(juce::Colour(0xFF00FF40).withAlpha(0.25f));
        g.drawText(displayText_, bounds.expanded(2), juce::Justification::centred);
    }
    
private:
    juce::String displayText_ = "----";
};

//==============================================================================
// Main Editor Implementation
//==============================================================================
BraidyAudioProcessorEditor::BraidyAudioProcessorEditor(BraidyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
    
    // Set size to match 16HP Eurorack module proportions (more compact)
    setSize(320, 480);  // Increased size for better component visibility
    
    DBG("=== Braidy Editor Constructor ===");
    DBG("Editor size set to: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    
    // Initialize display to show VOWL algorithm  
    displayMode_ = DisplayMode::Algorithm;
    currentAlgorithm_ = 22; // VOWL algorithm (index 22 in algorithmNames_ - "VOWL")
    
    setupComponents();
    updateDisplay();
    
    // Force initial layout and repaint
    resized();
    repaint();
    
    // Start timer for parameter updates (reduce frequency to avoid issues)
    startTimer(100);
    
    DBG("=== Constructor Complete ===");
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
    stopTimer();
}

void BraidyAudioProcessorEditor::setupComponents() {
    // Simple OLED Display (4-character green) - using inline class
    oledDisplay_ = std::make_unique<SimpleOLEDDisplay>();
    addAndMakeVisible(*oledDisplay_);
    oledDisplay_->setVisible(true);
    DBG("Created OLED Display");
    
    // Main EDIT encoder (large)
    editEncoder_ = std::make_unique<BraidsEncoder>();
    editEncoder_->onValueChange = [this](int delta) { handleEncoderRotation(delta); };
    editEncoder_->onClick = [this]() { handleEncoderClick(); };
    editEncoder_->onLongPress = [this]() { handleEncoderLongPress(); };
    addAndMakeVisible(*editEncoder_);
    editEncoder_->setVisible(true);
    DBG("Created Edit Encoder");
    
    // Top row knobs (FINE, COARSE, FM)
    fineKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar
    fineKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*fineKnob_);
    fineKnob_->setVisible(true);
    DBG("Created Fine Knob");
    
    coarseKnob_ = std::make_unique<BraidsKnob>();
    coarseKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*coarseKnob_);
    coarseKnob_->setVisible(true);
    DBG("Created Coarse Knob");
    
    fmKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar attenuverter
    fmKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*fmKnob_);
    fmKnob_->setVisible(true);
    DBG("Created FM Knob");
    
    // Main parameter knobs with colored indicators (matching hardware)
    timbreKnob_ = std::make_unique<BraidsKnob>(false, 0xFF00CCA3);  // Teal indicator
    timbreKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*timbreKnob_);
    timbreKnob_->setVisible(true);
    DBG("Created Timbre Knob");
    
    colorKnob_ = std::make_unique<BraidsKnob>(false, 0xFFE74C3C);  // Red indicator
    colorKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*colorKnob_);
    colorKnob_->setVisible(true);
    DBG("Created Color Knob");
    
    // Modulation attenuverter (center knob)
    timbreModKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar
    timbreModKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*timbreModKnob_);
    timbreModKnob_->setVisible(true);
    DBG("Created Modulation Knob");
    
    // CV Jacks (bottom row)
    const std::array<juce::String, 6> jackLabels = {"TRIG", "V/OCT", "FM", "TIMBRE", "COLOR", "OUT"};
    for (int i = 0; i < 6; ++i) {
        cvJacks_[i] = std::make_unique<CvJack>(jackLabels[i], i == 5);  // OUT is output
        addAndMakeVisible(*cvJacks_[i]);
        cvJacks_[i]->setVisible(true);
        DBG("Created CV Jack: " + jackLabels[i]);
    }
    
    DBG("=== All Components Created ===");
}

void BraidyAudioProcessorEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Draw authentic Braids panel background
    drawBraidsPanel(g);
    drawScrewHoles(g);
    
    // Panel title and subtitle (compact)
    auto titleBounds = bounds.removeFromTop(28);
    g.setColour(juce::Colour(BraidsColors::text));
    g.setFont(juce::Font("Arial", 12.0f, juce::Font::bold));
    g.drawText("Braids", titleBounds.removeFromTop(16), juce::Justification::centred);
    
    g.setFont(juce::Font("Arial", 8.0f, juce::Font::plain));
    g.drawText("macro oscillator", titleBounds, juce::Justification::centred);
    
    // Draw labels AFTER components are positioned (avoiding overlap)
    drawControlLabels(g);
    
    // LEDs for TIMBRE and COLOR (draw last to ensure visibility)
    drawParameterLeds(g);
    
    // Debug: Draw component outlines to verify positioning
    g.setColour(juce::Colours::yellow.withAlpha(0.3f));
    if (oledDisplay_) g.drawRect(oledDisplay_->getBounds(), 2);
    if (editEncoder_) g.drawRect(editEncoder_->getBounds(), 2);
    
    g.setColour(juce::Colours::blue.withAlpha(0.3f));
    if (fineKnob_) g.drawRect(fineKnob_->getBounds(), 1);
    if (coarseKnob_) g.drawRect(coarseKnob_->getBounds(), 1);
    if (fmKnob_) g.drawRect(fmKnob_->getBounds(), 1);
    
    g.setColour(juce::Colours::green.withAlpha(0.3f));
    if (timbreKnob_) g.drawRect(timbreKnob_->getBounds(), 1);
    if (timbreModKnob_) g.drawRect(timbreModKnob_->getBounds(), 1);
    if (colorKnob_) g.drawRect(colorKnob_->getBounds(), 1);
}

void BraidyAudioProcessorEditor::drawBraidsPanel(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Main panel background (light gray aluminum - matching hardware)
    juce::Colour panelColor(0xFFE8E8E8);  // Slightly warmer gray
    g.setColour(panelColor);
    g.fillRect(bounds);
    
    // Panel edges (subtle bevel effect)
    g.setColour(panelColor.brighter(0.15f));
    g.drawRect(bounds, 1);
    
    // Subtle brushed aluminum texture
    g.setColour(panelColor.darker(0.015f));
    for (int y = 0; y < bounds.getHeight(); y += 3) {
        g.drawHorizontalLine(y, 0, bounds.getWidth());
    }
    
    // Add subtle gradient for depth
    juce::ColourGradient gradient(panelColor.brighter(0.05f), 0, 0,
                                  panelColor.darker(0.05f), 0, bounds.getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds.reduced(1));
}

void BraidyAudioProcessorEditor::drawScrewHoles(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Eurorack mounting holes (top and bottom)
    g.setColour(juce::Colour(BraidsColors::jack));
    auto screwSize = 4;
    
    // Top holes
    g.fillEllipse(7, 5, screwSize, screwSize);
    g.fillEllipse(getWidth() - 11, 5, screwSize, screwSize);
    
    // Bottom holes  
    g.fillEllipse(7, getHeight() - 9, screwSize, screwSize);
    g.fillEllipse(getWidth() - 11, getHeight() - 9, screwSize, screwSize);
}

void BraidyAudioProcessorEditor::drawControlLabels(juce::Graphics& g) {
    g.setColour(juce::Colour(BraidsColors::text));
    g.setFont(juce::Font("Arial", 8.0f, juce::Font::plain));
    
    auto width = getWidth();
    auto knobSpacing = width / 3;
    
    // Display label
    if (oledDisplay_) {
        auto displayBounds = oledDisplay_->getBounds();
        g.drawText("MODEL", displayBounds.getX(), displayBounds.getBottom() + 2, 
                   displayBounds.getWidth(), 12, juce::Justification::centred);
    }
    
    // Encoder label
    if (editEncoder_) {
        auto encoderBounds = editEncoder_->getBounds();
        g.drawText("EDIT", encoderBounds.getX(), encoderBounds.getBottom() + 2, 
                   encoderBounds.getWidth(), 12, juce::Justification::centred);
    }
    
    // Top row labels (FINE, COARSE, FM)
    if (fineKnob_) {
        auto bounds = fineKnob_->getBounds();
        g.drawText("FINE", bounds.getX() - 5, bounds.getBottom() + 2, 
                   bounds.getWidth() + 10, 12, juce::Justification::centred);
    }
    if (coarseKnob_) {
        auto bounds = coarseKnob_->getBounds();
        g.drawText("COARSE", bounds.getX() - 5, bounds.getBottom() + 2, 
                   bounds.getWidth() + 10, 12, juce::Justification::centred);
    }
    if (fmKnob_) {
        auto bounds = fmKnob_->getBounds();
        g.drawText("FM", bounds.getX() - 5, bounds.getBottom() + 2, 
                   bounds.getWidth() + 10, 12, juce::Justification::centred);
    }
    
    // Main parameter labels (TIMBRE, MODULATION, COLOR)
    if (timbreKnob_) {
        auto bounds = timbreKnob_->getBounds();
        g.drawText("TIMBRE", bounds.getX() - 8, bounds.getBottom() + 2, 
                   bounds.getWidth() + 16, 10, juce::Justification::centred);
    }
    if (timbreModKnob_) {
        auto bounds = timbreModKnob_->getBounds();
        g.drawText("MODULATION", bounds.getX() - 12, bounds.getBottom() + 2, 
                   bounds.getWidth() + 24, 10, juce::Justification::centred);
    }
    if (colorKnob_) {
        auto bounds = colorKnob_->getBounds();
        g.drawText("COLOR", bounds.getX() - 8, bounds.getBottom() + 2, 
                   bounds.getWidth() + 16, 10, juce::Justification::centred);
    }
    
    // CV Jack labels
    if (cvJacks_[0]) {
        auto jackY = cvJacks_[0]->getBounds().getY();
        auto jackSpacing = width / 6;
        g.setFont(juce::Font("Arial", 7.0f, juce::Font::plain));
        
        const std::array<const char*, 6> labels = {"TRIG", "V/OCT", "FM", "TIMBRE", "COLOR", "OUT"};
        g.setFont(juce::Font("Arial", 6.5f, juce::Font::plain));  // Smaller font for compact layout
        for (int i = 0; i < 6; ++i) {
            auto x = static_cast<int>(jackSpacing * i);
            g.drawText(labels[i], x, jackY + 25, static_cast<int>(jackSpacing), 10, juce::Justification::centred);
        }
    }
}

void BraidyAudioProcessorEditor::drawParameterLeds(juce::Graphics& g) {
    // TIMBRE LED (teal/green - matches hardware)
    if (timbreKnob_) {
        auto timbreBounds = timbreKnob_->getBounds();
        auto ledBounds = juce::Rectangle<int>(5, 5).withCentre({timbreBounds.getCentreX(), timbreBounds.getY() - 10});
        
        auto brightness = timbreKnob_->getValue();
        auto ledColour = juce::Colour(0xFF00CCA3).withAlpha(0.4f + brightness * 0.6f);  // Teal color
        
        g.setColour(ledColour);
        g.fillEllipse(ledBounds.toFloat());
        
        // LED rim (brighter teal)
        g.setColour(juce::Colour(0xFF00CCA3));
        g.drawEllipse(ledBounds.toFloat(), 0.8f);
        
        // Inner glow effect
        g.setColour(ledColour.brighter(0.3f));
        g.fillEllipse(ledBounds.reduced(1).toFloat());
    }
    
    // COLOR LED (red - matches hardware) 
    if (colorKnob_) {
        auto colorBounds = colorKnob_->getBounds();
        auto ledBounds = juce::Rectangle<int>(5, 5).withCentre({colorBounds.getCentreX(), colorBounds.getY() - 10});
        
        auto brightness = colorKnob_->getValue();
        auto ledColour = juce::Colour(0xFFE74C3C).withAlpha(0.4f + brightness * 0.6f);  // Red color
        
        g.setColour(ledColour);
        g.fillEllipse(ledBounds.toFloat());
        
        // LED rim (brighter red)
        g.setColour(juce::Colour(0xFFE74C3C));
        g.drawEllipse(ledBounds.toFloat(), 0.8f);
        
        // Inner glow effect
        g.setColour(ledColour.brighter(0.3f));
        g.fillEllipse(ledBounds.reduced(1).toFloat());
    }
}

void BraidyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Title area with Braids branding (compact)
    auto titleArea = bounds.removeFromTop(28);
    
    // LED Display and EDIT encoder on same row (like hardware)
    auto displayEncoderArea = bounds.removeFromTop(60);
    auto displayWidth = 100;
    auto encoderSize = 50;
    auto gapBetween = 20;
    auto totalWidth = displayWidth + gapBetween + encoderSize;
    auto startX = (getWidth() - totalWidth) / 2;
    
    // OLED Display on left (more prominent positioning)
    if (oledDisplay_) {
        auto displayHeight = 30;
        auto displayY = titleArea.getBottom() + 10;
        oledDisplay_->setBounds(startX, displayY, displayWidth, displayHeight);
        oledDisplay_->setVisible(true);
        oledDisplay_->toFront(false);
    }
    
    // EDIT encoder on right (matching hardware position)
    if (editEncoder_) {
        auto encoderY = titleArea.getBottom() + 5;
        editEncoder_->setBounds(startX + displayWidth + gapBetween, encoderY, encoderSize, encoderSize);
        editEncoder_->setVisible(true);
        editEncoder_->toFront(false);
    }
    
    // Top row: FINE, COARSE, FM knobs (small knobs)
    auto topKnobY = titleArea.getBottom() + displayEncoderArea.getHeight() + 20;
    auto smallKnobSize = 35;
    auto knobSpacing = getWidth() / 3.0f;
    
    if (fineKnob_) {
        auto x = static_cast<int>(knobSpacing * 0 + (knobSpacing - smallKnobSize) / 2);
        fineKnob_->setBounds(x, topKnobY, smallKnobSize, smallKnobSize);
        fineKnob_->setVisible(true);
        fineKnob_->toFront(false);
    }
    if (coarseKnob_) {
        auto x = static_cast<int>(knobSpacing * 1 + (knobSpacing - smallKnobSize) / 2);
        coarseKnob_->setBounds(x, topKnobY, smallKnobSize, smallKnobSize);
        coarseKnob_->setVisible(true);
        coarseKnob_->toFront(false);
    }
    if (fmKnob_) {
        auto x = static_cast<int>(knobSpacing * 2 + (knobSpacing - smallKnobSize) / 2);
        fmKnob_->setBounds(x, topKnobY, smallKnobSize, smallKnobSize);
        fmKnob_->setVisible(true);
        fmKnob_->toFront(false);
    }
    
    // Main row: TIMBRE (left), MODULATION (center), COLOR (right)
    auto mainKnobY = topKnobY + smallKnobSize + 30;
    auto mainKnobSize = 45;  // Large knobs for TIMBRE and COLOR
    auto modKnobSize = 40;   // Slightly smaller for MODULATION
    
    // TIMBRE knob (left, large with teal indicator)
    if (timbreKnob_) {
        auto x = static_cast<int>(knobSpacing * 0 + (knobSpacing - mainKnobSize) / 2);
        timbreKnob_->setBounds(x, mainKnobY, mainKnobSize, mainKnobSize);
        timbreKnob_->setVisible(true);
        timbreKnob_->toFront(false);
    }
    
    // MODULATION knob in center (slightly smaller)
    if (timbreModKnob_) {
        auto x = static_cast<int>(knobSpacing * 1 + (knobSpacing - modKnobSize) / 2);
        auto adjustedY = mainKnobY + (mainKnobSize - modKnobSize) / 2;
        timbreModKnob_->setBounds(x, adjustedY, modKnobSize, modKnobSize);
        timbreModKnob_->setVisible(true);
        timbreModKnob_->toFront(false);
    }
    
    // COLOR knob (right, large with red indicator)
    if (colorKnob_) {
        auto x = static_cast<int>(knobSpacing * 2 + (knobSpacing - mainKnobSize) / 2);
        colorKnob_->setBounds(x, mainKnobY, mainKnobSize, mainKnobSize);
        colorKnob_->setVisible(true);
        colorKnob_->toFront(false);
    }
    
    // CV Jacks at bottom (6 jacks in a row)
    auto jackY = getHeight() - 50;
    auto jackWidth = 20;
    auto jackHeight = 25;
    auto jackSpacing = getWidth() / 6.0f;
    
    for (int i = 0; i < 6; ++i) {
        if (cvJacks_[i]) {
            auto x = static_cast<int>(jackSpacing * i + (jackSpacing - jackWidth) / 2);
            cvJacks_[i]->setBounds(x, jackY, jackWidth, jackHeight);
            cvJacks_[i]->setVisible(true);
            cvJacks_[i]->toFront(false);
        }
    }
    
    // Debug: Print bounds to verify layout
    DBG("=== Component Layout Debug ===");
    DBG("Editor size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    if (oledDisplay_) DBG("OLED Display: " + oledDisplay_->getBounds().toString());
    if (editEncoder_) DBG("Edit Encoder: " + editEncoder_->getBounds().toString());
    if (fineKnob_) DBG("Fine Knob: " + fineKnob_->getBounds().toString());
    if (coarseKnob_) DBG("Coarse Knob: " + coarseKnob_->getBounds().toString());
    if (fmKnob_) DBG("FM Knob: " + fmKnob_->getBounds().toString());
    if (timbreKnob_) DBG("Timbre Knob: " + timbreKnob_->getBounds().toString());
    if (timbreModKnob_) DBG("Timbre Mod Knob: " + timbreModKnob_->getBounds().toString());
    if (colorKnob_) DBG("Color Knob: " + colorKnob_->getBounds().toString());
}

//==============================================================================
// Parameter and Display Updates (simplified for initial version)
//==============================================================================
void BraidyAudioProcessorEditor::updateDisplay() {
    if (!oledDisplay_) {
        DBG("updateDisplay: oledDisplay_ is null!");
        return;
    }
    
    juce::String displayText;
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            displayText = algorithmNames_[currentAlgorithm_];
            static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
            // DBG("Display updated to algorithm: " + displayText);  // Commented to reduce console spam
            break;
            
        case DisplayMode::Value:
            // Show parameter value being edited
            if (inEditMode_) {
                displayText = juce::String(menuValue_).paddedLeft('0', 3);
                static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                DBG("Display updated to value: " + displayText);
            }
            break;
            
        case DisplayMode::Menu:
            if (currentMenuPage_ != MenuPage::None) {
                int pageIndex = static_cast<int>(currentMenuPage_) - 1;
                if (pageIndex >= 0 && pageIndex < menuPageNames_.size()) {
                    displayText = menuPageNames_[pageIndex];
                    static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                    DBG("Display updated to menu: " + displayText);
                }
            }
            break;
            
        case DisplayMode::Startup:
            displayText = "VOWL";  // Start with VOWL as shown in specs
            static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
            DBG("Display set to startup: " + displayText);
            // After startup delay, switch to algorithm display
            juce::Timer::callAfterDelay(1000, [this]() {
                displayMode_ = DisplayMode::Algorithm;
                currentAlgorithm_ = 23; // VOWL algorithm index
                updateDisplay();
            });
            break;
    }
    
    // Force display repaint
    if (oledDisplay_) {
        oledDisplay_->repaint();
    }
}

void BraidyAudioProcessorEditor::updateParameterValues() {
    // Simplified - no parameter connections to avoid crashes
    // This would be implemented once the parameter system is working
}

void BraidyAudioProcessorEditor::updateParameterFromKnob(BraidsKnob* knob, const juce::String& paramId) {
    // Simplified - no parameter connections to avoid crashes
    // This would be implemented once the parameter system is working
}

//==============================================================================
// Encoder Handling
//==============================================================================
void BraidyAudioProcessorEditor::handleEncoderRotation(int delta) {
    DBG("=== ENCODER ROTATION DEBUG ===");
    DBG("Delta: " + juce::String(delta));
    DBG("Display Mode: " + juce::String(static_cast<int>(displayMode_)));
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            {
                int oldAlgorithm = currentAlgorithm_;
                // Navigate algorithms
                currentAlgorithm_ = juce::jlimit(0, 47, currentAlgorithm_ + delta);
                
                DBG("Algorithm changed from " + juce::String(oldAlgorithm) + " to " + juce::String(currentAlgorithm_));
                DBG("Algorithm name: " + juce::String(algorithmNames_[currentAlgorithm_]));
                
                // Update the audio parameter
                updateAlgorithmParameter();
                break;
            }
            
        case DisplayMode::Menu:
            if (inEditMode_) {
                editMenuValue(delta);
            } else {
                navigateMenu(delta);
            }
            break;
            
        case DisplayMode::Value:
            if (inEditMode_) {
                menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
            }
            break;
            
        default:
            break;
    }
    
    updateDisplay();
}

void BraidyAudioProcessorEditor::handleEncoderClick() {
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            // Short click in algorithm mode does nothing (authentic behavior)
            break;
            
        case DisplayMode::Menu:
            // Toggle edit mode
            inEditMode_ = !inEditMode_;
            if (!inEditMode_) {
                // Apply the edited value
                applyMenuValue();
            }
            break;
            
        case DisplayMode::Value:
            // Exit value display mode
            displayMode_ = DisplayMode::Algorithm;
            break;
            
        default:
            break;
    }
    
    updateDisplay();
}

void BraidyAudioProcessorEditor::handleEncoderLongPress() {
    if (displayMode_ == DisplayMode::Algorithm) {
        // Enter settings menu
        enterMenuMode();
    } else if (displayMode_ == DisplayMode::Menu) {
        // Exit settings menu
        exitMenuMode();
    }
}

//==============================================================================
// Menu System  
//==============================================================================
void BraidyAudioProcessorEditor::enterMenuMode() {
    displayMode_ = DisplayMode::Menu;
    currentMenuPage_ = MenuPage::META;  // Start with first menu page
    inEditMode_ = false;
    updateDisplay();
}

void BraidyAudioProcessorEditor::exitMenuMode() {
    displayMode_ = DisplayMode::Algorithm;
    currentMenuPage_ = MenuPage::None;
    inEditMode_ = false;
    updateDisplay();
}

void BraidyAudioProcessorEditor::navigateMenu(int delta) {
    int currentPage = static_cast<int>(currentMenuPage_);
    int newPage = juce::jlimit(1, 20, currentPage + delta);  // 20 menu pages (1-20)
    currentMenuPage_ = static_cast<MenuPage>(newPage);
}

void BraidyAudioProcessorEditor::editMenuValue(int delta) {
    // Menu value editing logic would go here
    // This would depend on which menu page is active
    menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
}

void BraidyAudioProcessorEditor::applyMenuValue() {
    // Apply the edited menu value to the appropriate parameter
    // This would depend on the current menu page
    // For now, this is a placeholder
}

void BraidyAudioProcessorEditor::updateAlgorithmParameter() {
    DBG("=== UPDATE ALGORITHM PARAMETER DEBUG ===");
    
    // Update the SHAPE parameter in APVTS based on current algorithm
    if (auto* shapeParam = processorRef.getAPVTS().getParameter("alg")) {
        // Convert algorithm index (0-47) to normalized value (0.0-1.0)
        float normalizedValue = static_cast<float>(currentAlgorithm_) / 47.0f;
        
        DBG("Found 'alg' parameter in APVTS");
        DBG("Current algorithm index: " + juce::String(currentAlgorithm_));
        DBG("Normalized value: " + juce::String(normalizedValue, 4));
        
        float oldValue = shapeParam->getValue();
        shapeParam->setValueNotifyingHost(normalizedValue);
        float newValue = shapeParam->getValue();
        
        DBG("Parameter value changed from " + juce::String(oldValue, 4) + " to " + juce::String(newValue, 4));
        DBG("Algorithm: " + juce::String(algorithmNames_[currentAlgorithm_]));
    } else {
        DBG("ERROR: 'alg' parameter NOT FOUND in APVTS!");
        
        // List all available parameters
        DBG("Available parameters:");
        auto& apvts = processorRef.getAPVTS();
        for (auto* param : processorRef.getParameters()) {
            if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param)) {
                DBG("  - " + rangedParam->getParameterID());
            }
        }
    }
}

//==============================================================================
// JUCE Component Callbacks (unused but required)
//==============================================================================
void BraidyAudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    // Not used - we use custom knobs
}

void BraidyAudioProcessorEditor::buttonClicked(juce::Button* button) {
    // Not used - we use custom encoder
}

void BraidyAudioProcessorEditor::timerCallback() {
    // Update parameter values from processor
    updateParameterValues();
    
    // Only update display if algorithm changed (not every timer tick)
    static int lastAlgorithm = -1;
    if (currentAlgorithm_ != lastAlgorithm) {
        lastAlgorithm = currentAlgorithm_;
        if (displayMode_ == DisplayMode::Algorithm) {
            updateDisplay();
        }
    }
    
    // Repaint LEDs to show parameter activity (less frequent)
    static int repaintCounter = 0;
    if (++repaintCounter >= 5) {  // Only repaint every 5th tick (500ms)
        repaintCounter = 0;
        repaint();
    }
}