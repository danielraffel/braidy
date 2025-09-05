#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// Algorithm names for the 4-character display
const juce::StringArray BraidyAudioProcessorEditor::algorithmNames_ = {
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

//==============================================================================
BraidyAudioProcessorEditor::BraidyAudioProcessorEditor(BraidyAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
    , currentDisplayState_(DisplayState::Algorithm)
    , menuSelection_(0)
    , settingsMode_(false)
    , visualizerWritePos_(0)
    , panelColour_(juce::Colour(0xff2a2a2a))       // Braids gray
    , highlightColour_(juce::Colour(0xff00ff40))    // Braids green
    , textColour_(juce::Colour(0xffcccccc))        // Light gray
    , accentColour_(juce::Colour(0xff004020))      // Dark green
{
    // Set window size to match modular aesthetic
    setSize(480, 360);  // Slightly larger for better visibility
    
    setupComponents();
    updateDisplay();
    
    // Start timer for regular updates
    startTimer(50);  // 20 Hz refresh rate
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
    stopTimer();
}

void BraidyAudioProcessorEditor::setupComponents() {
    // Create display
    display_ = std::make_unique<braidy::BraidyDisplay>();
    display_->setDisplayMode(braidy::BraidyDisplay::DisplayMode::Normal);
    display_->setBrightness(0.8f);
    addAndMakeVisible(*display_);
    
    // Create encoders
    algorithmEncoder_ = std::make_unique<braidy::BraidyEncoder>();
    algorithmEncoder_->setRange(0, 47);  // 48 algorithms
    algorithmEncoder_->addListener(this);
    addAndMakeVisible(*algorithmEncoder_);
    
    timbreEncoder_ = std::make_unique<braidy::BraidyEncoder>();
    timbreEncoder_->setRange(0, 127);
    timbreEncoder_->addListener(this);
    addAndMakeVisible(*timbreEncoder_);
    
    colorEncoder_ = std::make_unique<braidy::BraidyEncoder>();
    colorEncoder_->setRange(0, 127);
    colorEncoder_->addListener(this);
    addAndMakeVisible(*colorEncoder_);
    
    // Create visualizer
    visualizer_ = std::make_unique<braidy::BraidyVisualizer>();
    visualizer_->setDisplayMode(braidy::BraidyVisualizer::DisplayMode::Waveform);
    visualizer_->setSampleRate(48000.0);
    visualizer_->setTraceColour(highlightColour_);
    visualizer_->setGridColour(panelColour_.brighter(0.3f));
    visualizer_->setBackgroundColour(panelColour_.darker(0.5f));
    addAndMakeVisible(*visualizer_);
    
    // Setup labels
    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(10.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, textColour_);
        addAndMakeVisible(label);
    };
    
    setupLabel(algorithmLabel_, "ALGORITHM");
    setupLabel(timbreLabel_, "TIMBRE");
    setupLabel(colorLabel_, "COLOR");
    
    // Title
    titleLabel_.setText("BRAIDY", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, highlightColour_);
    addAndMakeVisible(titleLabel_);
    
    // Initialize visualizer buffer
    visualizerBuffer_.setSize(1, 1024);
    visualizerBuffer_.clear();
    
    // Load initial parameter values
    updateParameterValues();
}

void BraidyAudioProcessorEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Main background - dark gray like Braids
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Draw modular background
    drawModularBackground(g, bounds);
    
    // Draw connection lines between components
    drawConnections(g);
    
    // Draw status LEDs
    drawLEDs(g);
}

