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
                                  public juce::Slider::Listener
{
public:
    ModulationSettingsOverlay(ModulationMatrix& modMatrix)
        : modulationMatrix_(modMatrix), isVisible_(false)
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
            lfo.rateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
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
            
            lfo.destCombo.addItem("None", 1);
            lfo.destCombo.addItem("Algorithm (META)", 2);
            lfo.destCombo.addItem("Timbre", 3);
            lfo.destCombo.addItem("Color", 4);
            lfo.destCombo.addItem("Pitch", 5);
            lfo.destCombo.addItem("Detune", 6);
            lfo.destCombo.addItem("Env Attack", 7);
            lfo.destCombo.addItem("Env Decay", 8);
            lfo.destCombo.addItem("Env Sustain", 9);
            lfo.destCombo.addItem("Env Release", 10);
            lfo.destCombo.addItem("Env->Timbre", 11);
            lfo.destCombo.addItem("Env->Color", 12);
            lfo.destCombo.addItem("Bit Depth", 13);
            lfo.destCombo.addItem("Sample Rate", 14);
            lfo.destCombo.addItem("Volume", 15);
            lfo.destCombo.addItem("Pan", 16);
            lfo.destCombo.addItem("Quantize Scale", 17);
            lfo.destCombo.addItem("Quantize Root", 18);
            lfo.destCombo.setSelectedId(1);
            lfo.destCombo.addListener(this);
            addAndMakeVisible(lfo.destCombo);
            
            // Modulation Amount
            lfo.amountLabel.setText("Amount:", juce::dontSendNotification);
            addAndMakeVisible(lfo.amountLabel);
            
            lfo.amountSlider.setRange(-1.0, 1.0, 0.01);
            lfo.amountSlider.setValue(0.0);
            lfo.amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            lfo.amountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
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
            
            lfo.shapeLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.shapeCombo.setBounds(xOffset + 55, yPos, lfoSectionWidth - 75, 25);
            yPos += 30;
            
            lfo.rateLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.rateSlider.setBounds(xOffset + 55, yPos, lfoSectionWidth - 75, 25);
            yPos += 30;
            
            lfo.depthLabel.setBounds(xOffset, yPos, 50, 25);
            lfo.depthSlider.setBounds(xOffset + 55, yPos, lfoSectionWidth - 75, 25);
            yPos += 30;
            
            lfo.tempoSyncButton.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            yPos += 35;
            
            lfo.destLabel.setBounds(xOffset, yPos, 70, 25);
            lfo.destCombo.setBounds(xOffset + 75, yPos, lfoSectionWidth - 95, 25);
            yPos += 30;
            
            lfo.amountLabel.setBounds(xOffset, yPos, 60, 25);
            lfo.amountSlider.setBounds(xOffset + 65, yPos, lfoSectionWidth - 85, 25);
            yPos += 30;
            
            lfo.bipolarButton.setBounds(xOffset, yPos, lfoSectionWidth - 20, 25);
            
            // Reset Y position for second LFO
            if (i == 0) yPos = 60;
        }
        
        // Global settings at bottom
        int globalY = bounds.getHeight() - 90;
        int buttonWidth = (bounds.getWidth() - 40) / 3;
        
        metaModeButton_.setBounds(10, globalY, buttonWidth, 30);
        quantizerEnableButton_.setBounds(20 + buttonWidth, globalY, buttonWidth, 30);
        bitCrusherEnableButton_.setBounds(30 + buttonWidth * 2, globalY, buttonWidth, 30);
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
                    lfoObj.setEnabled(lfo.enableButton.getToggleState());
                }
                else if (button == &lfo.tempoSyncButton)
                {
                    lfoObj.setTempoSync(lfo.tempoSyncButton.getToggleState());
                }
                else if (button == &lfo.bipolarButton)
                {
                    updateRouting(i);
                }
            }
            
            // Global settings callbacks
            if (button == &metaModeButton_ && onMetaModeChanged)
                onMetaModeChanged(metaModeButton_.getToggleState());
            
            if (button == &quantizerEnableButton_ && onQuantizerChanged)
                onQuantizerChanged(quantizerEnableButton_.getToggleState());
            
            if (button == &bitCrusherEnableButton_ && onBitCrusherChanged)
                onBitCrusherChanged(bitCrusherEnableButton_.getToggleState());
        }
    }
    
    void comboBoxChanged(juce::ComboBox* combo) override
    {
        for (int i = 0; i < 2; ++i)
        {
            auto& lfo = lfoSections_[i];
            
            if (combo == &lfo.shapeCombo)
            {
                auto& lfoObj = modulationMatrix_.getLFO(i);
                lfoObj.setShape(static_cast<LFO::Shape>(combo->getSelectedId() - 1));
            }
            else if (combo == &lfo.destCombo)
            {
                updateRouting(i);
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
                lfoObj.setRate(static_cast<float>(slider->getValue()));
            }
            else if (slider == &lfo.depthSlider)
            {
                lfoObj.setDepth(static_cast<float>(slider->getValue()));
            }
            else if (slider == &lfo.amountSlider)
            {
                updateRouting(i);
            }
        }
    }
    
    void showOverlay()
    {
        setVisible(true);
        isVisible_ = true;
        grabKeyboardFocus();  // Grab focus to receive ESC key
    }
    
    void hideOverlay()
    {
        setVisible(false);
        isVisible_ = false;
    }
    
    bool isOverlayVisible() const { return isVisible_; }
    
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
    
private:
    void updateRouting(int lfoIndex)
    {
        auto& lfo = lfoSections_[lfoIndex];
        int destId = lfo.destCombo.getSelectedId() - 2;  // -2 because "None" is 1
        
        if (destId >= 0)
        {
            auto dest = static_cast<ModulationMatrix::Destination>(destId);
            float amount = static_cast<float>(lfo.amountSlider.getValue());
            bool bipolar = lfo.bipolarButton.getToggleState();
            
            modulationMatrix_.setRouting(lfoIndex, dest, amount, bipolar);
        }
        else
        {
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
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSettingsOverlay)
};

} // namespace braidy