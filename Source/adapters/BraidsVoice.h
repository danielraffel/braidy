/*
 * BraidsVoice.h - JUCE SynthesiserVoice implementation for Braids
 *
 * This class implements JUCE's SynthesiserVoice interface to provide
 * polyphonic voice management for the BraidsEngine. Each voice holds
 * its own BraidsEngine instance for independent operation.
 *
 * Features:
 * - Polyphonic voice management
 * - MIDI note to Braids pitch conversion
 * - Velocity and pitch bend support
 * - Smooth parameter transitions
 * - Per-voice algorithm and parameter control
 */

#pragma once

#include <JuceHeader.h>
#include "BraidsEngine.h"

namespace BraidyAdapter {

/**
 * JUCE SynthesiserVoice implementation for Braids.
 * Each voice manages its own BraidsEngine instance.
 */
class BraidsVoice : public juce::SynthesiserVoice
{
public:
    BraidsVoice();
    ~BraidsVoice() override;

    // SynthesiserVoice interface
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // Voice-specific parameter control
    void setAlgorithm(int algorithm);
    int getAlgorithm() const;
    
    void setParameters(float param1, float param2);
    std::pair<float, float> getParameters() const;
    
    void strike();
    
    // Voice state queries
    bool isVoiceActive() const;
    int getCurrentMidiNote() const;
    float getCurrentVelocity() const;
    
    // Initialize with sample rate
    void setSampleRate(double sampleRate);

    // Get the underlying Braids engine
    BraidsEngine* getBraidsEngine() { return &braidsEngine_; }
    const BraidsEngine* getBraidsEngine() const { return &braidsEngine_; }

private:
    void updatePitch();
    float midiNoteToFrequency(float midiNote) const;
    
    BraidsEngine braidsEngine_;
    
    // Voice state
    bool isActive_;
    int currentMidiNote_;
    float currentVelocity_;
    float pitchBend_;
    double sampleRate_;
    
    // Parameters
    float parameter1_;
    float parameter2_;
    
    // Smoothing
    juce::SmoothedValue<float> smoothedPitch_;
    juce::SmoothedValue<float> smoothedParam1_;
    juce::SmoothedValue<float> smoothedParam2_;
    
    // Envelope for note-off handling
    juce::ADSR adsr_;
    juce::ADSR::Parameters adsrParams_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVoice)
};

/**
 * Simple SynthesiserSound implementation for BraidsVoice.
 * This is required by JUCE's synthesiser architecture.
 */
class BraidsSound : public juce::SynthesiserSound
{
public:
    BraidsSound() = default;
    ~BraidsSound() override = default;

    bool appliesToNote(int midiNoteNumber) override {
        juce::ignoreUnused(midiNoteNumber);
        return true; // Applies to all notes
    }

    bool appliesToChannel(int midiChannel) override {
        juce::ignoreUnused(midiChannel);
        return true; // Applies to all channels
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsSound)
};

} // namespace BraidyAdapter