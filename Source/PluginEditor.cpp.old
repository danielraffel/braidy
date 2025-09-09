#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>
#include <set>

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
    "PART", "DIGI", "QPSK"                           // Final algorithms (45-47)
};

// Exact menu page names from Braids (4-character display)
const std::array<const char*, 24> BraidyAudioProcessorEditor::menuPageNames_ = {
    "WAVE", "META", "BITS", "RATE", "BRIG",          // 0-4 (WAVE added as first option)
    "TSRC", "TDLY", "|\\ATT", "|\\DEC", "|\\FM",     // 5-9
    "|\\TIM", "|\\COL", "|\\VCA", "RANG", "OCTV",    // 10-14
    "QNTZ", "ROOT", "FLAT", "DRFT", "SIGN",          // 15-19
    "CV_T", "MARQ", "CAL>", "VERS"                   // 20-23
};

//==============================================================================
// BraidsEncoder Implementation
//==============================================================================
BraidyAudioProcessorEditor::BraidsEncoder::BraidsEncoder() {
    setSize(60, 60);  // Large encoder like on Braids
    setInterceptsMouseClicks(true, false);  // Accept mouse clicks but don't intercept children
    setWantsKeyboardFocus(false);  // Encoder doesn't need focus, parent editor handles it
    DBG("[ENCODER] BraidsEncoder constructed");
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
    
    // Convert angle change to encoder steps
    // Use higher sensitivity and ensure we capture small movements
    float sensitivity = 12.0f;  // Adjusted for better response
    int steps = 0;
    
    // Accumulate fractional angle changes to avoid dead zones
    static float accumulatedAngle = 0.0f;
    accumulatedAngle += angleDiff * sensitivity;
    
    // Convert accumulated angle to steps
    if (std::abs(accumulatedAngle) >= 1.0f) {
        steps = static_cast<int>(accumulatedAngle);
        accumulatedAngle -= steps;  // Keep fractional part
    }
    
    if (steps != 0) {
        DBG("=== ENCODER DRAG: steps = " << steps);
        
        // Reset press start time and mark rotation to prevent long press
        pressStartTime_ = juce::Time::currentTimeMillis();
        longPressTriggered_ = true;
        
        if (onValueChange) {
            onValueChange(steps);
        }
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
    setOpaque(false);  // Make transparent to avoid gray background
    if (isBipolar_) {
        value_ = 0.5f;  // Center position for bipolar
        defaultValue_ = 0.5f;
    } else {
        value_ = 0.0f;  // Start at minimum for unipolar
        defaultValue_ = 0.0f;
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
        // Bipolar: 12 o'clock is center (0.5), -135° to +135° from top
        // Subtract pi/2 to start at top (12 o'clock) instead of right (3 o'clock)
        angle = (value_ - 0.5f) * juce::MathConstants<float>::pi * 1.5f - juce::MathConstants<float>::halfPi;
    } else {
        // Unipolar: starts at -135° from top (about 7 o'clock), ends at +135° (about 5 o'clock)
        angle = (value_ * 1.5f - 0.75f) * juce::MathConstants<float>::pi - juce::MathConstants<float>::halfPi;
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

void BraidyAudioProcessorEditor::BraidsKnob::mouseDoubleClick(const juce::MouseEvent& e) {
    // Reset to default value on double-click
    resetToDefault();
}

void BraidyAudioProcessorEditor::BraidsKnob::resetToDefault() {
    value_ = defaultValue_;
    if (onValueChange) {
        onValueChange(value_);
    }
    repaint();
}

//==============================================================================
// CvJack Implementation
//==============================================================================
BraidyAudioProcessorEditor::CvJack::CvJack(const juce::String& label, bool isOutput) 
    : label_(label), isOutput_(isOutput) {
    setOpaque(false);  // Make transparent to avoid gray background
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
    
    // Enable debug logging to Documents folder for troubleshooting
    auto documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto logFile = documentsDir.getChildFile("braidy_debug.log");
    
    // Create a file logger - must store the pointer
    fileLogger_ = std::unique_ptr<juce::FileLogger>(
        juce::FileLogger::createDateStampedLogger("braidy", "debug_", ".log", "=== Braidy Debug Log ===")
    );
    
    if (fileLogger_) {
        juce::Logger::setCurrentLogger(fileLogger_.get());
        
        // Force immediate write by logging directly
        fileLogger_->logMessage("[STARTUP] Debug logging enabled to: " + fileLogger_->getLogFile().getFullPathName());
        fileLogger_->logMessage("[STARTUP] Plugin version: " + juce::String(JucePlugin_VersionString));
        fileLogger_->logMessage("[STARTUP] Build type: DEBUG");
        fileLogger_->logMessage("[STARTUP] Initial display mode: Algorithm");
        fileLogger_->logMessage("[STARTUP] Initial algorithm: CSAW (0)");
    } else {
        // Fallback to console if file logging fails
        juce::Logger::setCurrentLogger(nullptr);
        DBG("[STARTUP] WARNING: Could not create log file, using console output");
    }
    
    // Set size to match 16HP Eurorack module proportions (more compact)
    setSize(320, 480);  // Increased size for better component visibility
    
    // Enable keyboard input for computer keyboard to MIDI functionality
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();  // Actually grab focus so keyboard works
    
    DBG("[STARTUP] Editor size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    DBG("[STARTUP] Keyboard focus enabled");
    
    // Initialize display to show algorithm (not menu)  
    displayMode_ = DisplayMode::Algorithm;
    currentAlgorithm_ = 0; // Start with CSAW (index 0)
    currentMenuPage_ = MenuPage::None;  // No menu page selected
    inEditMode_ = false;  // Not editing any value
    
    // Create modulation overlay (initially hidden)
    modulationOverlay_ = std::make_unique<braidy::ModulationSettingsOverlay>(processorRef.getModulationMatrix());
    modulationOverlay_->setVisible(false);
    addChildComponent(modulationOverlay_.get());
    
    // Set up overlay callbacks
    modulationOverlay_->onClose = [this]() {
        modulationOverlay_->hideOverlay();
    };
    
    modulationOverlay_->onMetaModeChanged = [this](bool enabled) {
        processorRef.setMetaModeEnabled(enabled);
    };
    
    modulationOverlay_->onQuantizerChanged = [this](bool enabled) {
        processorRef.setQuantizerEnabled(enabled);
    };
    
    modulationOverlay_->onBitCrusherChanged = [this](bool enabled) {
        processorRef.setBitCrusherEnabled(enabled);
    };
    
    DBG("[STARTUP] Initial algorithm: [" + juce::String(currentAlgorithm_) + "] " + juce::String(algorithmNames_[currentAlgorithm_]));
    
    setupComponents();
    updateDisplay();
    
    // Force initial layout and repaint
    resized();
    repaint();
    
    // Initialize computer keyboard to MIDI mapping
    initializeKeyboardMapping();
    
    // CRITICAL: Ensure this editor has keyboard focus after all components are created
    grabKeyboardFocus();
    DBG("[FOCUS] Editor grabbed keyboard focus after component setup");
    
    // Verify focus state
    if (hasKeyboardFocus(true)) {
        DBG("[FOCUS] SUCCESS: Editor has keyboard focus");
    } else {
        DBG("[FOCUS] WARNING: Editor does not have keyboard focus");
    }
    
    // Start timer for parameter updates (reduce frequency to avoid issues)
    startTimer(100);
    
    DBG("[STARTUP] Constructor complete with MIDI keyboard support");
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
    stopTimer();
    
    // Clean up logger
    DBG("[SHUTDOWN] Editor closing");
    if (fileLogger_) {
        juce::Logger::setCurrentLogger(nullptr);
        fileLogger_.reset();
    }
}

void BraidyAudioProcessorEditor::setupComponents() {
    // Simple OLED Display (4-character green) - using inline class
    oledDisplay_ = std::make_unique<SimpleOLEDDisplay>();
    oledDisplay_->setWantsKeyboardFocus(false);  // Prevent display from stealing focus
    addAndMakeVisible(*oledDisplay_);
    oledDisplay_->setVisible(true);
    DBG("Created OLED Display");
    
    // Main EDIT encoder (large)
    editEncoder_ = std::make_unique<BraidsEncoder>();
    editEncoder_->setWantsKeyboardFocus(false);  // Ensure encoder doesn't steal focus
    editEncoder_->onValueChange = [this](int delta) { handleEncoderRotation(delta); };
    editEncoder_->onClick = [this]() { handleEncoderClick(); };
    editEncoder_->onLongPress = [this]() { handleEncoderLongPress(); };
    addAndMakeVisible(*editEncoder_);
    editEncoder_->setVisible(true);
    DBG("Created Edit Encoder");
    
    // Top row knobs (FINE, COARSE, FM)
    fineKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar
    fineKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    fineKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*fineKnob_);
    fineKnob_->setVisible(true);
    DBG("Created Fine Knob");
    
    coarseKnob_ = std::make_unique<BraidsKnob>();
    coarseKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    coarseKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*coarseKnob_);
    coarseKnob_->setVisible(true);
    DBG("Created Coarse Knob");
    
    fmKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar attenuverter
    fmKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    fmKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*fmKnob_);
    fmKnob_->setVisible(true);
    DBG("Created FM Knob");
    
    // Main parameter knobs with colored indicators (matching hardware)
    timbreKnob_ = std::make_unique<BraidsKnob>(false, 0xFF00CCA3);  // Teal indicator
    timbreKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    timbreKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*timbreKnob_);
    timbreKnob_->setVisible(true);
    DBG("Created Timbre Knob");
    
    colorKnob_ = std::make_unique<BraidsKnob>(false, 0xFFE74C3C);  // Red indicator
    colorKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    colorKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*colorKnob_);
    colorKnob_->setVisible(true);
    DBG("Created Color Knob");
    
    // Modulation attenuverter (center knob)
    timbreModKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar
    timbreModKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    timbreModKnob_->onValueChange = [this](float value) { /* Parameter handling */ };
    addAndMakeVisible(*timbreModKnob_);
    timbreModKnob_->setVisible(true);
    DBG("Created Modulation Knob");
    
    // CV Jacks (bottom row)
    const std::array<juce::String, 6> jackLabels = {"TRIG", "V/OCT", "FM", "TIMBRE", "COLOR", "OUT"};
    for (int i = 0; i < 6; ++i) {
        cvJacks_[i] = std::make_unique<CvJack>(jackLabels[i], i == 5);  // OUT is output
        cvJacks_[i]->setWantsKeyboardFocus(false);  // Prevent jacks from stealing focus
        addAndMakeVisible(*cvJacks_[i]);
        cvJacks_[i]->setVisible(true);
        DBG("Created CV Jack: " + jackLabels[i]);
    }
    
    // MOD button (hardware-style button for accessing modulation settings)
    settingsButton_ = std::make_unique<juce::TextButton>("MOD");
    settingsButton_->setButtonText("MOD");
    settingsButton_->setWantsKeyboardFocus(false);  // Prevent button from stealing focus
    settingsButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4A4A4A));
    settingsButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF5A5A5A));
    settingsButton_->setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE8E8E8));
    settingsButton_->setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF00CCA3));
    settingsButton_->addListener(this);
    addAndMakeVisible(*settingsButton_);
    DBG("Created MOD Button");
    
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
    auto knobSize = 42;  // Uniform size for all knobs
    auto knobSpacing = getWidth() / 3.0f;
    
    if (fineKnob_) {
        auto x = static_cast<int>(knobSpacing * 0 + (knobSpacing - knobSize) / 2);
        fineKnob_->setBounds(x, topKnobY, knobSize, knobSize);
        fineKnob_->setVisible(true);
        fineKnob_->toFront(false);
    }
    if (coarseKnob_) {
        auto x = static_cast<int>(knobSpacing * 1 + (knobSpacing - knobSize) / 2);
        coarseKnob_->setBounds(x, topKnobY, knobSize, knobSize);
        coarseKnob_->setVisible(true);
        coarseKnob_->toFront(false);
    }
    if (fmKnob_) {
        auto x = static_cast<int>(knobSpacing * 2 + (knobSpacing - knobSize) / 2);
        fmKnob_->setBounds(x, topKnobY, knobSize, knobSize);
        fmKnob_->setVisible(true);
        fmKnob_->toFront(false);
    }
    
    // Main row: TIMBRE (left), MODULATION (center), COLOR (right)
    auto mainKnobY = topKnobY + knobSize + 30;
    
    // TIMBRE knob (left, large with teal indicator)
    if (timbreKnob_) {
        auto x = static_cast<int>(knobSpacing * 0 + (knobSpacing - knobSize) / 2);
        timbreKnob_->setBounds(x, mainKnobY, knobSize, knobSize);
        timbreKnob_->setVisible(true);
        timbreKnob_->toFront(false);
    }
    
    // MODULATION knob in center (slightly smaller)
    if (timbreModKnob_) {
        auto x = static_cast<int>(knobSpacing * 1 + (knobSpacing - knobSize) / 2);
        timbreModKnob_->setBounds(x, mainKnobY, knobSize, knobSize);
        timbreModKnob_->setVisible(true);
        timbreModKnob_->toFront(false);
    }
    
    // COLOR knob (right, large with red indicator)
    if (colorKnob_) {
        auto x = static_cast<int>(knobSpacing * 2 + (knobSpacing - knobSize) / 2);
        colorKnob_->setBounds(x, mainKnobY, knobSize, knobSize);
        colorKnob_->setVisible(true);
        colorKnob_->toFront(false);
    }
    
    // MOD button (near display area - prominent position)
    if (settingsButton_) {
        // Position MOD button to the right of the encoder
        auto modX = startX + displayWidth + gapBetween + encoderSize + 10;
        auto modY = titleArea.getBottom() + 15;
        settingsButton_->setBounds(modX, modY, 35, 30);
        settingsButton_->setVisible(true);
        settingsButton_->toFront(false);
    }
    
    // Modulation overlay (full screen when visible)
    if (modulationOverlay_) {
        modulationOverlay_->setBounds(0, 0, getWidth(), getHeight());
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
        if (fileLogger_) {
            fileLogger_->logMessage("updateDisplay: oledDisplay_ is null!");
        }
        return;
    }
    
    juce::String displayText;
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            // Ensure we don't access out of bounds (limit to 0-47 for all algorithms)
            if (currentAlgorithm_ >= 0 && currentAlgorithm_ < 48) {
                displayText = algorithmNames_[currentAlgorithm_];
                static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                if (fileLogger_) {
                    fileLogger_->logMessage("[DISPLAY] Showing Algorithm [" + juce::String(currentAlgorithm_) + 
                        "]: " + displayText);
                }
            } else {
                if (fileLogger_) {
                    fileLogger_->logMessage("[DISPLAY] ERROR: Algorithm index out of bounds: " + 
                        juce::String(currentAlgorithm_));
                }
            }
            break;
            
        case DisplayMode::Value:
            // Show parameter value being edited with appropriate format
            if (inEditMode_) {
                displayText = getFormattedMenuValue();
                static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                
                if (fileLogger_) {
                    fileLogger_->logMessage("[DISPLAY] Value mode showing: " + displayText);
                }
            }
            break;
            
        case DisplayMode::Menu:
            {
                if (fileLogger_) {
                    fileLogger_->logMessage("[MENU DISPLAY] currentMenuPage_: " + 
                        juce::String(static_cast<int>(currentMenuPage_)) + 
                        " (0=None, 1=META, 2=BITS, etc.)");
                }
                
                if (currentMenuPage_ != MenuPage::None) {
                    int pageIndex = static_cast<int>(currentMenuPage_) - 1;
                    if (pageIndex >= 0 && pageIndex < 23) {  // We have 23 valid menu pages
                        displayText = menuPageNames_[pageIndex];
                        static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                        
                        if (fileLogger_) {
                            fileLogger_->logMessage("[DISPLAY] Menu page shown: " + displayText + 
                                " (index " + juce::String(pageIndex) + ")");
                        }
                    } else {
                        if (fileLogger_) {
                            fileLogger_->logMessage("[DISPLAY] ERROR: Menu page index out of bounds: " + 
                                juce::String(pageIndex));
                        }
                    }
                } else {
                    if (fileLogger_) {
                        fileLogger_->logMessage("[DISPLAY] Menu page is None, not showing menu");
                    }
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
                currentAlgorithm_ = 22; // VOWL is at index 22, not 23 (VOW2 is at 23)
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
    if (fileLogger_) {
        fileLogger_->logMessage("=== ENCODER ROTATION DEBUG ===");
        fileLogger_->logMessage("Delta: " + juce::String(delta));
        fileLogger_->logMessage("Display Mode: " + juce::String(static_cast<int>(displayMode_)) + 
            " (0=Algorithm, 1=Value, 2=Menu, 3=Startup)");
    }
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            {
                int oldAlgorithm = currentAlgorithm_;
                // Navigate algorithms with wrapping (like original Braids)
                currentAlgorithm_ += delta;
                
                // Wrap around at the boundaries
                if (currentAlgorithm_ < 0) {
                    currentAlgorithm_ = 47;  // Wrap to last algorithm
                } else if (currentAlgorithm_ > 47) {
                    currentAlgorithm_ = 0;   // Wrap to first algorithm
                }
                
                if (fileLogger_) {
                    fileLogger_->logMessage("Algorithm changed from [" + juce::String(oldAlgorithm) + "] " + 
                        juce::String(algorithmNames_[oldAlgorithm]) + " to [" + juce::String(currentAlgorithm_) + 
                        "] " + juce::String(algorithmNames_[currentAlgorithm_]));
                }
                
                // Update the audio parameter
                updateAlgorithmParameter();
                
                // Stay in algorithm mode, don't jump to menu
                displayMode_ = DisplayMode::Algorithm;
                updateDisplay();  // Update display immediately after changing algorithm
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
                editMenuValue(delta);
                updateDisplay();  // Update display immediately to show new value
            }
            break;
            
        default:
            break;
    }
    
    updateDisplay();
}

