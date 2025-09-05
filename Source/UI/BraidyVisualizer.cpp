#include "BraidyVisualizer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace braidy {

BraidyVisualizer::BraidyVisualizer()
    : writeIndex_(0)
    , samplesCollected_(0)
    , sampleRate_(48000.0)
    , needsRepaint_(false)
    , displayMode_(DisplayMode::Waveform)
    , timeScale_(0.1f)  // 100ms default
    , amplitudeScale_(1.0f)
    , showGrid_(true)
    , showLabels_(true)
    , traceColour_(juce::Colour(0xff00ff40))      // Bright green
    , gridColour_(juce::Colour(0xff333333))       // Dark gray
    , backgroundColour_(juce::Colour(0xff1a1a1a)) // Very dark
    , currentPeak_(0.0f)
    , peakDecay_(0.95f)
{
    // Initialize buffers
    audioBuffer_.fill(0.0f);
    displayBuffer_.fill(0.0f);
    spectrumBuffer_.fill(0.0f);
    fftData_.fill(0.0f);
    
    // Create FFT for spectrum analysis
    fft_ = std::make_unique<juce::dsp::FFT>(10);  // 2^10 = 1024 points
    
    // Start refresh timer at 30 Hz
    startTimer(1000 / 30);
}

void BraidyVisualizer::processAudioBlock(const float* data, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        audioBuffer_[writeIndex_] = data[i];
        writeIndex_ = (writeIndex_ + 1) % kBufferSize;
        
        // Update peak detector
        float absValue = std::abs(data[i]);
        if (absValue > currentPeak_) {
            currentPeak_ = absValue;
        }
    }
    
    samplesCollected_ += numSamples;
    
    // Update display buffer periodically
    int samplesPerUpdate = static_cast<int>(sampleRate_ * timeScale_ / kDisplaySamples);
    if (samplesCollected_ >= samplesPerUpdate) {
        updateDisplayBuffer();
        samplesCollected_ = 0;
        needsRepaint_ = true;
    }
}

void BraidyVisualizer::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void BraidyVisualizer::setDisplayMode(DisplayMode mode) {
    if (displayMode_ != mode) {
        displayMode_ = mode;
        needsRepaint_ = true;
        repaint();
    }
}

void BraidyVisualizer::setTimeScale(float timeScale) {
    timeScale_ = juce::jlimit(0.01f, 10.0f, timeScale);
    needsRepaint_ = true;
}

void BraidyVisualizer::setAmplitudeScale(float scale) {
    amplitudeScale_ = juce::jlimit(0.1f, 10.0f, scale);
    repaint();
}

void BraidyVisualizer::setRefreshRate(int hz) {
    int interval = 1000 / juce::jlimit(10, 60, hz);
    startTimer(interval);
}

void BraidyVisualizer::updateDisplayBuffer() {
    // Copy from circular audio buffer to linear display buffer
    int readIndex = (writeIndex_ - kDisplaySamples + kBufferSize) % kBufferSize;
    
    for (int i = 0; i < kDisplaySamples; ++i) {
        displayBuffer_[i] = audioBuffer_[(readIndex + i) % kBufferSize];
    }
    
    // Perform FFT if in spectrum mode
    if (displayMode_ == DisplayMode::Spectrum) {
        performFFT();
    }
}

void BraidyVisualizer::performFFT() {
    // Copy data to FFT buffer (interleaved real/imaginary)
    for (int i = 0; i < kDisplaySamples; ++i) {
        fftData_[i * 2] = displayBuffer_[i];      // Real
        fftData_[i * 2 + 1] = 0.0f;               // Imaginary
    }
    
    // Perform FFT
    fft_->performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Convert to magnitude spectrum
    for (int i = 0; i < kDisplaySamples / 2; ++i) {
        float real = fftData_[i * 2];
        float imag = fftData_[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);
        
        // Convert to dB
        spectrumBuffer_[i] = 20.0f * std::log10(magnitude + 1e-6f);
    }
}

void BraidyVisualizer::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(backgroundColour_);
    
    // Border
    g.setColour(gridColour_.brighter());
    g.drawRect(bounds, 1.0f);
    
    auto displayBounds = bounds.reduced(2.0f);
    
    // Grid
    if (showGrid_) {
        drawGrid(g, displayBounds);
    }
    
    // Waveform/spectrum
    switch (displayMode_) {
        case DisplayMode::Waveform:
            drawWaveform(g, displayBounds);
            break;
        case DisplayMode::Spectrum:
            drawSpectrum(g, displayBounds);
            break;
        case DisplayMode::Oscilloscope:
            drawOscilloscope(g, displayBounds);
            break;
    }
    
    // Labels
    if (showLabels_) {
        drawLabels(g, bounds);
    }
    
    // Peak indicator
    if (currentPeak_ > 0.01f) {
        g.setColour(traceColour_.withAlpha(0.7f));
        float peakY = displayBounds.getBottom() - (currentPeak_ / amplitudeScale_) * displayBounds.getHeight() * 0.5f;
        g.drawHorizontalLine(static_cast<int>(peakY), displayBounds.getX(), displayBounds.getRight());
    }
}