void BraidyAudioProcessorEditor::drawModularBackground(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Panel background
    g.setColour(panelColour_);
    g.fillRoundedRectangle(bounds.toFloat().reduced(8), 6.0f);
    
    // Panel border
    g.setColour(panelColour_.brighter(0.2f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(8), 6.0f, 1.5f);
    
    // Mounting screws (aesthetic detail)
    g.setColour(panelColour_.darker(0.3f));
    auto screwRadius = 3.0f;
    
    // Corner screws
    g.fillEllipse(bounds.getX() + 15 - screwRadius, bounds.getY() + 15 - screwRadius, 
                  screwRadius * 2, screwRadius * 2);
    g.fillEllipse(bounds.getRight() - 15 - screwRadius, bounds.getY() + 15 - screwRadius, 
                  screwRadius * 2, screwRadius * 2);
    g.fillEllipse(bounds.getX() + 15 - screwRadius, bounds.getBottom() - 15 - screwRadius, 
                  screwRadius * 2, screwRadius * 2);
    g.fillEllipse(bounds.getRight() - 15 - screwRadius, bounds.getBottom() - 15 - screwRadius, 
                  screwRadius * 2, screwRadius * 2);
    
    // Screw highlights
    g.setColour(panelColour_.brighter(0.1f));
    g.fillEllipse(bounds.getX() + 15 - screwRadius + 1, bounds.getY() + 15 - screwRadius + 1, 
                  screwRadius, screwRadius);
    g.fillEllipse(bounds.getRight() - 15 - screwRadius + 1, bounds.getY() + 15 - screwRadius + 1, 
                  screwRadius, screwRadius);
    g.fillEllipse(bounds.getX() + 15 - screwRadius + 1, bounds.getBottom() - 15 - screwRadius + 1, 
                  screwRadius, screwRadius);
    g.fillEllipse(bounds.getRight() - 15 - screwRadius + 1, bounds.getBottom() - 15 - screwRadius + 1, 
                  screwRadius, screwRadius);
}

void BraidyAudioProcessorEditor::drawConnections(juce::Graphics& g) {
    // Draw subtle connection lines between encoders and display (aesthetic)
    g.setColour(accentColour_);
    
    auto displayCenter = display_->getBounds().getCentre();
    auto algCenter = algorithmEncoder_->getBounds().getCentre();
    auto timbreCenter = timbreEncoder_->getBounds().getCentre();
    auto colorCenter = colorEncoder_->getBounds().getCentre();
    
    // Dotted lines to display
    juce::Path connectionPath;
    connectionPath.addLineSegment({algCenter.toFloat(), displayCenter.toFloat()}, 1.0f);
    connectionPath.addLineSegment({timbreCenter.toFloat(), displayCenter.toFloat()}, 1.0f);
    connectionPath.addLineSegment({colorCenter.toFloat(), displayCenter.toFloat()}, 1.0f);
    
    // Draw dashed connection line
    juce::PathStrokeType strokeType(0.5f);
    float dashLengths[] = {2.0f, 4.0f};
    strokeType.createDashedStroke(connectionPath, connectionPath, dashLengths, 2);
    g.strokePath(connectionPath, strokeType);
}

void BraidyAudioProcessorEditor::drawLEDs(juce::Graphics& g) {
    // Status LED near title
    auto titleBounds = titleLabel_.getBounds();
    auto ledBounds = juce::Rectangle<int>(6, 6).withCentre({titleBounds.getRight() + 15, titleBounds.getCentreY()});
    
    // LED based on voice activity
    int activeVoices = processorRef.getVoiceManager().GetActiveVoiceCount();
    juce::Colour ledColour = activeVoices > 0 ? highlightColour_ : panelColour_.darker();
    
    g.setColour(ledColour.darker(0.5f));
    g.fillEllipse(ledBounds.toFloat());
    
    g.setColour(ledColour);
    g.fillEllipse(ledBounds.reduced(1).toFloat());
    
    // Glow effect when active
    if (activeVoices > 0) {
        g.setColour(ledColour.withAlpha(0.3f));
        g.fillEllipse(ledBounds.expanded(2).toFloat());
    }
}

void BraidyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds().reduced(20);
    
    // Title at top
    auto titleArea = bounds.removeFromTop(25);
    titleLabel_.setBounds(titleArea.removeFromLeft(100));
    
    // Display below title
    auto displayArea = bounds.removeFromTop(50);
    display_->setBounds(displayArea.withSizeKeepingCentre(160, 40));
    
    bounds.removeFromTop(15);  // Spacing
    
    // Encoders in a row
    auto encoderArea = bounds.removeFromTop(100);
    int encoderSize = 80;
    int spacing = (encoderArea.getWidth() - (3 * encoderSize)) / 4;
    
    encoderArea.removeFromLeft(spacing);
    
    auto algArea = encoderArea.removeFromLeft(encoderSize);
    algorithmEncoder_->setBounds(algArea.removeFromBottom(encoderSize));
    algorithmLabel_.setBounds(algArea);
    
    encoderArea.removeFromLeft(spacing);
    
    auto timbreArea = encoderArea.removeFromLeft(encoderSize);
    timbreEncoder_->setBounds(timbreArea.removeFromBottom(encoderSize));
    timbreLabel_.setBounds(timbreArea);
    
    encoderArea.removeFromLeft(spacing);
    
    auto colorArea = encoderArea.removeFromLeft(encoderSize);
    colorEncoder_->setBounds(colorArea.removeFromBottom(encoderSize));
    colorLabel_.setBounds(colorArea);
    
    // Visualizer at bottom
    bounds.removeFromTop(15);  // Spacing
    visualizer_->setBounds(bounds.reduced(10));
}

void BraidyAudioProcessorEditor::encoderValueChanged(braidy::BraidyEncoder* encoder, int delta) {
    handleParameterChange(encoder, delta);
    updateDisplay();
}

