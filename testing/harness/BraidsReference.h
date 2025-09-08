#pragma once

#include <vector>
#include <cstdint>
#include <random>

// Forward declarations for braids components
namespace braids {
    class MacroOscillator;
}

/**
 * Reference implementation wrapper for the original Braids MacroOscillator.
 * This class provides a clean interface to generate reference audio output
 * using the original Mutable Instruments Braids code.
 */
class BraidsReference {
public:
    /**
     * Constructor initializes the reference oscillator.
     * @param sampleRate Sample rate (should be 48000 for Braids compatibility)
     * @param blockSize Processing block size (24 samples is Braids standard)
     */
    BraidsReference(float sampleRate = 48000.0f, int blockSize = 24);
    
    /**
     * Destructor cleans up the oscillator instance.
     */
    ~BraidsReference();
    
    /**
     * Initialize the oscillator with deterministic settings.
     * @param randomSeed Seed for deterministic random number generation
     */
    void initialize(uint32_t randomSeed = 42);
    
    /**
     * Set the oscillator shape/algorithm.
     * @param shape MacroOscillatorShape enum value
     */
    void setShape(int shape);
    
    /**
     * Set the fundamental frequency in Hz.
     * @param frequency Frequency in Hz (typically 20-20000)
     */
    void setFrequency(float frequency);
    
    /**
     * Set the timbre parameter (0.0 to 1.0).
     * @param timbre Normalized timbre value
     */
    void setTimbre(float timbre);
    
    /**
     * Set the color parameter (0.0 to 1.0).
     * @param color Normalized color value
     */
    void setColor(float color);
    
    /**
     * Set the auxiliary parameter value (0.0 to 1.0).
     * This is typically the FM amount or other shape-specific parameter.
     * @param aux Normalized auxiliary parameter value
     */
    void setAux(float aux);
    
    /**
     * Generate a block of audio samples.
     * @param outputBuffer Buffer to write samples to (must be at least blockSize samples)
     * @return Number of samples actually generated
     */
    int generateBlock(float* outputBuffer);
    
    /**
     * Generate multiple blocks of audio.
     * @param outputBuffer Buffer to write samples to
     * @param numSamples Total number of samples to generate
     * @return Number of samples actually generated
     */
    int generateSamples(float* outputBuffer, int numSamples);
    
    /**
     * Reset the oscillator state (phase, etc.).
     */
    void reset();
    
    /**
     * Get the current sample rate.
     * @return Sample rate in Hz
     */
    float getSampleRate() const { return sampleRate_; }
    
    /**
     * Get the current block size.
     * @return Block size in samples
     */
    int getBlockSize() const { return blockSize_; }
    
    /**
     * Enable/disable the internal low-pass filter.
     * @param enabled True to enable the filter
     */
    void setFilterEnabled(bool enabled);
    
    /**
     * Set the internal VCA/envelope amount.
     * @param level VCA level (0.0 to 1.0)
     */
    void setLevel(float level);

private:
    braids::MacroOscillator* oscillator_;
    
    float sampleRate_;
    int blockSize_;
    
    // Internal buffers for Braids processing
    std::vector<uint8_t> sync_buffer_;
    std::vector<int16_t> output_buffer_;
    
    // Parameter tracking
    int currentShape_;
    float currentFrequency_;
    float currentTimbre_;
    float currentColor_;
    float currentAux_;
    bool filterEnabled_;
    float level_;
    
    // Random number generator for deterministic operation
    std::mt19937 rng_;
    
    // Helper methods
    void updateOscillatorSettings();
    int16_t frequencyToMidiNote(float frequency);
    uint16_t parameterToUint16(float param);
};