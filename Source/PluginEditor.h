#pragma once

#include "PluginProcessor.h"
// #include "UI/BraidyDisplay.h"
// #include "UI/BraidyEncoder.h"
// #include "UI/ModulationSettingsOverlay.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <array>

//==============================================================================
/**
 * Braidy Audio Processor Editor - Authentic Mutable Instruments Braids UI
 */
class BraidyAudioProcessorEditor : public juce::AudioProcessorEditor, 
                                   public juce::Slider::Listener,
                                   public juce::Button::Listener,
                                   public juce::Timer
{
public:
    explicit BraidyAudioProcessorEditor (BraidyAudioProcessor&);
    ~BraidyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    // Component listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
    // Keyboard input for computer keyboard to MIDI
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    
    // Mouse handling to grab keyboard focus
    void mouseDown(const juce::MouseEvent& event) override;
    
    // Timer for display updates
    void timerCallback() override;

private:
    BraidyAudioProcessor& processorRef;
    
    // Main OLED display (4-character) - using simple inline implementation
    std::unique_ptr<juce::Component> oledDisplay_;
    
    // Main EDIT encoder (large, with push button)
    class BraidsEncoder : public juce::Component {
    public:
        BraidsEncoder();
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        
        std::function<void(int)> onValueChange;
        std::function<void()> onClick;
        std::function<void()> onLongPress;
        
    private:
        float angle_ = 0.0f;
        float lastAngle_ = 0.0f;
        bool isPressed_ = false;
        int64_t pressStartTime_ = 0;
        bool longPressTriggered_ = false;
    };
    
    std::unique_ptr<BraidsEncoder> editEncoder_;
    
    // Control knobs (smaller, Braids-style)
    class BraidsKnob : public juce::Component {
    public:
        BraidsKnob(bool isBipolar = false, uint32_t indicatorColor = 0);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        
        void setValue(float value);
        float getValue() const { return value_; }
        void resetToDefault();
        
        std::function<void(float)> onValueChange;
        
    private:
        float value_ = 0.5f;
        float defaultValue_ = 0.5f;
        bool isBipolar_;
        uint32_t indicatorColor_;
        float dragStartY_ = 0;
        float dragStartValue_ = 0;
    };
    
    // Pitch controls
    std::unique_ptr<BraidsKnob> fineKnob_;
    std::unique_ptr<BraidsKnob> coarseKnob_;
    
    // FM control (bipolar attenuverter)
    std::unique_ptr<BraidsKnob> fmKnob_;
    
    // Main parameter controls (larger knobs)
    std::unique_ptr<BraidsKnob> timbreKnob_;
    std::unique_ptr<BraidsKnob> colorKnob_;
    
    // Modulation attenuverter (center knob between TIMBRE and COLOR)
    std::unique_ptr<BraidsKnob> timbreModKnob_;
    
    // CV Jack representations
    class CvJack : public juce::Component {
    public:
        CvJack(const juce::String& label, bool isOutput = false);
        void paint(juce::Graphics& g) override;
        
    private:
        juce::String label_;
        bool isOutput_;
    };
    
    std::array<std::unique_ptr<CvJack>, 6> cvJacks_;
    
    // Menu system state
    enum class MenuPage {
        None,
        WAVE,  // Save settings and exit menu
        META,  // Meta-oscillator settings
        BITS,  // Bit depth
        RATE,  // Sample rate
        BRIG,  // Display brightness
        TSRC,  // Trigger source
        TDLY,  // Trigger delay
        ATK,   // Attack
        DEC,   // Decay
        FM,    // FM envelope amount
        TIM,   // Timbre envelope amount
        COL,   // Color envelope amount
        VCA,   // VCA envelope amount
        RANG,  // Frequency range
        OCTV,  // Octave
        QNTZ,  // Quantizer
        ROOT,  // Root note
        FLAT,  // Detuning
        DRFT,  // Drift
        SIGN,  // Signature
        CV_T,  // CV tester
        MARQ,  // Marquee text
        CALI,  // Calibration
        VERS   // Version
    };
    
    MenuPage currentMenuPage_ = MenuPage::None;
    int menuValue_ = 0;
    bool inEditMode_ = false;
    
    // Display state
    enum class DisplayMode {
        Algorithm,
        Value,
        Menu,
        Startup
    };
    
    DisplayMode displayMode_ = DisplayMode::Algorithm;
    int currentAlgorithm_ = 0;
    
    // Algorithm names (4-character, matching Braids exactly)
    static const std::array<const char*, 47> algorithmNames_;
    
    // Menu page names
    static const std::array<const char*, 24> menuPageNames_;
    
    // Visual style matching Braids panel
    struct BraidsColors {
        static constexpr uint32_t panel = 0xFFE5E5E5;        // Light gray panel
        static constexpr uint32_t text = 0xFF000000;         // Black text
        static constexpr uint32_t encoder = 0xFF2A2A2A;      // Dark gray encoder
        static constexpr uint32_t knob = 0xFF4A4A4A;         // Medium gray knob
        static constexpr uint32_t jack = 0xFF1A1A1A;         // Black jack
        static constexpr uint32_t oled_bg = 0xFF000000;      // Black OLED background
        static constexpr uint32_t oled_text = 0xFF00FF00;    // Green OLED text
        static constexpr uint32_t led_green = 0xFF00FF00;    // Green LED
        static constexpr uint32_t led_red = 0xFFFF0000;      // Red LED
        static constexpr uint32_t led_blue = 0xFF0080FF;     // Blue LED
    };
    
    // Helper methods
    void setupComponents();
    void updateDisplay();
    void updateParameterValues();
    void handleEncoderRotation(int delta);
    void handleEncoderClick();
    void handleEncoderLongPress();
    void enterMenuMode();
    void exitMenuMode();
    void navigateMenu(int delta);
    void editMenuValue(int delta);
    void drawBraidsPanel(juce::Graphics& g);
    void drawScrewHoles(juce::Graphics& g);
    void drawControlLabels(juce::Graphics& g);
    void drawParameterLeds(juce::Graphics& g);
    
    // Parameter mapping
    void updateAlgorithmFromParameter();
    void updateParameterFromKnob(BraidsKnob* knob, const juce::String& paramId);
    void updateAlgorithmParameter();
    void applyMenuValue();
    juce::String getFormattedMenuValue() const;
    void loadModelDefaults(int algorithmIndex);
    
    // File logger for debug output
    std::unique_ptr<juce::FileLogger> fileLogger_;
    
    // Modulation settings overlay - disabled for now
    // std::unique_ptr<braidy::ModulationSettingsOverlay> modulationOverlay_;
    
    // Settings button (modern addition)
    std::unique_ptr<juce::TextButton> settingsButton_;
    
    // Computer keyboard to MIDI functionality
    std::map<int, int> keyToMidiNoteMap_;  // Map keyboard keycode to MIDI note
    std::set<int> pressedKeys_;            // Track currently pressed keys
    void initializeKeyboardMapping();
    void sendMidiNoteOn(int midiNote, float velocity = 0.7f);
    void sendMidiNoteOff(int midiNote);
    void checkForReleasedKeys();
    int getOctaveOffset() const { return 60; }  // C4 as base note
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraidyAudioProcessorEditor)
};