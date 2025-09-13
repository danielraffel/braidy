#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BraidsModelDefaults.h"
#include <map>
#include <set>

// Exact algorithm names from Mutable Instruments Braids (4-character display)
// Note: Braids has exactly 47 algorithms (0-46)
const std::array<const char*, 47> BraidyAudioProcessorEditor::algorithmNames_ = {
    "CSAW", "MRPH", "S/SQ", "S/TR", "BUZZ",          // Basic analog (0-4)
    "+SUB", "SAW+", "+SYN", "SAW*", "TRI3",          // Sub variants (5-9)
    "SQ3",  "TR3",  "SI3",  "RI3",  "SWRM",          // Triple variants (10-14)
    "COMB", "TOY",  "FLTR", "PEAK", "BAND",          // Digital filters (15-19)
    "HIGH", "VOSM", "VOWL", "VOW2", "HARM",          // Formants (20-24)
    "FM",   "FBFM", "WTFM", "PLUK", "BOWD",          // FM & Physical (25-29)
    "BLOW", "FLUT", "BELL", "DRUM", "KICK",          // Physical modeling (30-34)
    "CYMB", "SNAR", "WTBL", "WMAP", "WLIN",          // Percussion & Wavetable (35-39)
    "WPAR", "NOIS", "TWLN", "CLKN", "CLDS",          // Wavetable & Noise (40-44)
    "PART", "DIGI"                                   // Final algorithms (45-46)
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
    hasRotated_ = false;  // Reset rotation flag
    pressStartTime_ = juce::Time::currentTimeMillis();
    
    // Calculate initial angle from mouse position
    auto center = getLocalBounds().getCentre().toFloat();
    lastAngle_ = std::atan2(e.position.y - center.y, e.position.x - center.x);
    DBG("Mouse down at angle: " << lastAngle_);
    
    // Start timer for long press detection with unique ID
    startTimer(1, 100);  // Timer ID 1, check every 100ms
    
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
    // Sensitivity controls how much mouse movement translates to rotation
    float sensitivity = 0.5f;  // Reduced for 90-degree steps
    
    // Need larger threshold to prevent accidental movements
    if (std::abs(angleDiff) < 0.05f) return;
    
    // Accumulate fractional angle changes
    accumulatedAngle_ += angleDiff * sensitivity;
    
    // Debug output for angle tracking
    static int debugCounter = 0;
    if (++debugCounter % 10 == 0) {  // Log every 10th update
        DBG("[ENCODER] Accumulated angle: " + juce::String(accumulatedAngle_, 2) + 
            " radians (" + juce::String(accumulatedAngle_ * 180.0f / juce::MathConstants<float>::pi, 1) + "°)");
    }
    
    // Convert accumulated angle to steps - more like original Braids
    // Smaller steps for finer control (about 15 degrees per step)
    int steps = 0;
    const float stepThreshold = juce::MathConstants<float>::pi / 12.0f;  // 15 degrees in radians
    
    if (std::abs(accumulatedAngle_) >= stepThreshold) {
        steps = (accumulatedAngle_ > 0) ? 1 : -1;  // Single step per 90 degrees
        
        // Keep remainder for smooth rotation
        float remainder = std::fmod(accumulatedAngle_, stepThreshold);
        accumulatedAngle_ = (accumulatedAngle_ > 0) ? remainder : -remainder;
        
        DBG("[ENCODER] Step triggered! Direction: " + juce::String(steps > 0 ? "CW" : "CCW") + 
            ", Remaining angle: " + juce::String(accumulatedAngle_, 2) + " radians");
    }
    
    if (steps != 0) {
        DBG("=== ENCODER DRAG: steps = " << steps);
        
        // Mark that we've rotated - this should cancel any long press detection
        hasRotated_ = true;
        
        if (onValueChange) {
            onValueChange(steps);
        }
    }
    
    lastAngle_ = currentAngle;
    repaint();
}

void BraidyAudioProcessorEditor::BraidsEncoder::mouseUp(const juce::MouseEvent& e) {
    if (!isPressed_) return;
    
    stopTimer(1);  // Stop timer ID 1 for long press
    
    auto pressDuration = juce::Time::currentTimeMillis() - pressStartTime_;
    
    // Only trigger click if we haven't rotated and haven't already triggered long press
    if (!hasRotated_ && !longPressTriggered_) {
        if (pressDuration < 500) {
            // Short click
            if (onClick) onClick();
        }
    }
    
    isPressed_ = false;
    hasRotated_ = false;
    longPressTriggered_ = false;
    repaint();
}