void BraidyVisualizer::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(gridColour_);
    
    // Horizontal lines (amplitude)
    int numHLines = 8;
    for (int i = 1; i < numHLines; ++i) {
        float y = bounds.getY() + (bounds.getHeight() * i) / numHLines;
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }
    
    // Vertical lines (time/frequency)
    int numVLines = 10;
    for (int i = 1; i < numVLines; ++i) {
        float x = bounds.getX() + (bounds.getWidth() * i) / numVLines;
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    
    // Center line
    g.setColour(gridColour_.brighter());
    float centerY = bounds.getCentreY();
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());
}

void BraidyVisualizer::drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(traceColour_);
    
    auto path = createWaveformPath(bounds);
    g.strokePath(path, juce::PathStrokeType(1.5f));
    
    // Add subtle glow
    g.setColour(traceColour_.withAlpha(0.3f));
    g.strokePath(path, juce::PathStrokeType(3.0f));
}

void BraidyVisualizer::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(traceColour_);
    
    auto path = createSpectrumPath(bounds);
    g.strokePath(path, juce::PathStrokeType(1.0f));
    
    // Fill under curve
    juce::Path fillPath = path;
    fillPath.lineTo(bounds.getRight(), bounds.getBottom());
    fillPath.lineTo(bounds.getX(), bounds.getBottom());
    fillPath.closeSubPath();
    
    g.setColour(traceColour_.withAlpha(0.2f));
    g.fillPath(fillPath);
}

void BraidyVisualizer::drawOscilloscope(juce::Graphics& g, juce::Rectangle<float> bounds) {
    // XY mode using consecutive samples as X and Y
    g.setColour(traceColour_.withAlpha(0.8f));
    
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f / amplitudeScale_;
    
    juce::Path path;
    bool firstPoint = true;
    
    for (int i = 0; i < kDisplaySamples - 1; i += 2) {
        float x = centerX + displayBuffer_[i] * scale;
        float y = centerY + displayBuffer_[i + 1] * scale;
        
        if (firstPoint) {
            path.startNewSubPath(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void BraidyVisualizer::drawLabels(juce::Graphics& g, juce::Rectangle<float> bounds) {
    g.setColour(gridColour_.brighter());
    g.setFont(juce::Font(10.0f));
    
    // Mode label
    juce::String modeText;
    switch (displayMode_) {
        case DisplayMode::Waveform: modeText = "WAVE"; break;
        case DisplayMode::Spectrum: modeText = "SPEC"; break;
        case DisplayMode::Oscilloscope: modeText = "XY"; break;
    }
    
    g.drawText(modeText, bounds.removeFromTop(12).reduced(4, 0), 
               juce::Justification::topLeft, false);
    
    // Scale info
    if (displayMode_ == DisplayMode::Waveform) {
        juce::String timeText = juce::String(timeScale_ * 1000.0f, 0) + "ms";
        g.drawText(timeText, bounds.removeFromBottom(12).reduced(4, 0), 
                   juce::Justification::bottomRight, false);
    }
    
    // Peak level
    if (currentPeak_ > 0.001f) {
        juce::String peakText = juce::String(20.0f * std::log10(currentPeak_), 1) + "dB";
        g.drawText(peakText, bounds.removeFromBottom(12).reduced(4, 0), 
                   juce::Justification::bottomLeft, false);
    }
}

juce::Path BraidyVisualizer::createWaveformPath(juce::Rectangle<float> bounds) {
    juce::Path path;
    
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerY = bounds.getCentreY();
    
    bool firstPoint = true;
    for (int i = 0; i < kDisplaySamples; ++i) {
        float x = bounds.getX() + (i * width) / kDisplaySamples;
        float y = centerY - (displayBuffer_[i] / amplitudeScale_) * height * 0.4f;
        
        y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);
        
        if (firstPoint) {
            path.startNewSubPath(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    return path;
}

juce::Path BraidyVisualizer::createSpectrumPath(juce::Rectangle<float> bounds) {
    juce::Path path;
    
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float bottomY = bounds.getBottom();
    
    int numBins = kDisplaySamples / 2;
    bool firstPoint = true;
    
    for (int i = 1; i < numBins; ++i) {  // Skip DC bin
        float x = bounds.getX() + (i * width) / numBins;
        
        // Map spectrum magnitude to display height
        float magnitude = spectrumBuffer_[i];
        float normalizedMag = (magnitude + 60.0f) / 60.0f;  // -60dB to 0dB range
        normalizedMag = juce::jlimit(0.0f, 1.0f, normalizedMag);
        
        float y = bottomY - normalizedMag * height;
        
        if (firstPoint) {
            path.startNewSubPath(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    return path;
}

void BraidyVisualizer::resized() {
    // Component size changed - no specific action needed
}

void BraidyVisualizer::timerCallback() {
    // Apply peak decay
    currentPeak_ *= peakDecay_;
    
    // Repaint if needed
    if (needsRepaint_) {
        needsRepaint_ = false;
        repaint();
    }
}

}  // namespace braidy