void BraidyAudioProcessorEditor::encoderClicked(braidy::BraidyEncoder* encoder) {
    if (encoder == algorithmEncoder_.get()) {
        // Cycle through display modes
        switch (currentDisplayState_) {
            case DisplayState::Algorithm:
                currentDisplayState_ = DisplayState::Parameter;
                break;
            case DisplayState::Parameter:
                currentDisplayState_ = DisplayState::Visualizer;
                break;
            case DisplayState::Visualizer:
                currentDisplayState_ = DisplayState::Algorithm;
                break;
            case DisplayState::Menu:
                // Exit menu mode
                settingsMode_ = false;
                currentDisplayState_ = DisplayState::Algorithm;
                break;
        }
        updateDisplay();
    } else if (encoder == timbreEncoder_.get()) {
        // Toggle visualizer mode
        if (visualizer_->isVisible()) {
            auto currentMode = braidy::BraidyVisualizer::DisplayMode::Waveform;
            // Cycle through: Waveform -> Spectrum -> Oscilloscope -> Waveform
            visualizer_->setDisplayMode(currentMode);  // This would need proper cycling logic
        }
    }
}

void BraidyAudioProcessorEditor::encoderLongPressed(braidy::BraidyEncoder* encoder) {
    if (encoder == algorithmEncoder_.get()) {
        // Enter settings menu
        settingsMode_ = true;
        currentDisplayState_ = DisplayState::Menu;
        menuSelection_ = 0;
        updateDisplay();
    }
}

void BraidyAudioProcessorEditor::handleParameterChange(braidy::BraidyEncoder* encoder, int delta) {
    auto& apvts = processorRef.getAPVTS();
    
    if (encoder == algorithmEncoder_.get()) {
        auto* param = apvts.getParameter("alg");
        if (param) {
            float newValue = param->getValue() + (delta / 47.0f);
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, newValue));
        }
    } else if (encoder == timbreEncoder_.get()) {
        auto* param = apvts.getParameter("tmb");
        if (param) {
            float newValue = param->getValue() + (delta / 127.0f);
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, newValue));
        }
    } else if (encoder == colorEncoder_.get()) {
        auto* param = apvts.getParameter("col");
        if (param) {
            float newValue = param->getValue() + (delta / 127.0f);
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, newValue));
        }
    }
}

void BraidyAudioProcessorEditor::updateParameterValues() {
    auto& apvts = processorRef.getAPVTS();
    
    // Update encoder positions
    if (auto* algParam = apvts.getParameter("alg")) {
        int algValue = static_cast<int>(algParam->getValue() * 47);
        algorithmEncoder_->setValue(algValue, false);
    }
    
    if (auto* tmbParam = apvts.getParameter("tmb")) {
        int tmbValue = static_cast<int>(tmbParam->getValue() * 127);
        timbreEncoder_->setValue(tmbValue, false);
    }
    
    if (auto* colParam = apvts.getParameter("col")) {
        int colValue = static_cast<int>(colParam->getValue() * 127);
        colorEncoder_->setValue(colValue, false);
    }
}

void BraidyAudioProcessorEditor::updateDisplay() {
    switch (currentDisplayState_) {
        case DisplayState::Algorithm: {
            int algIndex = algorithmEncoder_->getValue();
            display_->setText(getAlgorithmName(algIndex));
            break;
        }
        
        case DisplayState::Parameter: {
            // Show parameter values
            int tmb = timbreEncoder_->getValue();
            int col = colorEncoder_->getValue();
            juce::String paramText = juce::String(tmb).paddedLeft('0', 2) + 
                                   juce::String(col).paddedLeft('0', 2);
            display_->setText(paramText);
            break;
        }
        
        case DisplayState::Menu: {
            // Show menu options
            juce::StringArray menuItems = {"SAVE", "LOAD", "INIT", "HELP"};
            if (menuSelection_ < menuItems.size()) {
                display_->setText(menuItems[menuSelection_]);
            }
            break;
        }
        
        case DisplayState::Visualizer:
            display_->setText("WAVE");
            break;
    }
}

juce::String BraidyAudioProcessorEditor::getAlgorithmName(int algorithmIndex) const {
    if (algorithmIndex >= 0 && algorithmIndex < algorithmNames_.size()) {
        return algorithmNames_[algorithmIndex];
    }
    return "----";
}

juce::String BraidyAudioProcessorEditor::getParameterDisplayText(const juce::String& paramId) const {
    auto& apvts = processorRef.getAPVTS();
    if (auto* param = apvts.getParameter(paramId)) {
        int value = static_cast<int>(param->getValue() * 127);
        return juce::String(value).paddedLeft('0', 3);
    }
    return "000";
}

void BraidyAudioProcessorEditor::timerCallback() {
    updateParameterValues();
    
    // Update visualizer with audio data (placeholder - would need actual audio routing)
    // This would typically be fed from the processor's audio output
    
    // For now, generate some test audio for the visualizer
    static float phase = 0.0f;
    float testSamples[64];
    for (int i = 0; i < 64; ++i) {
        testSamples[i] = std::sin(phase) * 0.5f;
        phase += 0.1f;
    }
    
    visualizer_->processAudioBlock(testSamples, 64);
}