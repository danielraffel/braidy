#pragma once

#include <JuceHeader.h>
#include "../Modulation/ModulationMatrix.h"
#include "../Modulation/LFO.h"

namespace braidy {

/**
 * ModulationSettingsOverlay - Settings overlay for LFO modulation configuration
 * Provides UI for configuring 2 LFOs and their routing to various parameters
 */
class ModulationSettingsOverlay : public juce::Component,
                                  public juce::Button::Listener,
                                  public juce::ComboBox::Listener,
                                  public juce::Slider::Listener,
                                  public juce::Timer
{
public:
    ModulationSettingsOverlay(ModulationMatrix& modMatrix, juce::AudioProcessorValueTreeState& apvts, 
                               juce::FileLogger* fileLogger = nullptr)
        : modulationMatrix_(modMatrix), apvts_(apvts), fileLogger_(fileLogger), isVisible_(false)
    {
        // Close button (larger and more prominent)
        closeButton_.setButtonText("X");  // Simple X that works everywhere
        closeButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF555555));
        closeButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF777777));
        closeButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFFFF));
        closeButton_.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFF6666));
        closeButton_.addListener(this);
        addAndMakeVisible(closeButton_);
        
        // Make this component want keyboard focus for ESC key
        setWantsKeyboardFocus(true);
        
        // Title
        titleLabel_.setText("Modulation Settings", juce::dontSendNotification);
        titleLabel_.setFont(juce::Font(20.0f, juce::Font::bold));
        titleLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel_);
        
        // Create LFO sections
        for (int i = 0; i < 2; ++i)
        {
            auto& lfo = lfoSections_[i];
            
            // LFO Enable
            lfo.enableButton.setButtonText("LFO " + juce::String(i + 1) + " Enable");
            lfo.enableButton.setToggleState(false, juce::dontSendNotification);
            lfo.enableButton.addListener(this);
            addAndMakeVisible(lfo.enableButton);
            
            // LFO Shape
            lfo.shapeLabel.setText("Shape:", juce::dontSendNotification);
            addAndMakeVisible(lfo.shapeLabel);
            
            lfo.shapeCombo.addItem("Sine", 1);
            lfo.shapeCombo.addItem("Triangle", 2);
            lfo.shapeCombo.addItem("Square", 3);
            lfo.shapeCombo.addItem("Saw", 4);
            lfo.shapeCombo.addItem("Random", 5);
            lfo.shapeCombo.addItem("Sample & Hold", 6);
            lfo.shapeCombo.setSelectedId(1);
            lfo.shapeCombo.addListener(this);
            addAndMakeVisible(lfo.shapeCombo);
            
            // LFO Rate
            lfo.rateLabel.setText("Rate:", juce::dontSendNotification);
            addAndMakeVisible(lfo.rateLabel);
            
            lfo.rateSlider.setRange(0.01, 20.0, 0.01);
            lfo.rateSlider.setValue(1.0);
            lfo.rateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            lfo.rateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
            lfo.rateSlider.addListener(this);
            addAndMakeVisible(lfo.rateSlider);
            
            // LFO Depth
            lfo.depthLabel.setText("Depth:", juce::dontSendNotification);
            addAndMakeVisible(lfo.depthLabel);
            
            lfo.depthSlider.setRange(0.0, 1.0, 0.01);
            lfo.depthSlider.setValue(0.5);
            lfo.depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            lfo.depthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
            lfo.depthSlider.addListener(this);
            addAndMakeVisible(lfo.depthSlider);
            
            // Tempo Sync
            lfo.tempoSyncButton.setButtonText("Tempo Sync");
            lfo.tempoSyncButton.setToggleState(false, juce::dontSendNotification);
            lfo.tempoSyncButton.addListener(this);
            addAndMakeVisible(lfo.tempoSyncButton);
            
            // Destination routing
            lfo.destLabel.setText("Destination:", juce::dontSendNotification);
            addAndMakeVisible(lfo.destLabel);
            
            // Match APVTS destination list exactly (22 total: None + 21 ModulationMatrix destinations - META and PITCH removed)
            lfo.destCombo.addItem("None", 1);
            lfo.destCombo.addItem("Timbre", 2);
            lfo.destCombo.addItem("Color", 3);
            lfo.destCombo.addItem("FM", 4);
            lfo.destCombo.addItem("Modulation", 5);
            lfo.destCombo.addItem("Fine (Detune)", 6);
            lfo.destCombo.addItem("Coarse (Octave)", 7);
            lfo.destCombo.addItem("Env Attack", 8);
            lfo.destCombo.addItem("Env Decay", 9);
            lfo.destCombo.addItem("Env Sustain", 10);
            lfo.destCombo.addItem("Env Release", 11);
            lfo.destCombo.addItem("Env FM", 12);
            lfo.destCombo.addItem("Env Timbre", 13);
            lfo.destCombo.addItem("Env Color", 14);
            lfo.destCombo.addItem("Bit Depth", 15);
            lfo.destCombo.addItem("Sample Rate", 16);
            lfo.destCombo.addItem("Volume", 17);
            lfo.destCombo.addItem("Pan", 18);
            lfo.destCombo.addItem("Quantize Scale", 19);
            lfo.destCombo.addItem("Quantize Root", 20);
            lfo.destCombo.addItem("Vibrato Amount", 21);
            lfo.destCombo.addItem("Vibrato Rate", 22);
            lfo.destCombo.setSelectedId(1);
            lfo.destCombo.setJustificationType(juce::Justification::centredLeft);
            lfo.destCombo.addListener(this);
            addAndMakeVisible(lfo.destCombo);
            
            // Modulation Amount
            lfo.amountLabel.setText("Amount:", juce::dontSendNotification);
            addAndMakeVisible(lfo.amountLabel);
            
            lfo.amountSlider.setRange(-1.0, 1.0, 0.01);
            lfo.amountSlider.setValue(0.0);
            lfo.amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            lfo.amountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
            lfo.amountSlider.addListener(this);
            addAndMakeVisible(lfo.amountSlider);
            
            // Bipolar/Unipolar
            lfo.bipolarButton.setButtonText("Bipolar");
            lfo.bipolarButton.setToggleState(true, juce::dontSendNotification);
            lfo.bipolarButton.addListener(this);
            addAndMakeVisible(lfo.bipolarButton);
        }
        
        // Global settings
        metaModeButton_.setButtonText("META Mode Enable");
        metaModeButton_.setToggleState(false, juce::dontSendNotification);
        metaModeButton_.addListener(this);
        addAndMakeVisible(metaModeButton_);
        
        quantizerEnableButton_.setButtonText("Quantizer Enable");
        quantizerEnableButton_.setToggleState(false, juce::dontSendNotification);
        quantizerEnableButton_.addListener(this);
        addAndMakeVisible(quantizerEnableButton_);
        
        bitCrusherEnableButton_.setButtonText("Bit Crusher Enable");
        bitCrusherEnableButton_.setToggleState(false, juce::dontSendNotification);
        bitCrusherEnableButton_.addListener(this);
        addAndMakeVisible(bitCrusherEnableButton_);

        // Panic button for stuck notes - subtle outline style
        panicButton_.setButtonText("Kill Stuck Notes");
        panicButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        panicButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF444444));  // Subtle dark fill when pressed
        panicButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFCCCCCC));  // Light grey text
        panicButton_.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));   // White text when pressed
        panicButton_.addListener(this);
        addAndMakeVisible(panicButton_);

        // Set component properties
        setOpaque(true);
        setAlpha(0.98f);  // Higher opacity to reduce transparency
    }
    
    void paint(juce::Graphics& g) override
    {
        // Dark background with higher opacity (less transparent)
        g.fillAll(juce::Colour(20, 20, 20).withAlpha(0.98f));
        
        // Border
        g.setColour(juce::Colour(100, 100, 100));
        g.drawRect(getLocalBounds(), 2);
        
        // Section dividers
        g.setColour(juce::Colour(60, 60, 60));
        g.drawLine(0, 50, getWidth(), 50, 1);
        g.drawLine(getWidth() / 2, 50, getWidth() / 2, getHeight() - 100, 1);
        g.drawLine(0, getHeight() - 100, getWidth(), getHeight() - 100, 1);
    }
    
    // Make clicking outside controls close the overlay
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Check if click is on background (not on any child component)
        if (getComponentAt(e.getPosition()) == this)
        {
            // Optional: close overlay when clicking on background
            // hideOverlay();
            // if (onClose) onClose();
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Close button (reasonable size, fully clickable)
        closeButton_.setBounds(bounds.getWidth() - 50, 10, 40, 35);
        closeButton_.toFront(true);  // Ensure button is on top
        
        // Title
        titleLabel_.setBounds(0, 10, bounds.getWidth(), 30);
        
        // LFO sections
        int lfoSectionWidth = bounds.getWidth() / 2 - 20;
        int yPos = 60;
        
        for (int i = 0; i < 2; ++i)
        {
            auto& lfo = lfoSections_[i];
            int xOffset = i * (bounds.getWidth() / 2) + 10;
            
            lfo.enableButton.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            yPos += 35;
            
            // Shape: Increase width for "Sample & Hold"
            lfo.shapeLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.shapeCombo.setBounds(xOffset + 55, yPos, lfoSectionWidth - 60, 25);
            yPos += 30;
            
            lfo.rateLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.rateSlider.setBounds(xOffset + 55, yPos, lfoSectionWidth - 60, 25);
            yPos += 30;
            
            lfo.depthLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.depthSlider.setBounds(xOffset + 55, yPos, lfoSectionWidth - 75, 25);
            yPos += 30;
            
            lfo.tempoSyncButton.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            yPos += 35;
            
            // Destination: Move label to its own line for more space
            lfo.destLabel.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            yPos += 25;
            lfo.destCombo.setBounds(xOffset, yPos, lfoSectionWidth - 20, 30);
            yPos += 35;
            
            lfo.amountLabel.setBounds(xOffset, yPos, 60, 25);
            lfo.amountSlider.setBounds(xOffset + 65, yPos, lfoSectionWidth - 75, 30);
            yPos += 30;
            
            lfo.bipolarButton.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            yPos += 30;  // Add spacing after bipolar button
            
            // Reset Y position for second LFO (account for taller sections)
            if (i == 0) yPos = 60;
        }
        
        // Global settings at bottom - adjust Y position to account for taller LFO sections
        int globalY = bounds.getHeight() - 90;
        int buttonWidth = (bounds.getWidth() - 50) / 3;

        metaModeButton_.setBounds(10, globalY, buttonWidth, 30);
        quantizerEnableButton_.setBounds(20 + buttonWidth, globalY, buttonWidth, 30);
        bitCrusherEnableButton_.setBounds(30 + buttonWidth * 2, globalY, buttonWidth, 30);

        // Panic button below the other buttons
        int panicY = bounds.getHeight() - 45;
        panicButton_.setBounds(bounds.getWidth() / 2 - 80, panicY, 160, 35);
    }
    
    void buttonClicked(juce::Button* button) override
    {
        if (button == &closeButton_)
        {
            hideOverlay();
            if (onClose)
                onClose();
        }
        else
        {
            // Update LFO settings
            for (int i = 0; i < 2; ++i)
            {
                auto& lfo = lfoSections_[i];
                auto& lfoObj = modulationMatrix_.getLFO(i);
                
                if (button == &lfo.enableButton)
                {
                    bool newState = lfo.enableButton.getToggleState();
                    logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Enable button clicked - New state: " + (newState ? "TRUE" : "FALSE"));
                    
                    if (onLfoEnableChanged)
                        onLfoEnableChanged(i, newState);
                }
                else if (button == &lfo.tempoSyncButton)
                {
                    bool newState = lfo.tempoSyncButton.getToggleState();
                    logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " TempoSync button clicked - New state: " + (newState ? "TRUE" : "FALSE"));
                    
                    if (onLfoTempoSyncChanged)
                        onLfoTempoSyncChanged(i, newState);
                }
                else if (button == &lfo.bipolarButton)
                {
                    logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Bipolar button clicked - New state: " + (lfo.bipolarButton.getToggleState() ? "TRUE" : "FALSE"));
                    updateRouting(i);
                }
            }
            
            // Global settings callbacks
            if (button == &metaModeButton_ && onMetaModeChanged)
                onMetaModeChanged(metaModeButton_.getToggleState());

            if (button == &quantizerEnableButton_) {
                // Update APVTS parameter instead of using callback
                if (auto* param = apvts_.getParameter("quantizerEnable")) {
                    param->setValueNotifyingHost(quantizerEnableButton_.getToggleState() ? 1.0f : 0.0f);
                }
            }

            if (button == &bitCrusherEnableButton_ && onBitCrusherChanged)
                onBitCrusherChanged(bitCrusherEnableButton_.getToggleState());

            // Panic button to kill stuck notes
            if (button == &panicButton_ && onPanicButton) {
                logMessage("[PANIC] Kill stuck notes button pressed");
                onPanicButton();
            }
        }
    }
    
    void comboBoxChanged(juce::ComboBox* combo) override
    {
        for (int i = 0; i < 2; ++i)
        {
            auto& lfo = lfoSections_[i];
            
            if (combo == &lfo.shapeCombo)
            {
                int selectedId = combo->getSelectedId();
                int index = selectedId - 1;  // Convert 1-based ComboBox ID to 0-based index
                
                logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Shape combo changed - Selected ID: " + juce::String(selectedId) + ", Index: " + juce::String(index) + ", Text: '" + combo->getText() + "'");
                
                if (onLfoShapeChanged)
                    onLfoShapeChanged(i, index);
            }
            else if (combo == &lfo.destCombo)
            {
                int selectedId = combo->getSelectedId();
                
                // FIXED MAPPING: ComboBox IDs to APVTS array indices
                // ComboBox dropdown: {"None"=ID1, "Timbre"=ID2, "Color"=ID3, "FM"=ID4, "Modulation"=ID5, ...}
                // APVTS array:       {"None"=0,   "META"=1,    "Timbre"=2,  "Color"=3,  "FM"=4,      "Modulation"=5, ...}
                // The dropdown SKIPS "META" (APVTS index 1) and "Coarse (Pitch)" (APVTS index 6)
                int apvtsIndex = -1;
                
                switch (selectedId) {
                    case 1:  // "None" -> APVTS index 0
                        apvtsIndex = 0;
                        break;
                    case 2:  // "Timbre" -> APVTS index 2 (skip META at index 1)
                        apvtsIndex = 2;
                        break;
                    case 3:  // "Color" -> APVTS index 3
                        apvtsIndex = 3;
                        break;
                    case 4:  // "FM" -> APVTS index 4
                        apvtsIndex = 4;
                        break;
                    case 5:  // "Modulation" -> APVTS index 5
                        apvtsIndex = 5;
                        break;
                    case 6:  // "Fine (Detune)" -> APVTS index 7 (skip "Coarse (Pitch)" at index 6)
                        apvtsIndex = 7;
                        break;
                    case 7:  // "Coarse (Octave)" -> APVTS index 8
                        apvtsIndex = 8;
                        break;
                    default:  // For indices 8+ (Env Attack, etc.), add 1 to account for skipped "Coarse (Pitch)"
                        if (selectedId >= 8) {
                            apvtsIndex = selectedId + 1;  // 8->9, 9->10, 10->11, etc.
                        }
                        break;
                }
                
                logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Destination combo changed - Selected ID: " + juce::String(selectedId) + ", APVTS Index: " + juce::String(apvtsIndex) + ", Text: '" + combo->getText() + "'");
                
                // Set a reasonable default amount when selecting a destination (not "None")
                double currentAmount = lfo.amountSlider.getValue();
                if (selectedId > 1 && std::abs(currentAmount) < 0.01)
                {
                    lfo.amountSlider.setValue(0.5, juce::sendNotification);  // Default to 50% modulation
                    logMessage("[UI DEBUG] Set default amount of 0.5 for LFO" + juce::String(i + 1));
                }
                
                if (onLfoDestChanged && apvtsIndex >= 0)
                    onLfoDestChanged(i, apvtsIndex);
            }
        }
    }
    
    void sliderValueChanged(juce::Slider* slider) override
    {
        for (int i = 0; i < 2; ++i)
        {
            auto& lfo = lfoSections_[i];
            auto& lfoObj = modulationMatrix_.getLFO(i);
            
            if (slider == &lfo.rateSlider)
            {
                juce::String paramId = "lfo" + juce::String(i + 1) + "Rate";
                double newValue = slider->getValue();
                
                logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Rate slider changed - New value: " + juce::String(newValue));
                
                if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts_.getParameter(paramId))) {
                    float normalizedValue = param->convertTo0to1(static_cast<float>(newValue));
                    param->setValueNotifyingHost(normalizedValue);
                    logMessage("[UI DEBUG] APVTS parameter '" + paramId + "' set to: " + juce::String(newValue));
                    
                    // Verify the write was successful
                    if (auto* rawParam = apvts_.getRawParameterValue(paramId)) {
                        float currentValue = rawParam->load();
                        logMessage("[UI DEBUG] APVTS parameter '" + paramId + "' current value after write: " + juce::String(currentValue));
                    }
                } else {
                    logMessage("[UI DEBUG] ERROR: Could not find APVTS parameter '" + paramId + "'");
                }
            }
            else if (slider == &lfo.depthSlider)
            {
                juce::String paramId = "lfo" + juce::String(i + 1) + "Depth";
                double newValue = slider->getValue();
                
                logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Depth slider changed - New value: " + juce::String(newValue));
                
                if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts_.getParameter(paramId))) {
                    float normalizedValue = param->convertTo0to1(static_cast<float>(newValue));
                    param->setValueNotifyingHost(normalizedValue);
                    logMessage("[UI DEBUG] APVTS parameter '" + paramId + "' set to: " + juce::String(newValue));
                    
                    // Verify the write was successful
                    if (auto* rawParam = apvts_.getRawParameterValue(paramId)) {
                        float currentValue = rawParam->load();
                        logMessage("[UI DEBUG] APVTS parameter '" + paramId + "' current value after write: " + juce::String(currentValue));
                    }
                } else {
                    logMessage("[UI DEBUG] ERROR: Could not find APVTS parameter '" + paramId + "'");
                }
            }
            else if (slider == &lfo.amountSlider)
            {
                logMessage("[UI DEBUG] LFO" + juce::String(i + 1) + " Amount slider changed - New value: " + juce::String(slider->getValue()));
                updateRouting(i);
            }
        }
    }
    
    void showOverlay()
    {
        logMessage("[UI DEBUG] ===== SHOWING MODULATION OVERLAY =====");
        
        setVisible(true);
        isVisible_ = true;
        grabKeyboardFocus();  // Grab focus to receive ESC key
        
        // Load current APVTS parameter values into UI
        logMessage("[UI DEBUG] Loading current parameters from APVTS...");
        loadCurrentParameters();
        
        // Don't start timer - it was overwriting APVTS values with ModulationMatrix values
        // The UI should only be driven by APVTS parameters
        // startTimer(33);  // DISABLED - was causing persistence issues
        
        logMessage("[UI DEBUG] Modulation overlay shown (timer disabled to fix persistence)");
    }
    
    void refreshUIFromMatrix()
    {
        // Update each LFO section to reflect current modulation matrix state
        for (int lfoIndex = 0; lfoIndex < 2; ++lfoIndex)
        {
            auto& lfo = lfoSections_[lfoIndex];
            auto& lfoObj = modulationMatrix_.getLFO(lfoIndex);
            
            // Update LFO controls
            lfo.enableButton.setToggleState(lfoObj.isEnabled(), juce::dontSendNotification);
            lfo.shapeCombo.setSelectedId(static_cast<int>(lfoObj.getShape()) + 1, juce::dontSendNotification);
            lfo.rateSlider.setValue(lfoObj.getRate(), juce::dontSendNotification);
            lfo.depthSlider.setValue(lfoObj.getDepth(), juce::dontSendNotification);
            lfo.tempoSyncButton.setToggleState(lfoObj.isTempoSynced(), juce::dontSendNotification);
            
            // Find active routing for this LFO
            bool foundRouting = false;
            for (int d = 0; d < ModulationMatrix::NUM_DESTINATIONS; ++d)
            {
                auto dest = static_cast<ModulationMatrix::Destination>(d);
                const auto& routing = modulationMatrix_.getRouting(dest);
                
                if (routing.enabled && routing.sourceId == lfoIndex)
                {
                    // FIXED REVERSE MAPPING: ModulationMatrix enum values to ComboBox IDs
                    // Convert enum value to APVTS index: enum value d -> APVTS index (d + 1)
                    // Then map APVTS index to ComboBox ID based on which items are shown in dropdown
                    int comboId = 1; // Default to "None"
                    int apvtsIndex = d + 1; // enum value 0 -> APVTS index 1, enum value 1 -> APVTS index 2, etc.
                    
                    // Map APVTS index to ComboBox ID (reverse of the forward mapping)
                    switch (apvtsIndex) {
                        case 0:   // APVTS "None" -> ComboBox "None" (ID=1)
                            comboId = 1;
                            break;
                        case 2:   // APVTS "Timbre" -> ComboBox "Timbre" (ID=2)
                            comboId = 2;
                            break;
                        case 3:   // APVTS "Color" -> ComboBox "Color" (ID=3)
                            comboId = 3;
                            break;
                        case 4:   // APVTS "FM" -> ComboBox "FM" (ID=4) 
                            comboId = 4;
                            break;
                        case 5:   // APVTS "Modulation" -> ComboBox "Modulation" (ID=5)
                            comboId = 5;
                            break;
                        case 7:   // APVTS "Fine (Detune)" -> ComboBox "Fine (Detune)" (ID=6)
                            comboId = 6;
                            break;
                        case 8:   // APVTS "Coarse (Octave)" -> ComboBox "Coarse (Octave)" (ID=7)
                            comboId = 7;
                            break;
                        default:
                            // For APVTS indices 9+ (Env Attack, etc.), subtract 1 to get ComboBox ID
                            // APVTS index 9 ("Env Attack") -> ComboBox ID 8, APVTS index 10 -> ComboBox ID 9, etc.
                            if (apvtsIndex >= 9) {
                                comboId = apvtsIndex - 1;  // 9->8, 10->9, 11->10, etc.
                            }
                            // Skip APVTS indices 1 ("META") and 6 ("Coarse (Pitch)") - not in dropdown
                            break;
                    }
                    
                    lfo.destCombo.setSelectedId(comboId, juce::dontSendNotification);
                    lfo.amountSlider.setValue(routing.amount, juce::dontSendNotification);
                    lfo.bipolarButton.setToggleState(routing.bipolar, juce::dontSendNotification);
                    foundRouting = true;
                    
                    logMessage("[MODULATION] Refreshed UI - LFO" + juce::String(lfoIndex + 1) + " -> " + juce::String(ModulationMatrix::getDestinationName(dest)) + " (enum " + juce::String(d) + " -> ComboBox ID " + juce::String(comboId) + "), amount: " + juce::String(routing.amount));
                    break;
                }
            }
            
            if (!foundRouting)
            {
                // No routing found, set to "None"
                lfo.destCombo.setSelectedId(1, juce::dontSendNotification);
                lfo.amountSlider.setValue(0.0, juce::dontSendNotification);
                logMessage("[MODULATION] Refreshed UI - LFO" + juce::String(lfoIndex + 1) + " -> None");
            }
        }
    }
    
    void loadCurrentParameters()
    {
        logMessage("[UI DEBUG] ===== LOADING CURRENT PARAMETERS =====");
        
        // Load current APVTS parameter values into UI controls
        for (int lfoIndex = 0; lfoIndex < 2; ++lfoIndex)
        {
            auto& lfo = lfoSections_[lfoIndex];
            juce::String lfoPrefix = "lfo" + juce::String(lfoIndex + 1);
            
            logMessage("[UI DEBUG] Loading parameters for LFO" + juce::String(lfoIndex + 1) + "...");
            
            // Load LFO Enable
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "Enable"))
            {
                float value = param->load();
                bool enabled = value > 0.5f;
                lfo.enableButton.setToggleState(enabled, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "Enable: " + juce::String(value) + " -> " + (enabled ? "TRUE" : "FALSE"));
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "Enable");
            }
            
            // Load LFO Shape (AudioParameterChoice - use direct index)
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "Shape"))
            {
                float rawValue = param->load();
                int shapeIndex = static_cast<int>(rawValue);  // Direct index, no denormalization needed
                int comboId = shapeIndex + 1;  // Convert 0-based to 1-based ComboBox ID
                lfo.shapeCombo.setSelectedId(comboId, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "Shape: raw=" + juce::String(rawValue) + ", index=" + juce::String(shapeIndex) + ", comboId=" + juce::String(comboId) + ", text='" + lfo.shapeCombo.getText() + "'");
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "Shape");
            }
            
            // Load LFO Rate
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "Rate"))
            {
                float value = param->load();
                lfo.rateSlider.setValue(value, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "Rate: " + juce::String(value));
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "Rate");
            }
            
            // Load LFO Depth
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "Depth"))
            {
                float value = param->load();
                lfo.depthSlider.setValue(value, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "Depth: " + juce::String(value));
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "Depth");
            }
            
            // Load LFO Tempo Sync
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "TempoSync"))
            {
                float value = param->load();
                bool synced = value > 0.5f;
                lfo.tempoSyncButton.setToggleState(synced, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "TempoSync: " + juce::String(value) + " -> " + (synced ? "TRUE" : "FALSE"));
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "TempoSync");
            }
            
            // Load LFO Destination (AudioParameterChoice - use direct index)
            if (auto* param = apvts_.getRawParameterValue(lfoPrefix + "Dest"))
            {
                float rawValue = param->load();
                int apvtsIndex = static_cast<int>(rawValue);  // Direct index, no denormalization needed
                
                // REVERSE MAPPING: APVTS index to ComboBox ID
                // Must match the reverse mapping used in refreshUIFromMatrix() and be inverse of comboBoxChanged()
                // APVTS includes "META" at index 1 and "Coarse (Pitch)" at index 6 which are NOT in dropdown
                int comboId = 1; // Default to "None"
                
                switch (apvtsIndex) {
                    case 0:   // APVTS "None" -> ComboBox "None" (ID=1)
                        comboId = 1;
                        break;
                    case 2:   // APVTS "Timbre" -> ComboBox "Timbre" (ID=2)
                        comboId = 2;
                        break;
                    case 3:   // APVTS "Color" -> ComboBox "Color" (ID=3)
                        comboId = 3;
                        break;
                    case 4:   // APVTS "FM" -> ComboBox "FM" (ID=4)
                        comboId = 4;
                        break;
                    case 5:   // APVTS "Modulation" -> ComboBox "Modulation" (ID=5)
                        comboId = 5;
                        break;
                    case 7:   // APVTS "Fine (Detune)" -> ComboBox "Fine (Detune)" (ID=6)
                        comboId = 6;
                        break;
                    case 8:   // APVTS "Coarse (Octave)" -> ComboBox "Coarse (Octave)" (ID=7)
                        comboId = 7;
                        break;
                    default:
                        // For APVTS indices 9+ (Env Attack, etc.), subtract 1 to get ComboBox ID
                        // APVTS index 9 ("Env Attack") -> ComboBox ID 8, etc.
                        if (apvtsIndex >= 9) {
                            comboId = apvtsIndex - 1;  // 9->8, 10->9, 11->10, etc.
                        }
                        // Skip APVTS indices 1 ("META") and 6 ("Coarse (Pitch)") - not in dropdown
                        break;
                }
                
                lfo.destCombo.setSelectedId(comboId, juce::dontSendNotification);
                logMessage("[UI DEBUG] " + lfoPrefix + "Dest: raw=" + juce::String(rawValue) + ", APVTS index=" + juce::String(apvtsIndex) + ", comboId=" + juce::String(comboId) + ", text='" + lfo.destCombo.getText() + "'");
            } else {
                logMessage("[UI DEBUG] ERROR: Could not find parameter " + lfoPrefix + "Dest");
            }
            
            // Amount slider is not an APVTS parameter - it's handled by updateRouting()
            // For now, keep it at current value
            logMessage("[UI DEBUG] " + lfoPrefix + "Amount (not from APVTS): " + juce::String(lfo.amountSlider.getValue()));
        }

        // Sync quantizer button state with parameter
        syncQuantizerState();

        logMessage("[UI DEBUG] ===== FINISHED LOADING PARAMETERS =====");
    }
    
    void hideOverlay()
    {
        logMessage("[UI DEBUG] ===== HIDING MODULATION OVERLAY =====");
        
        // stopTimer();  // Timer disabled to fix persistence issues
        setVisible(false);
        isVisible_ = false;
        
        logMessage("[UI DEBUG] Modulation overlay hidden");
    }
    
    bool isOverlayVisible() const { return isVisible_; }
    
    // Sync meta mode state (called from main UI)
    void setMetaModeState(bool enabled) {
        metaModeButton_.setToggleState(enabled, juce::dontSendNotification);
    }

    void syncQuantizerState() {
        if (auto* param = apvts_.getRawParameterValue("quantizerEnable")) {
            bool enabled = param->load() > 0.5f;
            quantizerEnableButton_.setToggleState(enabled, juce::dontSendNotification);
        }
    }
    
    // Timer callback for real-time UI updates
    void timerCallback() override
    {
        // Don't refresh UI from matrix - this overwrites APVTS values!
        // The UI should only update from APVTS parameters
        // Comment out for now to fix persistence issue
        /*
        if (isVisible_)
        {
            refreshUIFromMatrix();
        }
        */
    }
    
    // Handle ESC key to close overlay
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::escapeKey)
        {
            hideOverlay();
            if (onClose)
                onClose();
            return true;  // Key handled
        }
        return false;  // Key not handled
    }
    
    // Callbacks
    std::function<void()> onClose;
    std::function<void(bool)> onMetaModeChanged;
    std::function<void(bool)> onQuantizerChanged;
    std::function<void(bool)> onBitCrusherChanged;
    std::function<void()> onPanicButton;  // Kill stuck notes
    
    // LFO Callbacks
    std::function<void(int, bool)> onLfoEnableChanged;    // (lfoIndex, enabled)
    std::function<void(int, int)> onLfoShapeChanged;      // (lfoIndex, shapeIndex)  
    std::function<void(int, bool)> onLfoTempoSyncChanged; // (lfoIndex, synced)
    std::function<void(int, int)> onLfoDestChanged;       // (lfoIndex, destIndex)
    