void BraidyAudioProcessorEditor::handleEncoderClick() {
    if (fileLogger_) {
        fileLogger_->logMessage("[CLICK] Encoder clicked in display mode: " + 
            juce::String(static_cast<int>(displayMode_)) + ", inEditMode: " + 
            juce::String(inEditMode_ ? "true" : "false"));
    }
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            // Short click in algorithm mode does nothing (authentic behavior)
            break;
            
        case DisplayMode::Menu:
            if (!inEditMode_) {
                // Check if WAVE is selected - save and exit
                if (currentMenuPage_ == MenuPage::WAVE) {
                    if (fileLogger_) {
                        fileLogger_->logMessage("[MENU] WAVE selected - saving settings and exiting menu");
                    }
                    // TODO: Save current settings to persistent storage
                    exitMenuMode();
                } else {
                    // Enter edit mode for other menu items
                    inEditMode_ = true;
                    displayMode_ = DisplayMode::Value;
                    if (fileLogger_) {
                        fileLogger_->logMessage("[MENU] Entering value edit mode for: " + 
                            juce::String(menuPageNames_[static_cast<int>(currentMenuPage_) - 1]));
                    }
                }
            } else {
                // Should not happen in menu mode
                inEditMode_ = false;
            }
            updateDisplay();
            break;
            
        case DisplayMode::Value:
            // Save value and return to menu
            if (inEditMode_) {
                applyMenuValue();
                inEditMode_ = false;
                displayMode_ = DisplayMode::Menu;
                if (fileLogger_) {
                    fileLogger_->logMessage("[MENU] Saved value " + juce::String(menuValue_) + 
                        ", returning to menu");
                }
            } else {
                // Exit to algorithm mode
                displayMode_ = DisplayMode::Algorithm;
            }
            updateDisplay();
            break;
            
        default:
            break;
    }
    
    updateDisplay();
}

