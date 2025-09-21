/*
 * BraidsSynthesiser.h - Custom synthesiser class for Braids voices
 *
 * This class extends JUCE's Synthesiser to provide specialized management
 * for BraidsVoice instances. It handles polyphony, parameter broadcasting,
 * and voice allocation optimized for the Braids engine.
 *
 * Features:
 * - Polyphonic voice management (configurable, default 8 voices)
 * - Global parameter updates to all voices
 * - Voice stealing algorithms
 * - Performance monitoring and optimization
 * - Thread-safe parameter updates
 */

#pragma once

#include <JuceHeader.h>
#include "BraidsVoice.h"
#include "../Modulation/ModulationMatrix.h"
#include <atomic>
#include <mutex>

namespace BraidyAdapter {

/**
 * Custom synthesiser class optimized for Braids voices.
 * Extends JUCE::Synthesiser with Braids-specific functionality.
 */
class BraidsSynthesiser : public juce::Synthesiser
{
public:
    explicit BraidsSynthesiser(int numVoices = 8);
    ~BraidsSynthesiser() override;

    // Additional methods for parameter control
    void setAlgorithm(int algorithm);
    void setParameters(float param1, float param2);

    // Global parameter control (affects all voices)
    void setGlobalAlgorithm(int algorithm);
    int getGlobalAlgorithm() const;
    
    void setGlobalParameters(float param1, float param2);
    std::pair<float, float> getGlobalParameters() const;

    // Quantizer settings
    void setQuantizerSettings(bool enabled, int scale, int root);

    // Voice management
    void setMaxPolyphony(int numVoices);
    int getMaxPolyphony() const;
    int getActiveVoiceCount() const;
    
    // Strike all active voices (useful for percussion algorithms)
    void strikeAllVoices();
    
    // Voice allocation settings
    enum class VoiceStealingMode {
        Oldest,      // Steal oldest voice
        Quietest,    // Steal quietest voice  
        Lowest,      // Steal lowest note
        Highest      // Steal highest note
    };
    
    void setVoiceStealingMode(VoiceStealingMode mode);
    VoiceStealingMode getVoiceStealingMode() const;
    
    // Performance monitoring
    struct PerformanceStats {
        float averageCpuLoad;
        int peakVoiceCount;
        int voiceStealCount;
        double lastRenderTime;
    };
    
    PerformanceStats getPerformanceStats() const;
    void resetPerformanceStats();
    
    // Voice access for advanced control
    BraidsVoice* getBraidsVoice(int voiceIndex);
    const BraidsVoice* getBraidsVoice(int voiceIndex) const;
    
    // Utility methods
    std::vector<std::string> getAllAlgorithmNames() const;
    std::pair<std::string, std::string> getCurrentParameterNames() const;
    
    // MIDI CC mapping
    void setParameterCC(int parameterIndex, int ccNumber);
    int getParameterCC(int parameterIndex) const;
    
    // Thread-safe parameter updates
    void updateParameterThreadSafe(int parameterIndex, float value);
    
    // Modulation matrix for real-time modulation
    void setModulationMatrix(braidy::ModulationMatrix* matrix);
    
private:
    void initializeVoices(int numVoices);
    void broadcastParametersToVoices();
    BraidsVoice* findVoiceToSteal();
    void updatePerformanceStats(double renderTime, int activeVoices);
    
    // Global state
    std::atomic<int> globalAlgorithm_;
    std::atomic<float> globalParam1_;
    std::atomic<float> globalParam2_;

    // Quantizer state
    std::atomic<bool> quantizerEnabled_;
    std::atomic<int> quantizerScale_;
    std::atomic<int> quantizerRoot_;
    
    // Voice management
    int maxPolyphony_;
    VoiceStealingMode voiceStealingMode_;
    
    // MIDI CC mapping
    std::array<int, 2> parameterCCMap_; // param1, param2
    
    // Performance monitoring
    mutable std::mutex statsMutex_;
    PerformanceStats stats_;
    float cpuLoadAverage_;
    std::chrono::high_resolution_clock::time_point lastStatsUpdate_;
    
    // Thread safety
    mutable std::mutex parameterMutex_;
    
    // Sample rate
    double currentSampleRate_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsSynthesiser)
};

/**
 * Helper class for managing Braids synthesiser instances.
 * Provides high-level interface for common operations.
 */
class BraidsManager
{
public:
    explicit BraidsManager(int numVoices = 8);
    ~BraidsManager() = default;

    // High-level interface
    void initialize(double sampleRate);
    void processAudio(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiBuffer);
    
    // Parameter control
    void setAlgorithm(int algorithm);
    void setParameter1(float value);
    void setParameter2(float value);
    
    // Preset management
    struct Preset {
        int algorithm;
        float param1;
        float param2;
        std::string name;
        std::string description;
    };
    
    void loadPreset(const Preset& preset);
    Preset getCurrentPreset() const;
    
    // Access to underlying synthesiser
    BraidsSynthesiser& getSynthesiser() { return synthesiser_; }
    const BraidsSynthesiser& getSynthesiser() const { return synthesiser_; }
    
    // Factory presets
    static std::vector<Preset> getFactoryPresets();
    
private:
    BraidsSynthesiser synthesiser_;
    Preset currentPreset_;
    bool initialized_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsManager)
};

} // namespace BraidyAdapter