#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace braidy {

/**
 * LFO - Low Frequency Oscillator for modulation
 * Generates periodic waveforms for parameter modulation.
 * Supports multiple waveform shapes and syncs to host tempo.
 */
class LFO
{
public:
    enum Shape
    {
        SINE,
        TRIANGLE,
        SQUARE,
        SAW,
        RANDOM,
        SAMPLE_AND_HOLD
    };
    
    LFO() = default;
    
    // Enable/disable the LFO
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set rate in Hz or beats per cycle depending on sync mode
    void setRate(float rate)
    {
        rate_ = juce::jlimit(0.01f, 20.0f, rate);
    }
    float getRate() const { return rate_; }
    
    // Set tempo sync mode
    void setTempoSync(bool sync) { tempoSync_ = sync; }
    bool isTempoSynced() const { return tempoSync_; }
    
    // Set waveform shape
    void setShape(Shape shape) { shape_ = shape; }
    Shape getShape() const { return shape_; }
    
    // Set depth/amount (0-1)
    void setDepth(float depth)
    {
        depth_ = juce::jlimit(0.0f, 1.0f, depth);
    }
    float getDepth() const { return depth_; }
    
    // Set phase offset (0-1)
    void setPhaseOffset(float offset)
    {
        phaseOffset_ = juce::jlimit(0.0f, 1.0f, offset);
    }
    float getPhaseOffset() const { return phaseOffset_; }
    
    // Advance the LFO by one audio block
    void advance(double sampleRate, int numSamples, double bpm = 120.0)
    {
        if (!enabled_) return;
        
        // Safety check for buffer size
        if (numSamples <= 0 || numSamples > 8192) {
            return; // Skip processing for invalid buffer sizes
        }
        
        double phaseIncrement;
        if (tempoSync_)
        {
            // Tempo-synced mode: rate_ is in beats
            // Add safety checks to prevent division by zero or extreme values
            if (bpm < 1.0 || bpm > 999.0 || rate_ < 0.01 || sampleRate < 1000.0) {
                // Fallback to free-running mode if values are unsafe
                phaseIncrement = 1.0 / sampleRate; // 1 Hz fallback
            } else {
                double samplesPerBeat = (60.0 * sampleRate) / bpm;
                double denominator = samplesPerBeat * rate_;
                if (denominator < 0.0001) {
                    phaseIncrement = 1.0 / sampleRate; // 1 Hz fallback
                } else {
                    phaseIncrement = 1.0 / denominator;
                }
            }
        }
        else
        {
            // Free-running mode: rate_ is in Hz
            if (sampleRate > 0.0) {
                phaseIncrement = rate_ / sampleRate;
            } else {
                phaseIncrement = 0.0;
            }
        }
        
        // Clamp phase increment to prevent extreme values that could cause crashes
        phaseIncrement = std::clamp(phaseIncrement, 0.0, 0.1); // Max 10% of sample rate
        
        for (int i = 0; i < numSamples; ++i)
        {
            phase_ += phaseIncrement;
            
            // Wrap phase
            while (phase_ >= 1.0) 
            {
                phase_ -= 1.0;
                
                // Generate new random value at cycle boundary
                if (shape_ == RANDOM || shape_ == SAMPLE_AND_HOLD)
                {
                    lastRandom_ = (random_.nextFloat() * 2.0f) - 1.0f;
                }
            }
        }
    }
    
    // Get current LFO value (-1 to +1)
    float getValue() const
    {
        if (!enabled_) return 0.0f;
        
        float effectivePhase = phase_ + phaseOffset_;
        while (effectivePhase >= 1.0f) effectivePhase -= 1.0f;
        
        float rawValue = 0.0f;
        
        switch (shape_)
        {
            case SINE:
                rawValue = std::sin(effectivePhase * 2.0 * M_PI);
                break;
                
            case TRIANGLE:
                if (effectivePhase < 0.5)
                    rawValue = 4.0f * effectivePhase - 1.0f;
                else
                    rawValue = 3.0f - 4.0f * effectivePhase;
                break;
                
            case SQUARE:
                rawValue = effectivePhase < 0.5f ? 1.0f : -1.0f;
                break;
                
            case SAW:
                rawValue = 2.0f * effectivePhase - 1.0f;
                break;
                
            case RANDOM:
                // Smooth random (interpolated)
                {
                    float nextPhase = effectivePhase + 0.01f;
                    if (nextPhase >= 1.0f) nextPhase -= 1.0f;
                    float blend = effectivePhase * 100.0f;
                    blend = blend - std::floor(blend);
                    rawValue = lastRandom_ * (1.0f - blend) + lastRandom_ * blend;
                }
                break;
                
            case SAMPLE_AND_HOLD:
                rawValue = lastRandom_;
                break;
        }
        
        return rawValue * depth_;
    }
    
    // Get unipolar value (0 to 1) for modulating positive-only parameters
    float getUnipolarValue() const
    {
        return (getValue() + 1.0f) * 0.5f;
    }
    
    // Reset phase to beginning
    void reset()
    {
        phase_ = 0.0;
        lastRandom_ = 0.0f;
    }
    
    // Sync phase to a specific position (0-1)
    void syncPhase(double phase)
    {
        phase_ = juce::jlimit(0.0, 1.0, phase);
    }
    
    // Save/restore state
    void saveToValueTree(juce::ValueTree& tree) const
    {
        tree.setProperty("enabled", enabled_, nullptr);
        tree.setProperty("rate", rate_, nullptr);
        tree.setProperty("tempoSync", tempoSync_, nullptr);
        tree.setProperty("shape", static_cast<int>(shape_), nullptr);
        tree.setProperty("depth", depth_, nullptr);
        tree.setProperty("phaseOffset", phaseOffset_, nullptr);
    }
    
    void loadFromValueTree(const juce::ValueTree& tree)
    {
        enabled_ = tree.getProperty("enabled", false);
        rate_ = tree.getProperty("rate", 1.0f);
        tempoSync_ = tree.getProperty("tempoSync", false);
        shape_ = static_cast<Shape>(static_cast<int>(tree.getProperty("shape", 0)));
        depth_ = tree.getProperty("depth", 0.5f);
        phaseOffset_ = tree.getProperty("phaseOffset", 0.0f);
    }
    
private:
    bool enabled_ = false;
    float rate_ = 1.0f;       // Hz or beats depending on sync mode
    bool tempoSync_ = false;  // Sync to host tempo
    Shape shape_ = SINE;
    float depth_ = 0.5f;       // Modulation amount (0-1)
    float phaseOffset_ = 0.0f; // Phase offset (0-1)
    double phase_ = 0.0;       // Current phase (0-1)
    float lastRandom_ = 0.0f;  // Last random value
    juce::Random random_;      // Random number generator
};

} // namespace braidy