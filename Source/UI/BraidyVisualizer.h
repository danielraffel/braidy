#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../BraidyCore/BraidyTypes.h"
#include <array>

namespace braidy {

/**
 * Real-time waveform visualizer for Braidy synthesizer output
 */
class BraidyVisualizer : public juce::Component, public juce::Timer
{
public:
    BraidyVisualizer();
    ~BraidyVisualizer() override = default;
    
    // Audio data processing
    void processAudioBlock(const float* data, int numSamples);
    void setSampleRate(double sampleRate);
    
    // Display settings
    enum class DisplayMode {
        Waveform,       // Time domain waveform
        Spectrum,       // Frequency spectrum
        Oscilloscope    // XY oscilloscope mode
    };
    
    void setDisplayMode(DisplayMode mode);
    void setTimeScale(float timeScale);    // 0.1 to 10.0 seconds
    void setAmplitudeScale(float scale);   // 0.1 to 5.0
    void setRefreshRate(int hz);           // 10 to 60 Hz
    
    // Visual settings
    void setTraceColour(juce::Colour colour) { traceColour_ = colour; repaint(); }
    void setGridColour(juce::Colour colour) { gridColour_ = colour; repaint(); }
    void setBackgroundColour(juce::Colour colour) { backgroundColour_ = colour; repaint(); }
    void setShowGrid(bool show) { showGrid_ = show; repaint(); }
    void setShowLabels(bool show) { showLabels_ = show; repaint(); }
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    // Audio processing
    static constexpr int kBufferSize = 2048;
    static constexpr int kDisplaySamples = 512;
    
    std::array<float, kBufferSize> audioBuffer_;
    std::array<float, kDisplaySamples> displayBuffer_;
    std::array<float, kDisplaySamples> spectrumBuffer_;
    
    int writeIndex_;
    int samplesCollected_;
    double sampleRate_;
    bool needsRepaint_;
    
    // Display settings
    DisplayMode displayMode_;
    float timeScale_;
    float amplitudeScale_;
    bool showGrid_;
    bool showLabels_;
    
    // Visual properties
    juce::Colour traceColour_;
    juce::Colour gridColour_;
    juce::Colour backgroundColour_;
    
    // FFT for spectrum analysis
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::array<float, kBufferSize * 2> fftData_;
    
    // Peak detection
    float currentPeak_;
    float peakDecay_;
    
    // Drawing methods
    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawOscilloscope(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawLabels(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    // Utility methods
    void updateDisplayBuffer();
    void performFFT();
    juce::Path createWaveformPath(juce::Rectangle<float> bounds);
    juce::Path createSpectrumPath(juce::Rectangle<float> bounds);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidyVisualizer)
};

}  // namespace braidy