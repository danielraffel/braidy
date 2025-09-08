/*
 * BraidsSynthesiser.cpp - Implementation of custom synthesiser class for Braids
 */

#include "BraidsSynthesiser.h"
#include <algorithm>
#include <chrono>

namespace BraidyAdapter {

BraidsSynthesiser::BraidsSynthesiser(int numVoices)
    : globalAlgorithm_(0)
    , globalParam1_(0.5f)
    , globalParam2_(0.5f)
    , maxPolyphony_(numVoices)
    , voiceStealingMode_(VoiceStealingMode::Oldest)
    , cpuLoadAverage_(32) // Average over 32 samples
    , currentSampleRate_(48000.0)
{
    // Initialize parameter CC mapping
    parameterCCMap_[0] = 1;  // Param1 -> Mod Wheel (CC1)
    parameterCCMap_[1] = 74; // Param2 -> Filter Cutoff (CC74)
    
    // Initialize performance stats
    stats_ = {};
    lastStatsUpdate_ = std::chrono::high_resolution_clock::now();
    
    // Add a BraidsSound (required by JUCE)
    addSound(new BraidsSound());
    
    // Initialize voices
    initializeVoices(numVoices);
}

BraidsSynthesiser::~BraidsSynthesiser() = default;

void BraidsSynthesiser::setCurrentPlaybackSampleRate(double sampleRate) {
    currentSampleRate_ = sampleRate;
    juce::Synthesiser::setCurrentPlaybackSampleRate(sampleRate);
    
    // Update all Braids voices with new sample rate
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* braidsVoice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
            braidsVoice->setSampleRate(sampleRate);
        }
    }
}

void BraidsSynthesiser::renderNextBlock(juce::AudioBuffer<float>& outputAudio, const juce::MidiBuffer& inputMidi, int startSample, int numSamples) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process MIDI CC messages for global parameter control
    for (const auto& midiMessage : inputMidi) {
        const auto& message = midiMessage.getMessage();
        
        if (message.isController()) {
            int ccNumber = message.getControllerNumber();
            float ccValue = message.getControllerValue() / 127.0f;
            
            // Check if this CC controls a global parameter
            for (int param = 0; param < 2; ++param) {
                if (parameterCCMap_[param] == ccNumber) {
                    updateParameterThreadSafe(param, ccValue);
                    break;
                }
            }
        }
    }
    
    // Call parent implementation
    juce::Synthesiser::renderNextBlock(outputAudio, inputMidi, startSample, numSamples);
    
    // Update performance statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double renderTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    int activeVoices = getActiveVoiceCount();
    
    updatePerformanceStats(renderTime, activeVoices);
}

void BraidsSynthesiser::setGlobalAlgorithm(int algorithm) {
    algorithm = std::clamp(algorithm, 0, 46); // 47 algorithms (0-46)
    globalAlgorithm_.store(algorithm);
    
    // Update all voices
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* braidsVoice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
            braidsVoice->setAlgorithm(algorithm);
        }
    }
}

int BraidsSynthesiser::getGlobalAlgorithm() const {
    return globalAlgorithm_.load();
}

void BraidsSynthesiser::setGlobalParameters(float param1, float param2) {
    param1 = std::clamp(param1, 0.0f, 1.0f);
    param2 = std::clamp(param2, 0.0f, 1.0f);
    
    globalParam1_.store(param1);
    globalParam2_.store(param2);
    
    broadcastParametersToVoices();
}

std::pair<float, float> BraidsSynthesiser::getGlobalParameters() const {
    return {globalParam1_.load(), globalParam2_.load()};
}

void BraidsSynthesiser::setMaxPolyphony(int numVoices) {
    if (numVoices != maxPolyphony_) {
        maxPolyphony_ = numVoices;
        
        // Clear existing voices
        clearVoices();
        
        // Re-initialize with new voice count
        initializeVoices(numVoices);
    }
}

int BraidsSynthesiser::getMaxPolyphony() const {
    return maxPolyphony_;
}

int BraidsSynthesiser::getActiveVoiceCount() const {
    int activeCount = 0;
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* braidsVoice = dynamic_cast<const BraidsVoice*>(getVoice(i))) {
            if (braidsVoice->isVoiceActive()) {
                ++activeCount;
            }
        }
    }
    return activeCount;
}