void BraidyAudioProcessorEditor::handleEncoderLongPress() {
    if (fileLogger_) {
        fileLogger_->logMessage("[LONG PRESS] Triggered! Current display mode: " + 
            juce::String(static_cast<int>(displayMode_)));
    }
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            // Enter settings menu
            enterMenuMode();
            break;
            
        case DisplayMode::Menu:
        case DisplayMode::Value:
            // Exit settings menu from any menu-related mode
            exitMenuMode();
            break;
            
        default:
            break;
    }
}

//==============================================================================
// Menu System  
//==============================================================================
void BraidyAudioProcessorEditor::enterMenuMode() {
    if (fileLogger_) {
        fileLogger_->logMessage("[MENU] Entering menu mode from algorithm: " + 
            juce::String(algorithmNames_[currentAlgorithm_]));
    }
    displayMode_ = DisplayMode::Menu;
    currentMenuPage_ = MenuPage::WAVE;  // Start with WAVE option (save and exit)
    inEditMode_ = false;
    updateDisplay();
}

void BraidyAudioProcessorEditor::exitMenuMode() {
    DBG("[MENU] Exiting menu mode, returning to algorithm display");
    displayMode_ = DisplayMode::Algorithm;
    currentMenuPage_ = MenuPage::None;
    inEditMode_ = false;
    updateDisplay();
}

