/*
 * BraidsEngine.h - JUCE wrapper for Braids MacroOscillator
 *
 * This wrapper provides a clean JUCE-compatible interface to the Braids DSP engine
 * while hiding the internal Braids implementation details using the pImpl pattern.
 *
 * Features:
 * - Wraps braids::MacroOscillator with all 47 algorithms
 * - Thread-safe for polyphonic operation
 * - Handles int16_t to float conversion correctly
 * - Parameter smoothing with stmlib::ParameterInterpolator
 * - Processes in 24-sample blocks as required by Braids
 */

#pragma once

#ifdef TEST
// For testing without JUCE
#include <string>
#include <vector>
#include <utility>
#else
#include <JuceHeader.h>
#endif
#include <memory>

namespace BraidyAdapter {

/**
 * Main wrapper class for the Braids DSP engine.
 * Uses pImpl pattern to hide Braids implementation details.
 */
class BraidsEngine
{
public:
    BraidsEngine();
    ~BraidsEngine();

    // Non-copyable, movable
    BraidsEngine(const BraidsEngine&) = delete;
    BraidsEngine& operator=(const BraidsEngine&) = delete;
    BraidsEngine(BraidsEngine&&) noexcept;
    BraidsEngine& operator=(BraidsEngine&&) noexcept;

    /**
     * Initialize the engine with the given sample rate.
     * Must be called before processing audio.
     */
    void initialize(double sampleRate);

    /**
     * Set the algorithm/shape for the oscillator.
     * @param algorithm 0-46 (47 total algorithms)
     */
    void setAlgorithm(int algorithm);
    
    /**
     * Get the current algorithm index.
     * @return Current algorithm (0-46)
     */
    int getAlgorithm() const;

    /**
     * Set the fundamental pitch.
     * @param pitch MIDI note number (can be fractional for fine tuning)
     */
    void setPitch(float pitch);

    /**
     * Set the two main parameters for the current algorithm.
     * @param param1 First parameter (0.0 to 1.0) - maps to TIMBRE (ADC channel 0)
     * @param param2 Second parameter (0.0 to 1.0) - maps to COLOR (ADC channel 1)
     */
    void setParameters(float param1, float param2);

    /**
     * Set the FM parameter (ADC channel 3).
     * In meta mode, this controls algorithm selection.
     * In normal mode, this controls FM depth/amount.
     * @param fmValue FM parameter (0.0 to 1.0)
     */
    void setFMParameter(float fmValue);

    /**
     * Trigger a strike/reset for percussion algorithms.
     */
    void strike();

    /**
     * Process audio samples.
     * @param outputBuffer Output buffer to fill
     * @param numSamples Number of samples to process
     * @param syncBuffer Optional sync input buffer (can be nullptr)
     */
    void processAudio(float* outputBuffer, int numSamples, const uint8_t* syncBuffer = nullptr);

    /**
     * Reset the engine state.
     */
    void reset();

    /**
     * Get the name of the current algorithm.
     * @return Algorithm name string
     */
    std::string getAlgorithmName() const;

    /**
     * Get all algorithm names.
     * @return Vector of all algorithm names
     */
    static std::vector<std::string> getAllAlgorithmNames();

    /**
     * Get parameter names for the current algorithm.
     * @return Pair of parameter names (param1, param2)
     */
    std::pair<std::string, std::string> getParameterNames() const;

    /**
     * Check if the engine is properly initialized.
     * @return true if initialized and ready for processing
     */
    bool isInitialized() const;

    /**
     * Enable/disable meta modulation mode.
     * When enabled, FM parameter (ADC channel 3) controls algorithm selection.
     * This matches exact hardware behavior where meta mode uses FM CV input.
     * @param enabled true to enable meta mode, false for normal mode
     */
    void setMetaMode(bool enabled);

    /**
     * Get the current meta modulation mode state.
     * @return true if meta mode is enabled
     */
    bool getMetaMode() const;

    /**
     * Get the current algorithm in meta mode for LED display.
     * @return Current algorithm index if meta mode is enabled, -1 otherwise
     */
    int getMetaAlgorithm() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace BraidyAdapter