#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BraidyAudioProcessorEditor::BraidyAudioProcessorEditor (BraidyAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Set window size
    setSize (600, 400);
    
    // Setup title
    titleLabel.setText("Braidy Synthesizer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Setup parameter controls
    setupSlider(shapeSlider, shapeLabel, "Algorithm");
    shapeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    shapeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    shapeSlider.setRange(0, 47, 1);  // 48 algorithms for now (Phase 1)
    setupSliderAttachment(shapeAttachment, shapeSlider, "alg");
    
    setupSlider(timbreSlider, timbreLabel, "Timbre");
    timbreSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    timbreSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    setupSliderAttachment(timbreAttachment, timbreSlider, "tmb");
    
    setupSlider(colorSlider, colorLabel, "Color");
    colorSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    colorSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    setupSliderAttachment(colorAttachment, colorSlider, "col");
    
    setupSlider(volumeSlider, volumeLabel, "Volume");
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    setupSliderAttachment(volumeAttachment, volumeSlider, "vol");
    
    setupSlider(attackSlider, attackLabel, "Attack");
    attackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    setupSliderAttachment(attackAttachment, attackSlider, "att");
    
    setupSlider(decaySlider, decayLabel, "Decay");
    decaySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    setupSliderAttachment(decayAttachment, decaySlider, "dec");
    
    // Setup status display
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(14.0f));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
}

BraidyAudioProcessorEditor::~BraidyAudioProcessorEditor() {
}

void BraidyAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText) {
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    
    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.attachToComponent(&slider, false);
}

void BraidyAudioProcessorEditor::setupSliderAttachment(
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,
    juce::Slider& slider, const juce::String& paramId) {
    
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), paramId, slider);
}

//==============================================================================
void BraidyAudioProcessorEditor::paint (juce::Graphics& g) {
    // Background gradient
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    auto bounds = getLocalBounds();
    
    // Create a subtle gradient background
    juce::ColourGradient gradient(juce::Colour(0xff2d2d2d), 0, 0,
                                  juce::Colour(0xff1a1a1a), 0, getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);
    
    // Draw border
    g.setColour(juce::Colour(0xff444444));
    g.drawRect(bounds, 2);
    
    // Draw sections
    g.setColour(juce::Colour(0xff333333));
    
    // Main controls section
    auto mainArea = bounds.removeFromTop(bounds.getHeight() - 60).reduced(20);
    g.drawRect(mainArea, 1);
    
    // Status section
    auto statusArea = bounds.reduced(20, 10);
    g.drawRect(statusArea, 1);
}

void BraidyAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Title area
    auto titleArea = bounds.removeFromTop(50);
    titleLabel.setBounds(titleArea.reduced(20, 10));
    
    // Main controls area
    auto mainArea = bounds.removeFromTop(bounds.getHeight() - 60).reduced(30);
    
    // Top row - main synthesis parameters (rotary knobs)
    auto topRow = mainArea.removeFromTop(120);
    int knobWidth = topRow.getWidth() / 3;
    
    auto shapeArea = topRow.removeFromLeft(knobWidth).reduced(10);
    shapeSlider.setBounds(shapeArea.removeFromBottom(100));
    
    auto timbreArea = topRow.removeFromLeft(knobWidth).reduced(10);
    timbreSlider.setBounds(timbreArea.removeFromBottom(100));
    
    auto colorArea = topRow.reduced(10);
    colorSlider.setBounds(colorArea.removeFromBottom(100));
    
    // Bottom section - envelope and volume (horizontal sliders)
    mainArea.removeFromTop(20);  // Spacing
    
    auto sliderHeight = 40;
    auto labelWidth = 80;
    
    auto volumeArea = mainArea.removeFromTop(sliderHeight);
    volumeArea.removeFromLeft(labelWidth);
    volumeSlider.setBounds(volumeArea.reduced(0, 5));
    
    auto attackArea = mainArea.removeFromTop(sliderHeight);
    attackArea.removeFromLeft(labelWidth);
    attackSlider.setBounds(attackArea.reduced(0, 5));
    
    auto decayArea = mainArea.removeFromTop(sliderHeight);
    decayArea.removeFromLeft(labelWidth);
    decaySlider.setBounds(decayArea.reduced(0, 5));
    
    // Status area
    auto statusArea = bounds.reduced(20, 10);
    statusLabel.setBounds(statusArea);
    
    // Update status with voice info
    int activeVoices = processorRef.getVoiceManager().GetActiveVoiceCount();
    statusLabel.setText("Active Voices: " + juce::String(activeVoices) + 
                       " | Algorithm: " + juce::String(static_cast<int>(processorRef.getBraidySettings().GetShape())),
                       juce::dontSendNotification);
}