void BraidyAudioProcessorEditor::navigateMenu(int delta) {
    int currentPage = static_cast<int>(currentMenuPage_);
    // Navigate through menu pages (1=WAVE to 24=VERS)
    int newPage = currentPage + delta;
    
    // Wrap around menu pages
    if (newPage < 1) {
        newPage = 24;  // Wrap to VERS
    } else if (newPage > 24) {
        newPage = 1;   // Wrap to WAVE
    }
    
    currentMenuPage_ = static_cast<MenuPage>(newPage);
    
    // Initialize menu value based on current page
    switch (currentMenuPage_) {
        case MenuPage::WAVE:
            // WAVE doesn't have a value, it's just a save/exit option
            menuValue_ = 0;
            break;
        case MenuPage::META:
            menuValue_ = 0;  // Meta mode off/on
            break;
        case MenuPage::BITS:
            menuValue_ = 16;  // Bit depth (4-16)
            break;
        case MenuPage::RATE:
            menuValue_ = 48;  // Sample rate (48kHz)
            break;
        case MenuPage::BRIG:
            menuValue_ = 127;  // Display brightness (0-127)
            break;
        case MenuPage::VCA:
            menuValue_ = processorRef.getBraidySettings().GetParameter(braidy::BraidyParameter::VCA_MODE) > 0.5f ? 1 : 0;
            break;
        default:
            menuValue_ = 0;
            break;
    }
}