void BraidyAudioProcessorEditor::BraidsEncoder::timerCallback(int timerID) {
    if (timerID != 1) return;  // Only handle our timer
    
    if (!isPressed_ || hasRotated_ || longPressTriggered_) {
        stopTimer(1);
        return;
    }
    
    auto pressDuration = juce::Time::currentTimeMillis() - pressStartTime_;
    
    if (pressDuration >= 500) {
        // Long press detected
        longPressTriggered_ = true;
        stopTimer(1);
        
        if (onLongPress) {
            DBG("[ENCODER] Long press callback triggered");
            onLongPress();
        }
    }
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
    
    // Initialize manipulation tracking
    wasRecentlyManipulated_ = false;
    lastManipulationTime_ = 0;
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
    isPressed_ = true;
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

void BraidyAudioProcessorEditor::BraidsKnob::setValueAndNotify(float value) {
    value_ = juce::jlimit(0.0f, 1.0f, value);
    
    // Only trigger callback when explicitly requested (user interaction)
    if (onValueChange) {
        onValueChange(value_);
    }
    
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

void BraidyAudioProcessorEditor::BraidsKnob::mouseUp(const juce::MouseEvent& e) {
    isPressed_ = false;
    
    // Set manipulation flag and start timer for brief hold period
    wasRecentlyManipulated_ = true;
    lastManipulationTime_ = juce::Time::currentTimeMillis();
    startTimer(100); // Check every 100ms
}

void BraidyAudioProcessorEditor::BraidsKnob::timerCallback() {
    // Clear manipulation flag after 500ms of no interaction
    if (juce::Time::currentTimeMillis() - lastManipulationTime_ > 500) {
        wasRecentlyManipulated_ = false;
        stopTimer();
    }
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
    g.setFont(8.0f);
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
    
    // Don't want keyboard focus to prevent interfering with Logic's shortcuts
    // The plugin should never grab keyboard focus automatically
    setWantsKeyboardFocus(false);
    
    DBG("[STARTUP] Editor size: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    DBG("[STARTUP] Keyboard focus enabled");
    
    // Initialize display to show algorithm (not menu)  
    displayMode_ = DisplayMode::Algorithm;
    currentAlgorithm_ = 0; // Start with CSAW (index 0)
    currentMenuPage_ = MenuPage::None;  // No menu page selected
    inEditMode_ = false;  // Not editing any value
    
    // Create modulation overlay (initially hidden) - simplified for now
    // modulationOverlay_ = std::make_unique<braidy::ModulationSettingsOverlay>(processorRef.getModulationMatrix());
    // modulationOverlay_->setVisible(false);  // Start hidden
    // addChildComponent(modulationOverlay_.get());
    
    DBG("[STARTUP] Initial algorithm: [" + juce::String(currentAlgorithm_) + "] " + juce::String(algorithmNames_[currentAlgorithm_]));
    
    setupComponents();
    updateDisplay();
    
    // Force initial layout and repaint
    resized();
    repaint();
    
    // Initialize computer keyboard to MIDI mapping
    initializeKeyboardMapping();
    
    // Don't automatically grab keyboard focus - this interferes with host shortcuts
    // User can click on the plugin to give it focus when needed
    DBG("[FOCUS] Plugin ready - click to focus for keyboard input");
    
    // Start timer for parameter updates (reduce frequency to avoid issues)
    startTimer(30);  // 33Hz for smooth real-time modulation display
    
    DBG("[STARTUP] Constructor complete with MIDI keyboard support");
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
    // Set shutdown flag to prevent timer callbacks
    isShuttingDown_ = true;
    
    // Stop all timers
    stopTimer();
    
    // Wait a brief moment for any pending timer callbacks to complete
    juce::Thread::sleep(10);
    
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
    fineKnob_->onValueChange = [this](float value) {
        // Fine tuning: +/- 100 cents (1 semitone)
        // Store as parameter that will be applied to all voices
        DBG("Fine knob changed: " + juce::String(value));
        if (auto* param = processorRef.getAPVTS().getParameter("fineTune")) {
            param->setValueNotifyingHost(value);
        }
    };
    addAndMakeVisible(*fineKnob_);
    fineKnob_->setVisible(true);
    DBG("Created Fine Knob");
    
    coarseKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar for +/- range
    coarseKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    coarseKnob_->onValueChange = [this](float value) {
        // Coarse tuning: Braids hardware has 5 octave positions (-2, -1, 0, +1, +2)
        // Convert continuous value to discrete octave selection
        DBG("Coarse knob changed: " + juce::String(value));
        if (auto* param = processorRef.getAPVTS().getParameter("coarseTune")) {
            // Map 0-1 to 5 discrete positions
            float discreteValue = std::round(value * 4.0f) / 4.0f;
            param->setValueNotifyingHost(discreteValue);
        }
    };
    addAndMakeVisible(*coarseKnob_);
    coarseKnob_->setVisible(true);
    DBG("Created Coarse Knob");
    
    fmKnob_ = std::make_unique<BraidsKnob>(false);  // Unipolar (0-100% FM depth)
    fmKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    fmKnob_->onValueChange = [this](float value) {
        // FM amount: 0-100% modulation depth
        if (auto* param = processorRef.getAPVTS().getParameter("fmAmount")) {
            param->setValueNotifyingHost(value);
        }
    };
    addAndMakeVisible(*fmKnob_);
    fmKnob_->setVisible(true);
    DBG("Created FM Knob");
    
    // Main parameter knobs with colored indicators (matching hardware)
    timbreKnob_ = std::make_unique<BraidsKnob>(false, 0xFF00CCA3);  // Teal indicator
    timbreKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    timbreKnob_->onValueChange = [this](float value) {
        // Update parameter 1 (timbre)
        if (auto* param = processorRef.getAPVTS().getParameter("param1")) {
            param->setValueNotifyingHost(value);
        }
    };
    addAndMakeVisible(*timbreKnob_);
    timbreKnob_->setVisible(true);
    DBG("Created Timbre Knob");
    
    colorKnob_ = std::make_unique<BraidsKnob>(false, 0xFFE74C3C);  // Red indicator
    colorKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    colorKnob_->onValueChange = [this](float value) {
        // Update parameter 2 (color) 
        if (auto* param = processorRef.getAPVTS().getParameter("param2")) {
            param->setValueNotifyingHost(value);
        }
    };
    addAndMakeVisible(*colorKnob_);
    colorKnob_->setVisible(true);
    DBG("Created Color Knob");
    
    // Modulation attenuverter (center knob)
    timbreModKnob_ = std::make_unique<BraidsKnob>(true);  // Bipolar for attenuverter
    timbreModKnob_->setWantsKeyboardFocus(false);  // Prevent knob from stealing focus
    timbreModKnob_->onValueChange = [this](float value) {
        // In original Braids, this is a modulation attenuverter
        // It controls the amount and polarity of modulation applied to timbre
        DBG("Timbre modulation knob changed: " + juce::String(value));
        
        // Control LFO1 modulation amount for TIMBRE parameter
        auto& modMatrix = processorRef.getModulationMatrix();
        
        // Convert knob value (-1 to +1) to modulation amount
        float modulationAmount = (value - 0.5f) * 2.0f;  // Convert 0-1 to -1 to +1
        
        if (std::abs(modulationAmount) > 0.01f) {
            // Enable modulation routing: LFO1 -> TIMBRE
            modMatrix.setRouting(0, braidy::ModulationMatrix::TIMBRE, modulationAmount, true);
            
            // Enable LFO1 if it's not already enabled
            auto& lfo1 = modMatrix.getLFO(0);
            if (!lfo1.isEnabled()) {
                lfo1.setEnabled(true);
                lfo1.setRate(2.0f);  // 2 Hz default rate
                lfo1.setDepth(1.0f);  // Full depth, controlled by amount
                lfo1.setShape(braidy::LFO::SINE);  // Sine wave default
                DBG("Enabled LFO1 for timbre modulation");
            }
        } else {
            // Disable modulation routing when knob is at center
            modMatrix.clearRouting(braidy::ModulationMatrix::TIMBRE);
        }
    };
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
    
    // Create modulation overlay LAST so it's on top of everything
    modulationOverlay_ = std::make_unique<braidy::ModulationSettingsOverlay>(modulationMatrix_, processorRef.getAPVTS(), fileLogger_.get());
    modulationOverlay_->setVisible(false);  // Start hidden
    modulationOverlay_->onClose = [this]() {
        modulationOverlay_->hideOverlay();
    };
    
    // Sync meta mode between main menu and modulation overlay
    modulationOverlay_->onMetaModeChanged = [this](bool enabled) {
        // Update the main parameter
        if (auto* param = processorRef.getAPVTS().getParameter("metaMode")) {
            param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        }
        // Sync with menu value (so both UIs stay in sync)
        if (currentMenuPage_ == MenuPage::META) {
            menuValue_ = enabled ? 1 : 0;
        }
    };
    
    // LFO Enable callback
    modulationOverlay_->onLfoEnableChanged = [this](int lfoIndex, bool enabled) {
        juce::String paramId = "lfo" + juce::String(lfoIndex + 1) + "Enable";
        if (auto* param = processorRef.getAPVTS().getParameter(paramId)) {
            param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        }
    };
    
    // LFO Shape callback
    modulationOverlay_->onLfoShapeChanged = [this](int lfoIndex, int shapeIndex) {
        juce::String paramId = "lfo" + juce::String(lfoIndex + 1) + "Shape";
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processorRef.getAPVTS().getParameter(paramId))) {
            float normalizedValue = param->convertTo0to1(shapeIndex);
            param->setValueNotifyingHost(normalizedValue);
        }
    };
    
    // LFO Tempo Sync callback
    modulationOverlay_->onLfoTempoSyncChanged = [this](int lfoIndex, bool synced) {
        juce::String paramId = "lfo" + juce::String(lfoIndex + 1) + "TempoSync";
        if (auto* param = processorRef.getAPVTS().getParameter(paramId)) {
            param->setValueNotifyingHost(synced ? 1.0f : 0.0f);
        }
    };
    
    // LFO Destination callback
    modulationOverlay_->onLfoDestChanged = [this](int lfoIndex, int destIndex) {
        juce::String paramId = "lfo" + juce::String(lfoIndex + 1) + "Dest";
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processorRef.getAPVTS().getParameter(paramId))) {
            float normalizedValue = param->convertTo0to1(destIndex);
            param->setValueNotifyingHost(normalizedValue);
            
            // Trigger audio-side routing update to handle META mode auto-routing
            processorRef.updateModulationFromParameters();
        }
    };
    addChildComponent(*modulationOverlay_);  // Use addChildComponent so it starts hidden
    modulationOverlay_->setAlwaysOnTop(true);  // Ensure overlay stays on top
    modulationOverlay_->toFront(true);  // Bring to front
    DBG("Created Modulation Overlay");
}

void BraidyAudioProcessorEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Draw authentic Braids panel background
    drawBraidsPanel(g);
    drawScrewHoles(g);
    
    // Panel title and subtitle (compact)
    auto titleBounds = bounds.removeFromTop(28);
    g.setColour(juce::Colour(BraidsColors::text));
    g.setFont(12.0f);
    g.drawText("Braids", titleBounds.removeFromTop(16), juce::Justification::centred);
    
    g.setFont(8.0f);
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
    g.setFont(8.0f);
    
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
        
        // Check if TIMBRE is being modulated by LFO
        auto& modMatrix = processorRef.getModulationMatrix();
        if (modMatrix.isModulated(braidy::ModulationMatrix::TIMBRE)) {
            // Add LFO modulation visual feedback
            float lfoValue = modMatrix.getModulation(braidy::ModulationMatrix::TIMBRE);
            float lfoIntensity = std::abs(lfoValue) * 0.5f; // Scale down for visual effect
            brightness = std::clamp(brightness + lfoIntensity, 0.0f, 1.0f);
            
            // Pulsing effect for modulated parameters
            static float pulsePhase = 0.0f;
            pulsePhase += 0.1f;
            if (pulsePhase > juce::MathConstants<float>::twoPi) pulsePhase = 0.0f;
            float pulse = (std::sin(pulsePhase) + 1.0f) * 0.15f; // 0-0.3 range
            brightness = std::clamp(brightness + pulse, 0.0f, 1.0f);
        }
        
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
        
        // Check if COLOR is being modulated by LFO
        auto& modMatrix = processorRef.getModulationMatrix();
        if (modMatrix.isModulated(braidy::ModulationMatrix::COLOR)) {
            // Add LFO modulation visual feedback
            float lfoValue = modMatrix.getModulation(braidy::ModulationMatrix::COLOR);
            float lfoIntensity = std::abs(lfoValue) * 0.5f; // Scale down for visual effect
            brightness = std::clamp(brightness + lfoIntensity, 0.0f, 1.0f);
            
            // Pulsing effect for modulated parameters
            static float pulsePhase = 0.0f;
            pulsePhase += 0.12f; // Slightly different phase for COLOR
            if (pulsePhase > juce::MathConstants<float>::twoPi) pulsePhase = 0.0f;
            float pulse = (std::sin(pulsePhase) + 1.0f) * 0.15f; // 0-0.3 range
            brightness = std::clamp(brightness + pulse, 0.0f, 1.0f);
        }
        
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
    
    // FM LED (blue - unique color for FM parameter)
    if (fmKnob_) {
        auto fmBounds = fmKnob_->getBounds();
        auto ledBounds = juce::Rectangle<int>(5, 5).withCentre({fmBounds.getCentreX(), fmBounds.getY() - 10});
        
        auto brightness = fmKnob_->getValue();
        
        // Check if FM is being modulated by LFO
        auto& modMatrix = processorRef.getModulationMatrix();
        if (modMatrix.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
            // Add LFO modulation visual feedback
            float lfoValue = modMatrix.getModulation(braidy::ModulationMatrix::FM_AMOUNT);
            float lfoIntensity = std::abs(lfoValue) * 0.5f; // Scale down for visual effect
            brightness = std::clamp(brightness + lfoIntensity, 0.0f, 1.0f);
            
            // Pulsing effect for modulated parameters
            static float pulsePhase = 0.0f;
            pulsePhase += 0.15f; // Different phase for FM
            if (pulsePhase > juce::MathConstants<float>::twoPi) pulsePhase -= juce::MathConstants<float>::twoPi;
            float pulse = (std::sin(pulsePhase) + 1.0f) * 0.3f; // Pulse between 0.0f and 0.6f
            brightness = std::clamp(brightness + pulse, 0.0f, 1.0f);
        }
        
        auto ledColour = juce::Colour(0xFF4A90E2).withAlpha(0.4f + brightness * 0.6f);  // Blue color
        
        g.setColour(ledColour);
        g.fillEllipse(ledBounds.toFloat());
        
        // LED rim (brighter blue)
        g.setColour(juce::Colour(0xFF4A90E2));
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
        modulationOverlay_->toFront(true);  // Keep it on top
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
            // Ensure we don't access out of bounds (limit to 0-46 for all algorithms)
            if (currentAlgorithm_ >= 0 && currentAlgorithm_ < static_cast<int>(algorithmNames_.size())) {
                displayText = algorithmNames_[currentAlgorithm_];
                static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                if (fileLogger_) {
                    fileLogger_->logMessage("[DISPLAY] Showing Algorithm [" + juce::String(currentAlgorithm_) + 
                        "]: " + displayText);
                }
            } else {
                // Reset to safe value and display error
                displayText = "----";
                static_cast<SimpleOLEDDisplay*>(oledDisplay_.get())->setText(displayText);
                if (fileLogger_) {
                    fileLogger_->logMessage("[DISPLAY] ERROR: Algorithm index out of bounds: " + 
                        juce::String(currentAlgorithm_) + " - resetting to 0");
                }
                currentAlgorithm_ = 0;  // Reset to safe value
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
                    if (pageIndex >= 0 && pageIndex < 24) {  // We have 24 valid menu pages (0-23)
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
                if (isShuttingDown_) return;  // Don't execute if shutting down
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
    // Skip most parameter updates if settings panel is open to avoid audio interruption
    bool overlayVisible = modulationOverlay_ && modulationOverlay_->isVisible();
    
    // Sync knob positions with parameter values
    auto& apvts = processorRef.getAPVTS();
    
    // Update algorithm from parameter when it changes from the host (e.g., Logic dropdown)
    // We need to detect when the parameter changes from outside the plugin UI
    if (!overlayVisible) {
        if (auto* algorithmParam = apvts.getRawParameterValue("algorithm")) {
            // AudioParameterChoice stores the actual index, not a normalized value!
            int paramAlgorithm = static_cast<int>(algorithmParam->load());
            
            // Only update if the parameter differs from our current UI state
            // This prevents feedback loops while still syncing with host changes
            // IMPORTANT: Add flag to prevent infinite loop from loadModelDefaults triggering parameter updates
            static bool updatingFromParameterSync = false;
            
            if (paramAlgorithm != currentAlgorithm_ && !updatingAlgorithmFromEncoder_ && 
                !updatingFromParameterSync && editEncoder_ && !editEncoder_->isBeingManipulated()) {
                
                // Set flag to prevent re-entry
                updatingFromParameterSync = true;
                
                // Host changed the algorithm (e.g., via Logic dropdown)
                currentAlgorithm_ = paramAlgorithm;
                
                // Validate bounds
                if (currentAlgorithm_ < 0 || currentAlgorithm_ >= static_cast<int>(algorithmNames_.size())) {
                    currentAlgorithm_ = 0;
                }
                
                // Update display to reflect the change
                updateDisplay();
                
                // Only load defaults if META mode is NOT active
                // When META mode is cycling, we don't want to reset parameters
                bool metaModeActive = false;
                if (auto* braidyProcessor = dynamic_cast<BraidyAudioProcessor*>(&processor)) {
                    if (auto* metaModeParam = braidyProcessor->getAPVTS().getRawParameterValue("metaMode")) {
                        metaModeActive = metaModeParam->load() > 0.5f;
                    }
                }
                
                if (!metaModeActive) {
                    // Load defaults for the new algorithm (this can trigger parameter changes)
                    loadModelDefaults(currentAlgorithm_);
                }
                
                if (fileLogger_) {
                    fileLogger_->logMessage("[PARAM SYNC] Algorithm changed from host to: " + 
                        juce::String(algorithmNames_[currentAlgorithm_]) + " (" + juce::String(currentAlgorithm_) + ")");
                }
                
                // Clear flag after processing
                updatingFromParameterSync = false;
            }
        }
    }
    
    // ALWAYS update knobs with modulation (not just when overlay is hidden!)
    // This is what makes the knobs move with LFO modulation
    
    // Update Timbre knob with modulation (skip only when knob being manually manipulated)
    if (timbreKnob_ && !timbreKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedTimbre();
        float currentKnobValue = timbreKnob_->getValue();
        
        // Only update if value actually changed to reduce CPU usage
        if (std::abs(modulatedValue - currentKnobValue) > 0.001f) {
            timbreKnob_->setValue(modulatedValue);  // No callback triggered
            
            // Debug logging to verify modulation is working
            static int logCounter = 0;
            if (++logCounter % 30 == 0) {  // Log every 30 frames (~1 second at 30Hz)
                if (fileLogger_) {
                    fileLogger_->logMessage("[MODULATION] Timbre knob updated: " + 
                        juce::String(currentKnobValue, 3) + " -> " + juce::String(modulatedValue, 3));
                }
            }
        }
    }
    
    // Update Color knob with modulation (skip only when knob being manually manipulated)
    if (colorKnob_ && !colorKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedColor();
        float currentKnobValue = colorKnob_->getValue();
        
        // Only update if value actually changed to reduce CPU usage
        if (std::abs(modulatedValue - currentKnobValue) > 0.001f) {
            colorKnob_->setValue(modulatedValue);  // No callback triggered
            
            // Debug logging to verify modulation is working
            static int logCounter = 0;
            if (++logCounter % 30 == 0) {  // Log every 30 frames (~1 second at 30Hz)
                if (fileLogger_) {
                    fileLogger_->logMessage("[MODULATION] Color knob updated: " + 
                        juce::String(currentKnobValue, 3) + " -> " + juce::String(modulatedValue, 3));
                }
            }
        }
    }
    
    // Update FM knob with modulation and meta mode algorithm display
    if (fmKnob_ && !fmKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedFM();
        float currentKnobValue = fmKnob_->getValue();
        
        // Only update if value actually changed to reduce CPU usage
        if (std::abs(modulatedValue - currentKnobValue) > 0.001f) {
            fmKnob_->setValue(modulatedValue);  // No callback triggered
            
            // Debug logging to verify modulation is working
            static int logCounter = 0;
            if (++logCounter % 30 == 0) {  // Log every 30 frames (~1 second at 30Hz)
                if (fileLogger_) {
                    fileLogger_->logMessage("[MODULATION] FM knob updated: " + 
                        juce::String(currentKnobValue, 3) + " -> " + juce::String(modulatedValue, 3));
                }
            }
        }
                
                // Algorithm display is now handled in timerCallback() for META mode
    }
    
    // Update other knobs with modulation
    if (fineKnob_ && !fineKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedFine();
        fineKnob_->setValue(modulatedValue);  // No callback triggered
    }
    
    if (coarseKnob_ && !coarseKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedCoarse();
        coarseKnob_->setValue(modulatedValue);  // No callback triggered
    }
    
    if (timbreModKnob_ && !timbreModKnob_->isBeingManipulated()) {
        // Get modulated value from processor (includes LFO if active)
        float modulatedValue = processorRef.getModulatedTimbreMod();
        timbreModKnob_->setValue(modulatedValue);  // No callback triggered
    }
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
        fileLogger_->logMessage("Current Algorithm Index: " + juce::String(currentAlgorithm_));
        
        // Log the conceptual rotation position (treating 47 algorithms as positions on a dial)
        // With 47 algorithms, each would be ~7.66 degrees apart for full 360 coverage
        // But we're making it change every 90 degrees for easier control
        int quadrant = (currentAlgorithm_ % 4);  // Which 90-degree quadrant
        juce::String position = (quadrant == 0 ? "12 o'clock" :
                                 quadrant == 1 ? "3 o'clock" :
                                 quadrant == 2 ? "6 o'clock" : "9 o'clock");
        fileLogger_->logMessage("Encoder Quadrant: " + position + " (Alg " + juce::String(currentAlgorithm_) + ")");
    }
    
    switch (displayMode_) {
        case DisplayMode::Algorithm:
            {
                // CRITICAL FIX: Check if algorithm change is already pending to prevent race conditions
                if (algorithmChangePending_.load()) {
                    if (fileLogger_) {
                        fileLogger_->logMessage("[RACE PROTECTION] Algorithm change already pending, ignoring encoder input");
                    }
                    return; // Ignore encoder input until previous change completes
                }
                
                int oldAlgorithm = currentAlgorithm_;
                // Navigate algorithms with wrapping (like original Braids)
                currentAlgorithm_ += delta;
                
                // Wrap around at the boundaries (47 algorithms total, index 0-46)
                if (currentAlgorithm_ < 0) {
                    currentAlgorithm_ = 46;  // Wrap to last algorithm (index 46)
                } else if (currentAlgorithm_ > 46) {
                    currentAlgorithm_ = 0;   // Wrap to first algorithm (index 0)
                }
                
                // Validate algorithm bounds before proceeding
                if (currentAlgorithm_ < 0 || currentAlgorithm_ >= static_cast<int>(algorithmNames_.size())) {
                    if (fileLogger_) {
                        fileLogger_->logMessage("[ERROR] Algorithm index out of bounds: " + juce::String(currentAlgorithm_) + ", resetting to 0");
                    }
                    currentAlgorithm_ = 0;
                }
                
                // Safe bounds check before accessing array
                if (fileLogger_) {
                    juce::String oldName = "INVALID";
                    juce::String newName = "INVALID";
                    
                    if (oldAlgorithm >= 0 && oldAlgorithm < static_cast<int>(algorithmNames_.size())) {
                        oldName = algorithmNames_[oldAlgorithm];
                    }
                    if (currentAlgorithm_ >= 0 && currentAlgorithm_ < static_cast<int>(algorithmNames_.size())) {
                        newName = algorithmNames_[currentAlgorithm_];
                    }
                    
                    fileLogger_->logMessage("Algorithm changed from [" + juce::String(oldAlgorithm) + "] " + 
                        oldName + " to [" + juce::String(currentAlgorithm_) + "] " + newName);
                }
                
                // THREAD SAFETY FIX: Set atomic flag before making changes
                algorithmChangePending_.store(true);
                pendingAlgorithmChange_.store(currentAlgorithm_);
                
                // Update the audio parameter thread-safely
                updateAlgorithmParameter();
                
                // Only load defaults if META mode is NOT active
                bool isMetaModeActive = false;
                if (auto* braidyProcessor = dynamic_cast<BraidyAudioProcessor*>(&processor)) {
                    if (auto* metaModeParam = braidyProcessor->getAPVTS().getRawParameterValue("metaMode")) {
                        isMetaModeActive = metaModeParam->load() > 0.5f;
                    }
                }
                
                if (!isMetaModeActive) {
                    // Load default parameters for the new algorithm
                    loadModelDefaults(currentAlgorithm_);
                }
                
                // Clear the pending flag after safe completion
                algorithmChangePending_.store(false);
                
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
                    // Settings are automatically saved via APVTS getStateInformation
                    // This happens when the DAW project is saved
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
            // Read current meta mode parameter value instead of defaulting to 0
            if (auto* metaParam = processorRef.getAPVTS().getRawParameterValue("metaMode")) {
                menuValue_ = (metaParam->load() > 0.5f) ? 1 : 0;
            } else {
                menuValue_ = 0;  // Fallback if parameter doesn't exist
            }
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
            menuValue_ = 0;  // Default VCA mode
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
    
    // Simplified parameter application - just use APVTS parameters for now
    switch (currentMenuPage_) {
        case MenuPage::WAVE:
            // WAVE saves settings and exits - handled in click handler
            break;
        case MenuPage::META:
            if (auto* param = apvts.getParameter("metaMode")) {
                param->setValueNotifyingHost(menuValue_ > 0 ? 1.0f : 0.0f);
                // Sync with modulation overlay
                if (modulationOverlay_) {
                    modulationOverlay_->setMetaModeState(menuValue_ > 0);
                }
            }
            break;
        case MenuPage::BITS:
            if (auto* param = apvts.getParameter("bitDepth")) {
                param->setValueNotifyingHost(menuValue_ / 16.0f);
            }
            break;
        case MenuPage::RATE:
            if (auto* param = apvts.getParameter("sampleRate")) {
                param->setValueNotifyingHost(menuValue_ / 96.0f);
            }
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
        default:
            // For other parameters, we'll just store the values locally for now
            DBG("Applied menu value " + juce::String(menuValue_) + " for menu item " + 
                juce::String(static_cast<int>(currentMenuPage_)));
            break;
    }
}

void BraidyAudioProcessorEditor::updateAlgorithmParameter() {
    DBG("=== UPDATE ALGORITHM PARAMETER DEBUG ===");
    DBG("Current algorithm index: " + juce::String(currentAlgorithm_));
    
    // Set flag to prevent feedback loops during parameter update
    // Use RAII to ensure flag is always cleared
    struct FlagGuard {
        bool& flag;
        FlagGuard(bool& f) : flag(f) { flag = true; }
        ~FlagGuard() { flag = false; }
    } guard(updatingAlgorithmFromEncoder_);
    
    // THREAD SAFETY: Double-check bounds even more strictly
    if (currentAlgorithm_ < 0 || currentAlgorithm_ >= 47) { // Braids has exactly 47 algorithms (0-46)
        if (fileLogger_) {
            fileLogger_->logMessage("[CRITICAL ERROR] Algorithm index completely out of bounds: " + 
                juce::String(currentAlgorithm_) + " - resetting to safe value");
        }
        DBG("CRITICAL ERROR: Algorithm index completely out of bounds: " + juce::String(currentAlgorithm_));
        currentAlgorithm_ = 0;  // Reset to safe value (CSAW)
        pendingAlgorithmChange_.store(0); // Update atomic as well
    }
    
    // Safe bounds check before accessing array
    if (currentAlgorithm_ >= 0 && currentAlgorithm_ < static_cast<int>(algorithmNames_.size())) {
        DBG("Algorithm: " + juce::String(algorithmNames_[currentAlgorithm_]));
        if (fileLogger_) {
            fileLogger_->logMessage("[ALGORITHM UPDATE] Setting to: " + juce::String(algorithmNames_[currentAlgorithm_]) + 
                " (index " + juce::String(currentAlgorithm_) + ")");
        }
    } else {
        DBG("Algorithm: INVALID INDEX (" + juce::String(currentAlgorithm_) + ")");
        if (fileLogger_) {
            fileLogger_->logMessage("[ERROR] Invalid algorithm index after bounds check: " + juce::String(currentAlgorithm_));
        }
        currentAlgorithm_ = 0;  // Reset to safe value
        pendingAlgorithmChange_.store(0); // Update atomic as well
    }
    
    // THREAD SAFETY: Use message manager to safely communicate with audio thread
    // This prevents direct manipulation during audio processing
    juce::MessageManager::callAsync([this]() {
        // Update the algorithm parameter in APVTS based on current algorithm
        if (auto* param = processorRef.getAPVTS().getParameter("algorithm")) {
            // Always use normalized value for parameters
            float normalizedValue = static_cast<float>(currentAlgorithm_) / 46.0f;
            normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue); // Extra safety clamp
            
            DBG("Found 'algorithm' parameter in APVTS");
            DBG("Setting algorithm index: " + juce::String(currentAlgorithm_));
            DBG("Normalized value: " + juce::String(normalizedValue, 4));
            
            float oldValue = param->getValue();
            
            // Use proper change gesture for automation
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalizedValue);
            param->endChangeGesture();
            
            float newValue = param->getValue();
            
            DBG("Parameter value changed from " + juce::String(oldValue, 4) + " to " + juce::String(newValue, 4));
            
            if (fileLogger_) {
                fileLogger_->logMessage("[PARAMETER CHANGE] Algorithm parameter updated: " + 
                    juce::String(oldValue, 4) + " -> " + juce::String(newValue, 4));
            }
        } else {
            DBG("ERROR: 'algorithm' parameter NOT FOUND in APVTS!");
            if (fileLogger_) {
                fileLogger_->logMessage("[ERROR] Algorithm parameter not found in APVTS!");
            }
            
            // List all available parameters
            DBG("Available parameters:");
            for (auto* param : processorRef.getParameters()) {
                if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param)) {
                    DBG("  - " + rangedParam->getParameterID());
                }
            }
        }
    });
}

