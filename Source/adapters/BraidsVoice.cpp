/*
 * BraidsVoice.cpp - Implementation of JUCE SynthesiserVoice for Braids
 */

#include "BraidsVoice.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace BraidyAdapter {

BraidsVoice::BraidsVoice()
    : isActive_(false)
    , currentMidiNote_(-1)
    , currentVelocity_(0.0f)
    , pitchBend_(0.0f)
    , sampleRate_(48000.0)
    , parameter1_(0.5f)
    , parameter2_(0.5f)
{
    // Initialize ADSR with reasonable defaults
    adsrParams_.attack = 0.01f;   // 10ms attack
    adsrParams_.decay = 0.1f;     // 100ms decay  
    adsrParams_.sustain = 0.8f;   // 80% sustain level
    adsrParams_.release = 0.3f;   // 300ms release
    adsr_.setParameters(adsrParams_);
    
    // Initialize smoothed values
    smoothedPitch_.setCurrentAndTargetValue(60.0f);
    smoothedParam1_.setCurrentAndTargetValue(0.5f);
    smoothedParam2_.setCurrentAndTargetValue(0.5f);
}

BraidsVoice::~BraidsVoice() = default;

bool BraidsVoice::canPlaySound(juce::SynthesiserSound* sound) {
    return dynamic_cast<BraidsSound*>(sound) != nullptr;
}

void BraidsVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) {
    juce::ignoreUnused(sound);
    
    std::cout << "[DEBUG] BraidsVoice::startNote - note=" << midiNoteNumber 
              << " velocity=" << velocity << std::endl;
    
    isActive_ = true;
    currentMidiNote_ = midiNoteNumber;
    currentVelocity_ = velocity;
    
    // Handle pitch bend
    pitchBend_ = (currentPitchWheelPosition - 8192) / 8192.0f; // Convert to -1.0 to +1.0 range
    
    // Set target pitch (MIDI note + pitch bend)
    float targetPitch = static_cast<float>(midiNoteNumber) + pitchBend_ * 2.0f; // ±2 semitones pitch bend range
    smoothedPitch_.setTargetValue(targetPitch);
    
    // Initialize the Braids engine if not already done
    if (!braidsEngine_.isInitialized()) {
        braidsEngine_.initialize(sampleRate_);
    }
    
    // Trigger ADSR
    adsr_.noteOn();
    
    // For percussion algorithms, trigger a strike
    int algorithm = braidsEngine_.getAlgorithm();
    if (algorithm >= 32 && algorithm <= 38) { // Percussion algorithms
        braidsEngine_.strike();
    }
    
    updatePitch();
}

void BraidsVoice::stopNote(float velocity, bool allowTailOff) {
    juce::ignoreUnused(velocity);
    
    if (allowTailOff) {
        adsr_.noteOff();
    } else {
        // Force immediate stop
        isActive_ = false;
        adsr_.reset();
        braidsEngine_.reset();
        clearCurrentNote();
    }
}

void BraidsVoice::pitchWheelMoved(int newPitchWheelValue) {
    pitchBend_ = (newPitchWheelValue - 8192) / 8192.0f; // Convert to -1.0 to +1.0 range
    
    if (isActive_ && currentMidiNote_ >= 0) {
        float targetPitch = static_cast<float>(currentMidiNote_) + pitchBend_ * 2.0f; // ±2 semitones
        smoothedPitch_.setTargetValue(targetPitch);
        updatePitch();
    }
}

void BraidsVoice::controllerMoved(int controllerNumber, int newControllerValue) {
    // Handle CC messages for real-time parameter control
    float normalizedValue = newControllerValue / 127.0f;
    
    switch (controllerNumber) {
        case 1: // Mod wheel -> Parameter 1
            smoothedParam1_.setTargetValue(normalizedValue);
            parameter1_ = normalizedValue;
            break;
        case 2: // Breath controller -> Parameter 2
            smoothedParam2_.setTargetValue(normalizedValue);
            parameter2_ = normalizedValue;
            break;
        case 74: // Filter cutoff (typical) -> Parameter 1
            smoothedParam1_.setTargetValue(normalizedValue);
            parameter1_ = normalizedValue;
            break;
        case 71: // Filter resonance (typical) -> Parameter 2
            smoothedParam2_.setTargetValue(normalizedValue);
            parameter2_ = normalizedValue;
            break;
        default:
            break;
    }
}

void BraidsVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) {
    if (!isActive_ || !braidsEngine_.isInitialized()) {
        static int warnCount = 0;
        if (++warnCount < 5) {
            std::cout << "[DEBUG] BraidsVoice::renderNextBlock - skipping (active=" 
                      << isActive_ << " initialized=" << braidsEngine_.isInitialized() << ")" << std::endl;
        }
        return;
    }
    
    // Check if ADSR is still active
    if (!adsr_.isActive()) {
        isActive_ = false;
        clearCurrentNote();
        return;
    }
    
    // Prepare temporary buffer for this voice
    juce::AudioBuffer<float> tempBuffer(1, numSamples);
    tempBuffer.clear();
    
    // Update smoothed parameters
    bool pitchChanged = false;
    bool paramsChanged = false;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        if (smoothedPitch_.isSmoothing()) {
            smoothedPitch_.getNextValue();
            pitchChanged = true;
        }
        
        if (smoothedParam1_.isSmoothing()) {
            smoothedParam1_.getNextValue();
            paramsChanged = true;
        }
        
        if (smoothedParam2_.isSmoothing()) {
            smoothedParam2_.getNextValue();
            paramsChanged = true;
        }
    }
    
    // Apply parameter updates
    if (pitchChanged) {
        updatePitch();
    }
    
    if (paramsChanged) {
        braidsEngine_.setParameters(smoothedParam1_.getCurrentValue(), smoothedParam2_.getCurrentValue());
    }
    
    // Process audio through Braids engine
    float* audioData = tempBuffer.getWritePointer(0);
    braidsEngine_.processAudio(audioData, numSamples);
    
    // Debug: Check if we got any audio
    static int debugCounter = 0;
    if (++debugCounter % 100 == 0) {
        float maxVal = 0;
        for (int i = 0; i < numSamples; ++i) {
            maxVal = std::max(maxVal, std::abs(audioData[i]));
        }
        std::cout << "[DEBUG] BraidsVoice - max audio: " << maxVal 
                  << " note: " << currentMidiNote_ << std::endl;
    }
    
    // Apply ADSR envelope and add to output buffer
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        float* channelData = outputBuffer.getWritePointer(channel, startSample);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float envelopeValue = adsr_.getNextSample();
            float processedSample = audioData[sample] * envelopeValue * currentVelocity_;
            channelData[sample] += processedSample;
        }
    }
}

void BraidsVoice::setAlgorithm(int algorithm) {
    braidsEngine_.setAlgorithm(algorithm);
}

int BraidsVoice::getAlgorithm() const {
    return braidsEngine_.getAlgorithm();
}

void BraidsVoice::setParameters(float param1, float param2) {
    parameter1_ = std::clamp(param1, 0.0f, 1.0f);
    parameter2_ = std::clamp(param2, 0.0f, 1.0f);
    
    smoothedParam1_.setTargetValue(parameter1_);
    smoothedParam2_.setTargetValue(parameter2_);
    
    braidsEngine_.setParameters(parameter1_, parameter2_);
}

std::pair<float, float> BraidsVoice::getParameters() const {
    return {parameter1_, parameter2_};
}

void BraidsVoice::strike() {
    braidsEngine_.strike();
}

bool BraidsVoice::isVoiceActive() const {
    return isActive_;
}

int BraidsVoice::getCurrentMidiNote() const {
    return currentMidiNote_;
}

float BraidsVoice::getCurrentVelocity() const {
    return currentVelocity_;
}

void BraidsVoice::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update ADSR sample rate
    adsr_.setSampleRate(sampleRate);
    
    // Set smoothing times (in samples)
    double smoothingTimeMs = 10.0; // 10ms smoothing
    int smoothingSamples = static_cast<int>(sampleRate * smoothingTimeMs / 1000.0);
    
    smoothedPitch_.reset(sampleRate, smoothingTimeMs / 1000.0);
    smoothedParam1_.reset(sampleRate, smoothingTimeMs / 1000.0);
    smoothedParam2_.reset(sampleRate, smoothingTimeMs / 1000.0);
    
    // Initialize Braids engine with new sample rate
    braidsEngine_.initialize(sampleRate);
}

void BraidsVoice::updatePitch() {
    float currentPitch = smoothedPitch_.getCurrentValue();
    braidsEngine_.setPitch(currentPitch);
}

float BraidsVoice::midiNoteToFrequency(float midiNote) const {
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

} // namespace BraidyAdapter