void BraidyAudioProcessorEditor::editMenuValue(int delta) {
    // Edit menu value based on current page
    switch (currentMenuPage_) {
        case MenuPage::WAVE:
            // WAVE doesn't have editable values - it's just save/exit
            break;
        case MenuPage::META:
            menuValue_ = (menuValue_ == 0) ? 1 : 0;  // Toggle
            break;
        case MenuPage::BITS:
            menuValue_ = juce::jlimit(4, 16, menuValue_ + delta);
            break;
        case MenuPage::RATE:
            menuValue_ = juce::jlimit(4, 96, menuValue_ + delta);
            break;
        case MenuPage::BRIG:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
            break;
        case MenuPage::TSRC:
            menuValue_ = juce::jlimit(0, 2, menuValue_ + delta);  // 0=Off, 1=Gate, 2=Trigger
            break;
        case MenuPage::TDLY:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
            break;
        case MenuPage::ATK:
        case MenuPage::DEC:
        case MenuPage::FM:
        case MenuPage::TIM:
        case MenuPage::COL:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
            break;
        case MenuPage::VCA:
            menuValue_ = (menuValue_ == 0) ? 1 : 0;  // Toggle VCA on/off
            break;
        case MenuPage::RANG:
            menuValue_ = juce::jlimit(0, 4, menuValue_ + delta);  // Range settings
            break;
        case MenuPage::OCTV:
            menuValue_ = juce::jlimit(-2, 2, menuValue_ + delta);  // Octave -2 to +2
            break;
        case MenuPage::QNTZ:
            menuValue_ = juce::jlimit(0, 12, menuValue_ + delta);  // Quantizer modes
            break;
        case MenuPage::ROOT:
            menuValue_ = juce::jlimit(0, 11, menuValue_ + delta);  // Root note C-B
            break;
        case MenuPage::FLAT:
            menuValue_ = juce::jlimit(-99, 99, menuValue_ + delta);  // Detuning
            break;
        case MenuPage::DRFT:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);  // Drift amount
            break;
        case MenuPage::SIGN:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);  // Signature
            break;
        default:
            menuValue_ = juce::jlimit(0, 127, menuValue_ + delta);
            break;
    }
}