//==============================================================================
// JUCE Component Callbacks (unused but required)
//==============================================================================
void BraidyAudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    // Not used - we use custom knobs
}

void BraidyAudioProcessorEditor::buttonClicked(juce::Button* button) {
    if (button == settingsButton_.get()) {
        DBG("MOD button pressed");
        if (modulationOverlay_) {
            if (modulationOverlay_->isOverlayVisible()) {
                modulationOverlay_->hideOverlay();
            } else {
                // Sync META mode state with current parameter before showing
                if (auto* metaParam = processorRef.getAPVTS().getRawParameterValue("metaMode")) {
                    bool isEnabled = metaParam->load() > 0.5f;
                    modulationOverlay_->setMetaModeState(isEnabled);
                }
                modulationOverlay_->showOverlay();
                modulationOverlay_->toFront(true);  // Bring to front when showing
            }
        }
    }
}

void BraidyAudioProcessorEditor::timerCallback() {
    // Update parameter values from processor
    updateParameterValues();
    
    // Update algorithm display
    if (displayMode_ == DisplayMode::Algorithm) {
        int displayAlgorithm = currentAlgorithm_;
        
        // In META mode, show the modulated algorithm in the display
        // but DON'T update currentAlgorithm_ (which controls the knob position)
        if (processorRef.isMetaModeEnabled()) {
            displayAlgorithm = processorRef.getMetaModeAlgorithm();
        } else {
            displayAlgorithm = processorRef.getCurrentAlgorithm();
            // Only sync currentAlgorithm_ when NOT in META mode
            if (displayAlgorithm != currentAlgorithm_) {
                currentAlgorithm_ = displayAlgorithm;
            }
        }
        
        // Only update display if algorithm actually changed
        static int lastDisplayedAlgorithm = -1;
        if (displayAlgorithm != lastDisplayedAlgorithm) {
            lastDisplayedAlgorithm = displayAlgorithm;
            // DON'T update currentAlgorithm_ here when in META mode
            // currentAlgorithm_ = displayAlgorithm;  // REMOVED THIS LINE
            
            // Update display to show current/modulated algorithm
            if (oledDisplay_ && displayAlgorithm >= 0 && displayAlgorithm < static_cast<int>(algorithmNames_.size())) {
                if (auto* display = dynamic_cast<SimpleOLEDDisplay*>(oledDisplay_.get())) {
                    display->setText(algorithmNames_[displayAlgorithm]);
                    
                    // Debug output for algorithm changes
                    if (processorRef.isMetaModeEnabled()) {
                        DBG("[META MODE DISPLAY] Algorithm display updated to: " + 
                            juce::String(algorithmNames_[displayAlgorithm]) + " (" + juce::String(displayAlgorithm) + ")");
                    }
                }
            }
        }
    }
    
    // Don't automatically grab keyboard focus in timer
    // This interferes with host (Logic) keyboard shortcuts like Cmd+K
    
    // Repaint LEDs to show parameter activity
    // With 33Hz timer, repaint every 2 ticks = ~60ms for smooth LED updates
    static int repaintCounter = 0;
    if (++repaintCounter >= 2) {  
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
    // Don't automatically grab keyboard focus - this interferes with Logic's keyboard shortcuts
    // Users can still use the plugin's built-in keyboard support when the plugin is focused
    // but we won't steal focus from the host
    DBG("[MOUSE] Window clicked - not grabbing focus to preserve host shortcuts");
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
            if (isShuttingDown_) return;  // Don't execute if shutting down
            this->checkForReleasedKeys();
        });
        return true;
    }
    return false;
}