void BraidsSynthesiser::strikeAllVoices() {
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* braidsVoice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
            if (braidsVoice->isVoiceActive()) {
                braidsVoice->strike();
            }
        }
    }
}

void BraidsSynthesiser::setVoiceStealingMode(VoiceStealingMode mode) {
    voiceStealingMode_ = mode;
}

BraidsSynthesiser::VoiceStealingMode BraidsSynthesiser::getVoiceStealingMode() const {
    return voiceStealingMode_;
}

BraidsSynthesiser::PerformanceStats BraidsSynthesiser::getPerformanceStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void BraidsSynthesiser::resetPerformanceStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = {};
    cpuLoadAverage_.clear();
}

BraidsVoice* BraidsSynthesiser::getBraidsVoice(int voiceIndex) {
    if (voiceIndex >= 0 && voiceIndex < getNumVoices()) {
        return dynamic_cast<BraidsVoice*>(getVoice(voiceIndex));
    }
    return nullptr;
}

const BraidsVoice* BraidsSynthesiser::getBraidsVoice(int voiceIndex) const {
    if (voiceIndex >= 0 && voiceIndex < getNumVoices()) {
        return dynamic_cast<const BraidsVoice*>(getVoice(voiceIndex));
    }
    return nullptr;
}

std::vector<std::string> BraidsSynthesiser::getAllAlgorithmNames() const {
    return BraidsEngine::getAllAlgorithmNames();
}

std::pair<std::string, std::string> BraidsSynthesiser::getCurrentParameterNames() const {
    // Get parameter names from the first voice's engine
    if (auto* firstVoice = getBraidsVoice(0)) {
        return firstVoice->getBraidsEngine()->getParameterNames();
    }
    return {"Parameter 1", "Parameter 2"};
}

void BraidsSynthesiser::setParameterCC(int parameterIndex, int ccNumber) {
    if (parameterIndex >= 0 && parameterIndex < 2) {
        parameterCCMap_[parameterIndex] = ccNumber;
    }
}

int BraidsSynthesiser::getParameterCC(int parameterIndex) const {
    if (parameterIndex >= 0 && parameterIndex < 2) {
        return parameterCCMap_[parameterIndex];
    }
    return -1;
}

void BraidsSynthesiser::updateParameterThreadSafe(int parameterIndex, float value) {
    std::lock_guard<std::mutex> lock(parameterMutex_);
    
    value = std::clamp(value, 0.0f, 1.0f);
    
    if (parameterIndex == 0) {
        globalParam1_.store(value);
    } else if (parameterIndex == 1) {
        globalParam2_.store(value);
    }
    
    broadcastParametersToVoices();
}

void BraidsSynthesiser::initializeVoices(int numVoices) {
    // Add voices
    for (int i = 0; i < numVoices; ++i) {
        auto* voice = new BraidsVoice();
        voice->setSampleRate(currentSampleRate_);
        voice->setAlgorithm(globalAlgorithm_.load());
        voice->setParameters(globalParam1_.load(), globalParam2_.load());
        addVoice(voice);
    }
}

void BraidsSynthesiser::broadcastParametersToVoices() {
    float param1 = globalParam1_.load();
    float param2 = globalParam2_.load();
    
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* braidsVoice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
            braidsVoice->setParameters(param1, param2);
        }
    }
}