juce::String BraidyAudioProcessorEditor::getFormattedMenuValue() const {
    // Format the menu value appropriately for each setting type
    switch (currentMenuPage_) {
        case MenuPage::WAVE:
            return "SAVE";  // Show SAVE when in WAVE mode
            
        case MenuPage::META:
            return menuValue_ == 0 ? " OFF" : " ON ";
            
        case MenuPage::VCA:
            return menuValue_ == 0 ? " OFF" : " ON ";
            
        case MenuPage::TSRC:
            switch (menuValue_) {
                case 0: return " OFF";
                case 1: return "GATE";
                case 2: return "TRIG";
                default: return " OFF";
            }
            
        case MenuPage::RANG:
            // Frequency range names from Braids
            switch (menuValue_) {
                case 0: return "FLTR";  // Filter mode
                case 1: return " LO ";  // Low range
                case 2: return " MID";  // Mid range  
                case 3: return " HI ";  // High range
                case 4: return "FULL";  // Full range
                default: return " MID";
            }
            
        case MenuPage::OCTV:
            // Show octave with sign
            if (menuValue_ > 0) {
                return " +" + juce::String(menuValue_);
            } else if (menuValue_ < 0) {
                return " " + juce::String(menuValue_);  // Negative sign included
            } else {
                return "  0 ";
            }
            
        case MenuPage::ROOT:
            // Musical note names
            {
                const char* notes[] = {"  C ", " C# ", "  D ", " D# ", "  E ", "  F ", 
                                      " F# ", "  G ", " G# ", "  A ", " A# ", "  B "};
                return juce::String(notes[menuValue_ % 12]);
            }
            
        case MenuPage::QNTZ:
            // Quantizer scale names
            switch (menuValue_) {
                case 0: return " OFF";
                case 1: return "SEMI";  // Semitones
                case 2: return "IONI";  // Ionian (Major)
                case 3: return "DORI";  // Dorian
                case 4: return "PHRY";  // Phrygian
                case 5: return "LYDI";  // Lydian
                case 6: return "MIXO";  // Mixolydian
                case 7: return "AEOL";  // Aeolian (Minor)
                case 8: return "LOCR";  // Locrian
                case 9: return "BLU+";  // Blues major
                case 10: return "BLU-";  // Blues minor
                case 11: return "PEN+";  // Pentatonic major
                case 12: return "PEN-";  // Pentatonic minor
                default: return " OFF";
            }
            
        case MenuPage::FLAT:
            // Detuning with sign (cents)
            if (menuValue_ > 0) {
                return "+" + juce::String(menuValue_).paddedLeft('0', 2);
            } else if (menuValue_ < 0) {
                return juce::String(menuValue_).paddedLeft('0', 3);
            } else {
                return "  0 ";
            }
            
        case MenuPage::BITS:
            // Bit depth - show as number with "b" suffix
            return juce::String(menuValue_).paddedLeft(' ', 2) + "b";
            
        case MenuPage::RATE:
            // Sample rate in kHz
            return juce::String(menuValue_).paddedLeft(' ', 2) + "k";
            
        case MenuPage::BRIG:
            // Brightness as percentage
            {
                int percent = (menuValue_ * 100) / 127;
                return juce::String(percent).paddedLeft(' ', 3) + "%";
            }
            
        case MenuPage::CALI:
            return "CAL>";  // Calibration mode indicator
            
        case MenuPage::VERS:
            return "1.0 ";  // Version number
            
        default:
            // For all other numeric values (envelopes, etc.), show as 0-127
            return juce::String(menuValue_).paddedLeft(' ', 3);
    }
}