void BraidyAudioProcessorEditor::sendMidiNoteOn(int midiNote, float velocity) {
    // Send MIDI note via the processor's MIDI collector
    DBG("[MIDI SEND] Note ON: " + juce::String(midiNote) + " velocity: " + juce::String(velocity, 2));
    
    auto midiMessage = juce::MidiMessage::noteOn(1, midiNote, juce::uint8(velocity * 127.0f));
    midiMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    processorRef.getMidiCollector().addMessageToQueue(midiMessage);
}

void BraidyAudioProcessorEditor::sendMidiNoteOff(int midiNote) {
    // Send MIDI note off via the processor's MIDI collector
    DBG("[MIDI SEND] Note OFF: " + juce::String(midiNote));
    
    auto midiMessage = juce::MidiMessage::noteOff(1, midiNote);
    midiMessage.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    processorRef.getMidiCollector().addMessageToQueue(midiMessage);
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

void BraidyAudioProcessorEditor::loadModelDefaults(int algorithmIndex) {
    // THREAD SAFETY: Check if we're in the middle of an algorithm change
    if (algorithmChangePending_.load()) {
        if (fileLogger_) {
            fileLogger_->logMessage("[DEFAULTS] Algorithm change pending, deferring default loading");
        }
        // Defer the load until the algorithm change completes
        juce::Timer::callAfterDelay(50, [this, algorithmIndex]() {
            loadModelDefaults(algorithmIndex);
        });
        return;
    }
    
    // Ensure algorithm index is valid (0-46 for 47 algorithms in BraidsDefaults)
    // CRITICAL: BraidsDefaults::MODEL_DEFAULTS has exactly 47 entries (indices 0-46)
    if (algorithmIndex < 0 || algorithmIndex >= static_cast<int>(BraidsDefaults::MODEL_DEFAULTS.size())) {
        if (fileLogger_) {
            fileLogger_->logMessage("[DEFAULTS] ERROR: Algorithm index out of bounds: " + 
                juce::String(algorithmIndex) + " (max: " + 
                juce::String(BraidsDefaults::MODEL_DEFAULTS.size() - 1) + ") - aborting load");
        }
        return;
    }
    
    // Additional safety check for PLUK and other problematic algorithms
    if (algorithmIndex == 28) { // PLUK is at index 28
        if (fileLogger_) {
            fileLogger_->logMessage("[DEFAULTS] Loading PLUK algorithm (index 28) with enhanced safety checks");
        }
        // PLUK needs special handling - ensure synthesiser is in a stable state
        if (auto* synth = processorRef.getSynthesiser()) {
            // Stop all active voices before loading PLUK defaults to prevent crashes
            synth->allNotesOff(0, true); // Force stop all notes
            
            // Small delay to let voices stop cleanly
            juce::Timer::callAfterDelay(10, [this, algorithmIndex]() {
                loadModelDefaultsSafe(algorithmIndex);
            });
            return;
        }
    }
    
    // For non-PLUK algorithms or if we can't access synthesiser, load directly
    loadModelDefaultsSafe(algorithmIndex);
}

void BraidyAudioProcessorEditor::loadModelDefaultsSafe(int algorithmIndex) {
    // This is the actual implementation, called after safety checks
    if (algorithmIndex < 0 || algorithmIndex >= static_cast<int>(BraidsDefaults::MODEL_DEFAULTS.size())) {
        return; // Already logged error above
    }
    
    // Get the model defaults for this algorithm
    const auto& defaults = BraidsDefaults::MODEL_DEFAULTS[algorithmIndex];
    
    // THREAD SAFETY: Use async message to avoid race conditions with audio thread
    juce::MessageManager::callAsync([this, algorithmIndex, defaults]() {
        // Update parameter values through APVTS (thread-safe)
        if (auto* param1 = processorRef.getAPVTS().getParameter("param1")) {
            param1->beginChangeGesture();
            param1->setValueNotifyingHost(defaults.timbre);
            param1->endChangeGesture();
        }
        
        if (auto* param2 = processorRef.getAPVTS().getParameter("param2")) {
            param2->beginChangeGesture();
            param2->setValueNotifyingHost(defaults.color);
            param2->endChangeGesture();
        }
        
        // Update knob positions to reflect new values
        // Only update if knobs are not being manually manipulated to prevent jumping
        if (timbreKnob_ && !timbreKnob_->isBeingManipulated()) {
            timbreKnob_->setValue(defaults.timbre);
        }
        
        if (colorKnob_ && !colorKnob_->isBeingManipulated()) {
            colorKnob_->setValue(defaults.color);
        }
        
        // Preserve current FM and modulation knob positions to avoid jumping
        // Only reset if knobs are not initialized yet
        if (fmKnob_ && fmKnob_->getValue() == 0.0f) {
            fmKnob_->setValue(defaults.fm);  // Use algorithm-specific default instead of hardcoded 0.5f
        }
        if (timbreModKnob_ && timbreModKnob_->getValue() == 0.5f) {
            timbreModKnob_->setValue(defaults.modulation);  // Use algorithm-specific default
        }
        // Note: colorModKnob_ doesn't exist in current implementation
        
        if (fileLogger_) {
            fileLogger_->logMessage("[DEFAULTS] Loaded defaults for " + juce::String(defaults.name) + 
                                     " - Timbre: " + juce::String(defaults.timbre) + 
                                     ", Color: " + juce::String(defaults.color));
        }
    });
    
    // Handle percussion trigger separately to avoid blocking the UI
    if (defaults.isPercussive && processorRef.getSynthesiser()) {
        // Send a trigger/strike to the synthesizer for percussion models
        // This triggers a short C4 note to demonstrate the percussion sound
        juce::Timer::callAfterDelay(200, [this]() { // Delay to let algorithm change settle
            if (isShuttingDown_) return;  // Don't execute if shutting down
            sendMidiNoteOn(60, 0.8f);  // C4 with velocity 0.8
            juce::Timer::callAfterDelay(100, [this]() {
                if (isShuttingDown_) return;  // Don't execute if shutting down
                sendMidiNoteOff(60);
            });
        });
    }
}