private:
    void updateRouting(int lfoIndex)
    {
        auto& lfo = lfoSections_[lfoIndex];
        int selectedId = lfo.destCombo.getSelectedId();
        
        // CORRECT MAPPING: ComboBox IDs to APVTS indices
        // APVTS destination array: {"None", "META", "Timbre", "Color", "FM", "Modulation", "Coarse (Pitch)", "Fine (Detune)", "Coarse (Octave)", "Env Attack", ...}
        // APVTS indices (0-based):  0=None, 1=META, 2=Timbre, 3=Color, 4=FM, 5=Modulation, 6=Coarse(Pitch), 7=Fine(Detune), 8=Coarse(Octave), 9=Env Attack, ...
        // 
        // ComboBox dropdown items: {"None", "Timbre", "Color", "FM", "Modulation", "Fine (Detune)", "Coarse (Octave)", "Env Attack", ...}
        // ComboBox IDs (1-based):   1=None, 2=Timbre, 3=Color, 4=FM, 5=Modulation, 6=Fine(Detune), 7=Coarse(Octave), 8=Env Attack, ...
        //
        // The dropdown REMOVED "META" (APVTS index 1) and "Coarse (Pitch)" (APVTS index 6) from display
        
        logMessage("[MODULATION] LFO" + juce::String(lfoIndex + 1) + " routing update - Selected ID: " + juce::String(selectedId) + ", Text: '" + lfo.destCombo.getText() + "'");
        
        if (selectedId > 1)  // Not "None"
        {
            int apvtsIndex = -1;
            
            // FIXED MAPPING: ComboBox selections to APVTS array indices
            switch (selectedId) {
                case 2:  // "Timbre" -> APVTS index 2 ("Timbre")
                    apvtsIndex = 2;
                    break;
                case 3:  // "Color" -> APVTS index 3 ("Color")  
                    apvtsIndex = 3;
                    break;
                case 4:  // "FM" -> APVTS index 4 ("FM")
                    apvtsIndex = 4;
                    break;
                case 5:  // "Modulation" -> APVTS index 5 ("Modulation")
                    apvtsIndex = 5;
                    break;
                case 6:  // "Fine (Detune)" -> APVTS index 7 ("Fine (Detune)") - skip APVTS index 6 "Coarse (Pitch)"
                    apvtsIndex = 7;
                    break;
                case 7:  // "Coarse (Octave)" -> APVTS index 8 ("Coarse (Octave)")
                    apvtsIndex = 8;
                    break;
                default:
                    // For remaining items (Env Attack onward), add offset of +1 to account for removed "META" and "Coarse (Pitch)"
                    // ComboBox ID 8 ("Env Attack") -> APVTS index 9, ComboBox ID 9 -> APVTS index 10, etc.
                    if (selectedId >= 8) {
                        apvtsIndex = selectedId + 1;  // 8->9, 9->10, 10->11, etc.
                    }
                    break;
            }
            
            if (apvtsIndex >= 0)
            {
                // EXPLICIT MAPPING: APVTS index to ModulationMatrix::Destination enum value
                // This mapping must match the actual parameter behavior in the audio processor
                ModulationMatrix::Destination dest;
                bool validDest = true;
                
                switch (apvtsIndex) {
                    case 1:  // "META" -> ALGORITHM_SELECTION (enum 0)
                        dest = ModulationMatrix::ALGORITHM_SELECTION;
                        break;
                    case 2:  // "Timbre" -> TIMBRE (enum 1)
                        dest = ModulationMatrix::TIMBRE;
                        break;
                    case 3:  // "Color" -> COLOR (enum 2)
                        dest = ModulationMatrix::COLOR;
                        break;
                    case 4:  // "FM" -> FM_AMOUNT (enum 3)
                        dest = ModulationMatrix::FM_AMOUNT;
                        break;
                    case 5:  // "Modulation" -> ENV_TIMBRE_AMOUNT (enum 12) - modulates the modulation knob
                        dest = ModulationMatrix::ENV_TIMBRE_AMOUNT;
                        break;
                    case 6:  // "Coarse (Pitch)" -> PITCH (enum 4)
                        dest = ModulationMatrix::PITCH;
                        break;
                    case 7:  // "Fine (Detune)" -> DETUNE (enum 5)
                        dest = ModulationMatrix::DETUNE;
                        break;
                    case 8:  // "Coarse (Octave)" -> OCTAVE (enum 6)
                        dest = ModulationMatrix::OCTAVE;
                        break;
                    case 9:  // "Env Attack" -> ENV_ATTACK (enum 7 in ModulationMatrix)
                        dest = ModulationMatrix::ENV_ATTACK;
                        break;
                    case 10: // "Env Decay" -> ENV_DECAY (enum 8 in ModulationMatrix)
                        dest = ModulationMatrix::ENV_DECAY;
                        break;
                    case 11: // "Env Sustain" -> ENV_SUSTAIN (enum 9 in ModulationMatrix)
                        dest = ModulationMatrix::ENV_SUSTAIN;
                        break;
                    case 12: // "Env Release" -> ENV_RELEASE (enum 10 in ModulationMatrix)
                        dest = ModulationMatrix::ENV_RELEASE;
                        break;
                    default:
                        validDest = false;
                        logMessage("[MODULATION] ERROR: No mapping defined for APVTS index " + juce::String(apvtsIndex));
                        break;
                }
                
                if (validDest)
                {
                    float amount = static_cast<float>(lfo.amountSlider.getValue());
                    bool bipolar = lfo.bipolarButton.getToggleState();
                    
                    logMessage("[MODULATION] Setting routing: LFO" + juce::String(lfoIndex + 1) + " -> " + juce::String(ModulationMatrix::getDestinationName(dest)) + " (ComboBox ID " + juce::String(selectedId) + " -> APVTS index " + juce::String(apvtsIndex) + " -> enum value " + juce::String(static_cast<int>(dest)) + "), amount: " + juce::String(amount));
                    
                    modulationMatrix_.setRouting(lfoIndex, dest, amount, bipolar);
                }
            } else {
                logMessage("[MODULATION] ERROR: Invalid ComboBox ID " + juce::String(selectedId) + " could not be mapped to APVTS index");
            }
        }
        else
        {
            logMessage("[MODULATION] Clearing routings for LFO" + juce::String(lfoIndex + 1));
            
            // Clear all routings for this LFO
            for (int d = 0; d < ModulationMatrix::NUM_DESTINATIONS; ++d)
            {
                auto dest = static_cast<ModulationMatrix::Destination>(d);
                if (modulationMatrix_.getRouting(dest).sourceId == lfoIndex)
                {
                    modulationMatrix_.clearRouting(dest);
                }
            }
        }
    }
    
    ModulationMatrix& modulationMatrix_;
    juce::AudioProcessorValueTreeState& apvts_;
    bool isVisible_;
    
    juce::TextButton closeButton_;
    juce::Label titleLabel_;
    
    struct LFOSection
    {
        juce::ToggleButton enableButton;
        juce::Label shapeLabel;
        juce::ComboBox shapeCombo;
        juce::Label rateLabel;
        juce::Slider rateSlider;
        juce::Label depthLabel;
        juce::Slider depthSlider;
        juce::ToggleButton tempoSyncButton;
        juce::Label destLabel;
        juce::ComboBox destCombo;
        juce::Label amountLabel;
        juce::Slider amountSlider;
        juce::ToggleButton bipolarButton;
    };
    
    LFOSection lfoSections_[2];

    juce::ToggleButton metaModeButton_;
    juce::ToggleButton quantizerEnableButton_;
    juce::ToggleButton bitCrusherEnableButton_;

    // Panic button for stuck notes
    juce::TextButton panicButton_;
    
    juce::FileLogger* fileLogger_;
    
    // Helper function to log messages using the file logger if available, otherwise fallback to logMessage
    void logMessage(const juce::String& message) const
    {
        if (fileLogger_)
        {
            fileLogger_->logMessage(message);
        }
        else
        {
            DBG(message);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSettingsOverlay)
};

} // namespace braidy