BraidsVoice* BraidsSynthesiser::findVoiceToSteal() {
    BraidsVoice* victimVoice = nullptr;
    
    switch (voiceStealingMode_) {
        case VoiceStealingMode::Oldest: {
            // Find the voice that has been playing the longest
            juce::uint32 oldestStartTime = 0xFFFFFFFF;
            for (int i = 0; i < getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
                    if (voice->isVoiceActive()) {
                        // JUCE doesn't expose voice start time directly, so use oldest active voice
                        if (!victimVoice) victimVoice = voice;
                    }
                }
            }
            break;
        }
        
        case VoiceStealingMode::Lowest: {
            int lowestNote = 128;
            for (int i = 0; i < getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
                    if (voice->isVoiceActive() && voice->getCurrentMidiNote() < lowestNote) {
                        lowestNote = voice->getCurrentMidiNote();
                        victimVoice = voice;
                    }
                }
            }
            break;
        }
        
        case VoiceStealingMode::Highest: {
            int highestNote = -1;
            for (int i = 0; i < getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
                    if (voice->isVoiceActive() && voice->getCurrentMidiNote() > highestNote) {
                        highestNote = voice->getCurrentMidiNote();
                        victimVoice = voice;
                    }
                }
            }
            break;
        }
        
        case VoiceStealingMode::Quietest: {
            float quietestVelocity = 1.0f;
            for (int i = 0; i < getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidsVoice*>(getVoice(i))) {
                    if (voice->isVoiceActive() && voice->getCurrentVelocity() < quietestVelocity) {
                        quietestVelocity = voice->getCurrentVelocity();
                        victimVoice = voice;
                    }
                }
            }
            break;
        }
    }
    
    return victimVoice;
}

void BraidsSynthesiser::updatePerformanceStats(double renderTime, int activeVoices) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    auto now = std::chrono::high_resolution_clock::now();
    auto timeSinceLastUpdate = std::chrono::duration<double>(now - lastStatsUpdate_).count();
    
    if (timeSinceLastUpdate >= 0.1) { // Update stats every 100ms
        float cpuLoad = static_cast<float>(renderTime / (timeSinceLastUpdate * 1000.0));
        cpuLoadAverage_.pushNextSampleValue(cpuLoad);
        
        stats_.averageCpuLoad = cpuLoadAverage_.getAverage();
        stats_.peakVoiceCount = std::max(stats_.peakVoiceCount, activeVoices);
        stats_.lastRenderTime = renderTime;
        
        lastStatsUpdate_ = now;
    }
}

// BraidsManager implementation

BraidsManager::BraidsManager(int numVoices)
    : synthesiser_(numVoices)
    , initialized_(false)
{
    // Initialize with default preset
    currentPreset_ = {0, 0.5f, 0.5f, "Default", "Default Braids preset"};
}

void BraidsManager::initialize(double sampleRate) {
    synthesiser_.setCurrentPlaybackSampleRate(sampleRate);
    initialized_ = true;
}

void BraidsManager::processAudio(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiBuffer) {
    if (!initialized_) return;
    
    synthesiser_.renderNextBlock(buffer, midiBuffer, 0, buffer.getNumSamples());
}

void BraidsManager::setAlgorithm(int algorithm) {
    synthesiser_.setGlobalAlgorithm(algorithm);
    currentPreset_.algorithm = algorithm;
}

void BraidsManager::setParameter1(float value) {
    auto params = synthesiser_.getGlobalParameters();
    synthesiser_.setGlobalParameters(value, params.second);
    currentPreset_.param1 = value;
}

void BraidsManager::setParameter2(float value) {
    auto params = synthesiser_.getGlobalParameters();
    synthesiser_.setGlobalParameters(params.first, value);
    currentPreset_.param2 = value;
}

void BraidsManager::loadPreset(const Preset& preset) {
    currentPreset_ = preset;
    synthesiser_.setGlobalAlgorithm(preset.algorithm);
    synthesiser_.setGlobalParameters(preset.param1, preset.param2);
}

BraidsManager::Preset BraidsManager::getCurrentPreset() const {
    return currentPreset_;
}

std::vector<BraidsManager::Preset> BraidsManager::getFactoryPresets() {
    return {
        {0, 0.3f, 0.2f, "Classic Saw", "Classic sawtooth with sub oscillator"},
        {4, 0.7f, 0.3f, "Buzz Lead", "Buzzing lead sound"},
        {24, 0.5f, 0.8f, "Harmonics", "Rich harmonic content"},
        {25, 0.4f, 0.6f, "FM Bell", "FM synthesis bell-like tone"},
        {32, 0.6f, 0.4f, "Struck Bell", "Metallic struck bell"},
        {37, 0.3f, 0.7f, "Wavetables", "Wavetable synthesis"},
        {41, 0.8f, 0.2f, "Filtered Noise", "Filtered noise texture"},
        {44, 0.5f, 0.5f, "Granular Cloud", "Granular synthesis cloud"}
    };
}

} // namespace BraidyAdapter