/*
  ==============================================================================

    Complete Braids UI Implementation
    Matches hardware exactly with all components visible

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// LED Display Component (Prominent Green OLED)
//==============================================================================
class BraidsLEDDisplay : public juce::Component {
public:
    BraidsLEDDisplay() {
        setText("CSAW");
    }
    
    void setText(const juce::String& text) {
        displayText_ = text.substring(0, 4).toUpperCase().paddedRight(' ', 4);
        repaint();
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Black background with rounded corners
        g.setColour(juce::Colour(0xFF000000));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Dark green inner glow
        g.setColour(juce::Colour(0xFF003300));
        g.fillRoundedRectangle(bounds.reduced(2), 3.0f);
        
        // Black center
        g.setColour(juce::Colour(0xFF000000));
        g.fillRoundedRectangle(bounds.reduced(3), 2.0f);
        
        // Bright green LED text
        g.setColour(juce::Colour(0xFF00FF00));
        g.setFont(juce::Font("Courier New", bounds.getHeight() * 0.6f, juce::Font::bold));
        g.drawText(displayText_, bounds, juce::Justification::centred);
        
        // Extra glow layer
        g.setColour(juce::Colour(0xFF00FF00).withAlpha(0.3f));
        g.setFont(juce::Font("Courier New", bounds.getHeight() * 0.6f, juce::Font::bold));
        g.drawText(displayText_, bounds.expanded(1), juce::Justification::centred);
    }
    
private:
    juce::String displayText_ = "----";
};

//==============================================================================
// Complete Editor Implementation
//==============================================================================
BraidyAudioProcessorEditor::BraidyAudioProcessorEditor(BraidyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
    
    // Compact Eurorack size
    setSize(280, 380);
    
    // Create LED Display (most prominent component)
    ledDisplay_ = std::make_unique<BraidsLEDDisplay>();
    addAndMakeVisible(ledDisplay_.get());
    
    // Main EDIT encoder
    editEncoder_ = std::make_unique<BraidsEncoder>();
    editEncoder_->onValueChange = [this](int delta) { 
        currentAlgorithm_ = (currentAlgorithm_ + delta + 48) % 48;
        updateDisplay();
    };
    editEncoder_->onClick = [this]() { handleEncoderClick(); };
    editEncoder_->onLongPress = [this]() { handleEncoderLongPress(); };
    addAndMakeVisible(editEncoder_.get());
    
    // Top row knobs (FINE, COARSE, FM)
    fineKnob_ = std::make_unique<BraidsKnob>(true);
    addAndMakeVisible(fineKnob_.get());
    
    coarseKnob_ = std::make_unique<BraidsKnob>();
    addAndMakeVisible(coarseKnob_.get());
    
    fmKnob_ = std::make_unique<BraidsKnob>(true);
    addAndMakeVisible(fmKnob_.get());
    
    // Main parameter knobs with colored indicators
    timbreKnob_ = std::make_unique<BraidsKnob>(false, 0xFF00CCA3); // Teal
    addAndMakeVisible(timbreKnob_.get());
    
    colorKnob_ = std::make_unique<BraidsKnob>(false, 0xFFE74C3C);  // Red
    addAndMakeVisible(colorKnob_.get());
    
    // Modulation knob (center, bipolar)
    modulationKnob_ = std::make_unique<BraidsKnob>(true);
    addAndMakeVisible(modulationKnob_.get());
    
    // CV Jacks
    for (int i = 0; i < 6; ++i) {
        const std::array<juce::String, 6> labels = {"TRIG", "V/OCT", "FM", "TIMBRE", "COLOR", "OUT"};
        cvJacks_[i] = std::make_unique<CvJack>(labels[i], i == 5);
        addAndMakeVisible(cvJacks_[i].get());
    }
    
    updateDisplay();
    startTimer(100);
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
    stopTimer();
}

void BraidyAudioProcessorEditor::paint(juce::Graphics& g) {
    // Eurorack panel background
    g.fillAll(juce::Colour(0xFFE8E8E8));
    
    // Panel edges
    g.setColour(juce::Colour(0xFFD0D0D0));
    g.drawRect(getLocalBounds(), 2);
    
    // Screw holes
    g.setColour(juce::Colour(0xFF303030));
    g.fillEllipse(5, 5, 6, 6);
    g.fillEllipse(getWidth() - 11, 5, 6, 6);
    g.fillEllipse(5, getHeight() - 11, 6, 6);
    g.fillEllipse(getWidth() - 11, getHeight() - 11, 6, 6);
    
    // Labels
    g.setColour(juce::Colour(0xFF000000));
    g.setFont(juce::Font("Arial", 9.0f, juce::Font::plain));
    
    // LED Display label
    if (ledDisplay_) {
        auto bounds = ledDisplay_->getBounds();
        g.drawText("MODEL", bounds.getX(), bounds.getBottom() + 1, 
                   bounds.getWidth(), 10, juce::Justification::centred);
    }
    
    // EDIT encoder label
    if (editEncoder_) {
        auto bounds = editEncoder_->getBounds();
        g.drawText("EDIT", bounds.getX(), bounds.getBottom() + 1, 
                   bounds.getWidth(), 10, juce::Justification::centred);
    }
    
    // Knob labels
    g.setFont(juce::Font("Arial", 8.0f, juce::Font::plain));
    
    if (fineKnob_) {
        auto b = fineKnob_->getBounds();
        g.drawText("FINE", b.getX() - 5, b.getBottom(), b.getWidth() + 10, 10, juce::Justification::centred);
    }
    if (coarseKnob_) {
        auto b = coarseKnob_->getBounds();
        g.drawText("COARSE", b.getX() - 5, b.getBottom(), b.getWidth() + 10, 10, juce::Justification::centred);
    }
    if (fmKnob_) {
        auto b = fmKnob_->getBounds();
        g.drawText("FM", b.getX() - 5, b.getBottom(), b.getWidth() + 10, 10, juce::Justification::centred);
    }
    
    // Draw colored LEDs above TIMBRE and COLOR knobs
    if (timbreKnob_) {
        auto b = timbreKnob_->getBounds();
        auto ledY = b.getY() - 12;
        // Teal LED
        g.setColour(juce::Colour(0xFF00CCA3).withAlpha(0.3f + timbreKnob_->getValue() * 0.7f));
        g.fillEllipse(b.getCentreX() - 4.0f, ledY, 8.0f, 8.0f);
        g.setColour(juce::Colour(0xFF00CCA3));
        g.drawEllipse(b.getCentreX() - 4.0f, ledY, 8.0f, 8.0f, 1.0f);
        
        g.setColour(juce::Colour(0xFF000000));
        g.drawText("TIMBRE", b.getX() - 10, b.getBottom(), b.getWidth() + 20, 10, juce::Justification::centred);
    }
    
    if (modulationKnob_) {
        auto b = modulationKnob_->getBounds();
        g.drawText("MODULATION", b.getX() - 15, b.getBottom(), b.getWidth() + 30, 10, juce::Justification::centred);
    }
    
    if (colorKnob_) {
        auto b = colorKnob_->getBounds();
        auto ledY = b.getY() - 12;
        // Red LED
        g.setColour(juce::Colour(0xFFE74C3C).withAlpha(0.3f + colorKnob_->getValue() * 0.7f));
        g.fillEllipse(b.getCentreX() - 4.0f, ledY, 8.0f, 8.0f);
        g.setColour(juce::Colour(0xFFE74C3C));
        g.drawEllipse(b.getCentreX() - 4.0f, ledY, 8.0f, 8.0f, 1.0f);
        
        g.setColour(juce::Colour(0xFF000000));
        g.drawText("COLOR", b.getX() - 10, b.getBottom(), b.getWidth() + 20, 10, juce::Justification::centred);
    }
}

void BraidyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Top section - LED Display and EDIT encoder (most prominent)
    auto topSection = bounds.removeFromTop(80);
    topSection.removeFromTop(20); // Space from top
    
    // LED Display (left side, large and prominent)
    auto displayBounds = topSection.removeFromLeft(120);
    displayBounds = displayBounds.withSizeKeepingCentre(100, 35);
    ledDisplay_->setBounds(displayBounds);
    
    // EDIT encoder (right side)
    auto encoderBounds = topSection.withSizeKeepingCentre(50, 50);
    editEncoder_->setBounds(encoderBounds);
    
    bounds.removeFromTop(20); // Space after display row
    
    // Top row knobs (FINE, COARSE, FM)
    auto topKnobRow = bounds.removeFromTop(50);
    auto knobWidth = 32;
    auto spacing = getWidth() / 3;
    
    fineKnob_->setBounds(spacing / 2 - knobWidth / 2, topKnobRow.getY(), knobWidth, knobWidth);
    coarseKnob_->setBounds(spacing + spacing / 2 - knobWidth / 2, topKnobRow.getY(), knobWidth, knobWidth);
    fmKnob_->setBounds(spacing * 2 + spacing / 2 - knobWidth / 2, topKnobRow.getY(), knobWidth, knobWidth);
    
    bounds.removeFromTop(30); // Space between rows
    
    // Main parameter knobs (TIMBRE, MODULATION, COLOR)
    auto mainKnobRow = bounds.removeFromTop(60);
    auto mainKnobSize = 42;
    
    // TIMBRE on left
    timbreKnob_->setBounds(spacing / 2 - mainKnobSize / 2, mainKnobRow.getY(), mainKnobSize, mainKnobSize);
    
    // MODULATION in center (slightly smaller)
    auto modKnobSize = 36;
    modulationKnob_->setBounds(spacing + spacing / 2 - modKnobSize / 2, 
                               mainKnobRow.getY() + 3, modKnobSize, modKnobSize);
    
    // COLOR on right
    colorKnob_->setBounds(spacing * 2 + spacing / 2 - mainKnobSize / 2, mainKnobRow.getY(), mainKnobSize, mainKnobSize);
    
    bounds.removeFromTop(30); // Space before jacks
    
    // CV Jacks at bottom
    auto jackRow = bounds.removeFromTop(40);
    auto jackSpacing = getWidth() / 6;
    for (int i = 0; i < 6; ++i) {
        cvJacks_[i]->setBounds(jackSpacing * i + (jackSpacing - 25) / 2, jackRow.getY(), 25, 35);
    }
}

void BraidyAudioProcessorEditor::updateDisplay() {
    if (ledDisplay_) {
        auto* display = static_cast<BraidsLEDDisplay*>(ledDisplay_.get());
        if (currentAlgorithm_ >= 0 && currentAlgorithm_ < algorithmNames_.size()) {
            display->setText(algorithmNames_[currentAlgorithm_]);
        }
    }
}

void BraidyAudioProcessorEditor::timerCallback() {
    // Update parameter values from processor if needed
}

// Stub implementations for missing methods
void BraidyAudioProcessorEditor::handleEncoderClick() {
    // Toggle edit mode
}

void BraidyAudioProcessorEditor::handleEncoderLongPress() {
    // Enter/exit menu
}

// Algorithm names
const std::array<const char*, 48> BraidyAudioProcessorEditor::algorithmNames_ = {
    "CSAW", "MORPH", "SAW^", "FOLD", "BUZZ", "SQR^", "RING", "SYN^", "SYN4",
    "WAV^", "WAV4", "WMAP", "WLIN", "WTX4", "NOIS", "TWNQ", "CLKN",
    "VOSM", "VOWL", "FOF", "HARM", "CHMB", "DIGI", "CHIP", "GRAIN", "CLOUD", "PRTC",
    "FM", "FBFM", "WTFM", "PLUK", "BOWD", "BLOW", "FLUT", "BELL", "DRUM",
    "KICK", "CYMB", "SNAR", "WTBL", "MAZE", "RFRC", "QPSK", "ZLPF", "ZPDF",
    "WTFX", "CLOU", "PRTC"
};