void BraidyAudioProcessorEditor::applyMenuValue() {
    // Apply the edited menu value to the appropriate parameter
    auto& apvts = processorRef.getAPVTS();
    auto& settings = processorRef.getBraidySettings();
    
    switch (currentMenuPage_) {
        case MenuPage::WAVE:
            // WAVE saves settings and exits - handled in click handler
            break;
            
        case MenuPage::META:
            settings.SetParameter(braidy::BraidyParameter::META_ENABLED, menuValue_ != 0 ? 1.0f : 0.0f);
            break;
        case MenuPage::BITS:
            settings.SetParameter(braidy::BraidyParameter::BIT_CRUSHER_BITS, static_cast<float>(menuValue_));
            if (auto* param = apvts.getParameter("bitDepth")) {
                param->setValueNotifyingHost(menuValue_ / 16.0f);
            }
            break;
        case MenuPage::RATE:
            settings.SetParameter(braidy::BraidyParameter::BIT_CRUSHER_RATE, static_cast<float>(menuValue_));
            if (auto* param = apvts.getParameter("sampleRate")) {
                param->setValueNotifyingHost(menuValue_ / 96.0f);
            }
            break;
        case MenuPage::BRIG:
            // Display brightness - UI only
            break;
        case MenuPage::ATK:
            if (auto* param = apvts.getParameter("attack")) {
                param->setValueNotifyingHost(menuValue_ / 127.0f);
            }
            break;
        case MenuPage::DEC:
            if (auto* param = apvts.getParameter("decay")) {
                param->setValueNotifyingHost(menuValue_ / 127.0f);
            }
            break;
        case MenuPage::FM:
            if (auto* param = apvts.getParameter("fm")) {
                param->setValueNotifyingHost(menuValue_ / 127.0f);
            }
            break;
        case MenuPage::TIM:
            if (auto* param = apvts.getParameter("timbre")) {
                param->setValueNotifyingHost(menuValue_ / 127.0f);
            }
            break;
        case MenuPage::COL:
            if (auto* param = apvts.getParameter("color")) {
                param->setValueNotifyingHost(menuValue_ / 127.0f);
            }
            break;
        case MenuPage::VCA:
            settings.SetParameter(braidy::BraidyParameter::VCA_MODE, menuValue_ != 0 ? 1.0f : 0.0f);
            if (auto* param = apvts.getParameter("vca")) {
                param->setValueNotifyingHost(menuValue_);
            }
            break;
        case MenuPage::RANG:
            // Using pitch octave for frequency range
            settings.SetParameter(braidy::BraidyParameter::PITCH_OCTAVE, static_cast<float>(menuValue_ - 2));
            break;
        case MenuPage::OCTV:
            settings.SetParameter(braidy::BraidyParameter::PITCH_OCTAVE, static_cast<float>(menuValue_));
            break;
        case MenuPage::QNTZ:
            settings.SetParameter(braidy::BraidyParameter::QUANTIZER_SCALE, static_cast<float>(menuValue_));
            break;
        case MenuPage::ROOT:
            settings.SetParameter(braidy::BraidyParameter::QUANTIZER_ROOT, static_cast<float>(menuValue_));
            break;
        case MenuPage::FLAT:
            // Using paraphony detune for detuning
            settings.SetParameter(braidy::BraidyParameter::PARAPHONY_DETUNE, menuValue_ / 99.0f);
            break;
        case MenuPage::DRFT:
            // Using meta speed for drift
            settings.SetParameter(braidy::BraidyParameter::META_SPEED, menuValue_ / 127.0f);
            break;
        case MenuPage::SIGN:
            // Signature - not directly mapped to a parameter yet
            break;
        default:
            break;
    }
    
    // Save the current waveform's state after any parameter change
    auto& stateManager = processorRef.getWaveformStateManager();
    braidy::MacroOscillatorShape currentShape = static_cast<braidy::MacroOscillatorShape>(currentAlgorithm_);
    stateManager.SaveCurrentToWaveform(currentShape, settings);
    DBG("Saved current parameters to waveform: " + juce::String(algorithmNames_[currentAlgorithm_]));
    
    // Parameters are updated directly, no need to call private update method
}

void BraidyAudioProcessorEditor::updateAlgorithmParameter() {
    DBG("=== UPDATE ALGORITHM PARAMETER DEBUG ===");
    
    // Get the previous algorithm shape for state management
    static int previousAlgorithm = currentAlgorithm_;
    braidy::MacroOscillatorShape previousShape = static_cast<braidy::MacroOscillatorShape>(previousAlgorithm);
    braidy::MacroOscillatorShape currentShape = static_cast<braidy::MacroOscillatorShape>(currentAlgorithm_);
    
    // Save current waveform state and load new waveform state
    if (previousAlgorithm != currentAlgorithm_) {
        DBG("Switching waveform from " + juce::String(algorithmNames_[previousAlgorithm]) + 
            " to " + juce::String(algorithmNames_[currentAlgorithm_]));
        
        // Use WaveformStateManager to switch between waveforms
        auto& stateManager = processorRef.getWaveformStateManager();
        auto& settings = processorRef.getBraidySettings();
        
        // This saves the previous waveform's state and loads the new one
        stateManager.SwitchWaveform(previousShape, currentShape, settings);
        
        DBG("Loaded saved state for " + juce::String(algorithmNames_[currentAlgorithm_]));
        previousAlgorithm = currentAlgorithm_;
    }
    
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
    if (button == settingsButton_.get()) {
        // Toggle modulation overlay visibility
        if (modulationOverlay_) {
            if (modulationOverlay_->isOverlayVisible()) {
                modulationOverlay_->hideOverlay();
            } else {
                modulationOverlay_->showOverlay();
                modulationOverlay_->toFront(true);
            }
        }
    }
}

void BraidyAudioProcessorEditor::timerCallback() {
    // Update parameter values from processor
    updateParameterValues();
    
    // Don't update display from timer - let encoder events handle it
    // This prevents duplicate display updates and lag
    
    // Repaint LEDs to show parameter activity (less frequent)
    static int repaintCounter = 0;
    if (++repaintCounter >= 5) {  // Only repaint every 5th tick (500ms)
        repaintCounter = 0;
        repaint();
    }
}

//==============================================================================
// Computer Keyboard to MIDI Implementation
//==============================================================================
void BraidyAudioProcessorEditor::initializeKeyboardMapping() {
    // Standard computer keyboard to MIDI note mapping
    // Bottom row (white keys): A S D F G H J K L ; '
    keyToMidiNoteMap_['a'] = 0;   // C
    keyToMidiNoteMap_['s'] = 2;   // D
    keyToMidiNoteMap_['d'] = 4;   // E
    keyToMidiNoteMap_['f'] = 5;   // F
    keyToMidiNoteMap_['g'] = 7;   // G
    keyToMidiNoteMap_['h'] = 9;   // A
    keyToMidiNoteMap_['j'] = 11;  // B
    keyToMidiNoteMap_['k'] = 12;  // C (next octave)
    keyToMidiNoteMap_['l'] = 14;  // D
    keyToMidiNoteMap_[';'] = 16;  // E
    keyToMidiNoteMap_['\''] = 17; // F
    
    // Top row (black keys): W E T Y U O P
    keyToMidiNoteMap_['w'] = 1;   // C#
    keyToMidiNoteMap_['e'] = 3;   // D#
    keyToMidiNoteMap_['t'] = 6;   // F#
    keyToMidiNoteMap_['y'] = 8;   // G#
    keyToMidiNoteMap_['u'] = 10;  // A#
    keyToMidiNoteMap_['o'] = 13;  // C#
    keyToMidiNoteMap_['p'] = 15;  // D#
    
    // Lower octave with Z X C V B N M
    keyToMidiNoteMap_['z'] = -12; // C (lower octave)
    keyToMidiNoteMap_['x'] = -10; // D
    keyToMidiNoteMap_['c'] = -8;  // E
    keyToMidiNoteMap_['v'] = -7;  // F
    keyToMidiNoteMap_['b'] = -5;  // G
    keyToMidiNoteMap_['n'] = -3;  // A
    keyToMidiNoteMap_['m'] = -1;  // B
    
    DBG("[MIDI KEYBOARD] Initialized keyboard mapping with " + juce::String(keyToMidiNoteMap_.size()) + " keys");
}

void BraidyAudioProcessorEditor::mouseDown(const juce::MouseEvent& event) {
    // Grab keyboard focus when the window is clicked
    grabKeyboardFocus();
    DBG("[MOUSE] Window clicked - grabbing keyboard focus");
}

bool BraidyAudioProcessorEditor::keyPressed(const juce::KeyPress& key) {
    auto keyChar = key.getKeyCode();
    
    DBG("[KEYBOARD] keyPressed called with keycode: " + juce::String(keyChar) + 
        " (" + juce::String::charToString(keyChar) + ")");
    
    // Convert to lowercase for consistent mapping
    if (keyChar >= 'A' && keyChar <= 'Z') {
        keyChar = keyChar - 'A' + 'a';
    }
    
    // Check if this key is in our MIDI mapping
    auto it = keyToMidiNoteMap_.find(keyChar);
    if (it != keyToMidiNoteMap_.end()) {
        // Check if key is already pressed to avoid retriggering
        if (pressedKeys_.find(keyChar) == pressedKeys_.end()) {
            pressedKeys_.insert(keyChar);
            
            int relativeNote = it->second;
            int midiNote = getOctaveOffset() + relativeNote;
            
            // Ensure MIDI note is in valid range (0-127)
            if (midiNote >= 0 && midiNote <= 127) {
                sendMidiNoteOn(midiNote, 0.7f);
                DBG("[MIDI KEYBOARD] Key '" + juce::String::charToString(keyChar) + 
                    "' pressed -> MIDI note " + juce::String(midiNote));
            }
        }
        return true;
    } else {
        DBG("[KEYBOARD] Key not in MIDI mapping: " + juce::String::charToString(keyChar));
    }
    
    return false;
}

bool BraidyAudioProcessorEditor::keyStateChanged(bool isKeyDown) {
    // This is called when any key state changes
    // We use a timer-based approach to detect key releases
    if (!isKeyDown) {
        // Start a timer to check for released keys
        juce::Timer::callAfterDelay(10, [this]() {
            this->checkForReleasedKeys();
        });
        return true;
    }
    return false;
}

void BraidyAudioProcessorEditor::sendMidiNoteOn(int midiNote, float velocity) {
    if (auto* processor = dynamic_cast<BraidyAudioProcessor*>(&processorRef)) {
        // Create MIDI note on message
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, midiNote, velocity);
        
        // Send to processor for immediate processing
        processor->processMidiMessage(noteOn);
        
        DBG("[MIDI SEND] Note ON: " + juce::String(midiNote) + " velocity: " + juce::String(velocity, 2));
    }
}

void BraidyAudioProcessorEditor::sendMidiNoteOff(int midiNote) {
    if (auto* processor = dynamic_cast<BraidyAudioProcessor*>(&processorRef)) {
        // Create MIDI note off message
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, midiNote, 0.0f);
        
        // Send to processor for immediate processing
        processor->processMidiMessage(noteOff);
        
        DBG("[MIDI SEND] Note OFF: " + juce::String(midiNote));
    }
}

void BraidyAudioProcessorEditor::checkForReleasedKeys() {
    // Simple approach: clear all pressed keys and send note offs
    // This is a fallback since JUCE doesn't provide easy key release detection
    std::set<int> keysToRelease = pressedKeys_;
    
    for (int keyChar : keysToRelease) {
        auto it = keyToMidiNoteMap_.find(keyChar);
        if (it != keyToMidiNoteMap_.end()) {
            int relativeNote = it->second;
            int midiNote = getOctaveOffset() + relativeNote;
            
            if (midiNote >= 0 && midiNote <= 127) {
                sendMidiNoteOff(midiNote);
                DBG("[MIDI KEYBOARD] Auto-release key '" + juce::String::charToString(keyChar) + 
                    "' -> MIDI note " + juce::String(midiNote) + " OFF");
            }
        }
    }
    
    pressedKeys_.clear();
}