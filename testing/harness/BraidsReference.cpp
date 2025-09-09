#include "BraidsReference.h"
#include "../../eurorack/braids/macro_oscillator.h"
#include <cmath>
#include <algorithm>
#include <cstring>

BraidsReference::BraidsReference(float sampleRate, int blockSize)
    : oscillator_(nullptr)
    , sampleRate_(sampleRate)
    , blockSize_(blockSize)
    , currentShape_(0)
    , currentFrequency_(440.0f)
    , currentTimbre_(0.5f)
    , currentColor_(0.5f)
    , currentAux_(0.0f)
    , filterEnabled_(true)
    , level_(1.0f)
    , rng_(42) {
    
    // Initialize buffers
    sync_buffer_.resize(blockSize, 0);
    output_buffer_.resize(blockSize);
    
    // Create the oscillator instance
    oscillator_ = new braids::MacroOscillator();
}

BraidsReference::~BraidsReference() {
    delete oscillator_;
}

void BraidsReference::initialize(uint32_t randomSeed) {
    if (!oscillator_) return;
    
    // Seed the RNG for deterministic behavior
    rng_.seed(randomSeed);
    
    // Initialize the oscillator
    oscillator_->Init();
    
    // Set initial parameters
    updateOscillatorSettings();
    
    // Reset state
    reset();
}

void BraidsReference::setShape(int shape) {
    currentShape_ = shape;
    if (oscillator_) {
        oscillator_->set_shape(static_cast<braids::MacroOscillatorShape>(shape));
    }
}

void BraidsReference::setFrequency(float frequency) {
    currentFrequency_ = std::max(0.1f, std::min(20000.0f, frequency));
    updateOscillatorSettings();
}

void BraidsReference::setTimbre(float timbre) {
    currentTimbre_ = std::max(0.0f, std::min(1.0f, timbre));
    updateOscillatorSettings();
}

void BraidsReference::setColor(float color) {
    currentColor_ = std::max(0.0f, std::min(1.0f, color));
    updateOscillatorSettings();
}

void BraidsReference::setAux(float aux) {
    currentAux_ = std::max(0.0f, std::min(1.0f, aux));
    updateOscillatorSettings();
}

void BraidsReference::setFilterEnabled(bool enabled) {
    filterEnabled_ = enabled;
    updateOscillatorSettings();
}

void BraidsReference::setLevel(float level) {
    level_ = std::max(0.0f, std::min(1.0f, level));
}

int BraidsReference::generateBlock(float* outputBuffer) {
    if (!oscillator_ || !outputBuffer) return 0;
    
    // Generate sync buffer (all zeros for free-running)
    std::fill(sync_buffer_.begin(), sync_buffer_.end(), 0);
    
    // Render the block
    oscillator_->Render(sync_buffer_.data(), output_buffer_.data(), blockSize_);
    
    // Convert from int16_t to float and apply level
    const float scale = level_ / 32767.0f;
    for (int i = 0; i < blockSize_; ++i) {
        outputBuffer[i] = static_cast<float>(output_buffer_[i]) * scale;
    }
    
    return blockSize_;
}

int BraidsReference::generateSamples(float* outputBuffer, int numSamples) {
    if (!outputBuffer || numSamples <= 0) return 0;
    
    int totalGenerated = 0;
    float* writePtr = outputBuffer;
    
    while (totalGenerated < numSamples) {
        int samplesToGenerate = std::min(blockSize_, numSamples - totalGenerated);
        
        // Use a temporary buffer for the block
        std::vector<float> tempBuffer(blockSize_);
        int generated = generateBlock(tempBuffer.data());
        
        if (generated == 0) break;
        
        // Copy the requested number of samples
        std::memcpy(writePtr, tempBuffer.data(), samplesToGenerate * sizeof(float));
        writePtr += samplesToGenerate;
        totalGenerated += samplesToGenerate;
    }
    
    return totalGenerated;
}

void BraidsReference::reset() {
    if (oscillator_) {
        // Reset the oscillator's internal state
        oscillator_->Init();
        updateOscillatorSettings();
    }
}

void BraidsReference::updateOscillatorSettings() {
    if (!oscillator_) return;
    
    // Set shape
    oscillator_->set_shape(static_cast<braids::MacroOscillatorShape>(currentShape_));
    
    // Convert frequency to MIDI note (Braids uses pitch internally)
    int16_t pitch = frequencyToMidiNote(currentFrequency_);
    oscillator_->set_pitch(pitch);
    
    // Convert normalized parameters to uint16_t range
    oscillator_->set_parameters(
        parameterToUint16(currentTimbre_),  // timbre
        parameterToUint16(currentColor_)    // color
    );
    
    // Note: aux parameter is not directly settable in MacroOscillator
    // AD/VCA envelope methods don't exist in the actual Braids API
}

int16_t BraidsReference::frequencyToMidiNote(float frequency) {
    // Convert frequency to MIDI note * 128 (Braids pitch format)
    // MIDI note 69 (A4) = 440 Hz
    // Formula: note = 69 + 12 * log2(freq / 440)
    float midiNote = 69.0f + 12.0f * std::log2f(frequency / 440.0f);
    
    // Braids expects pitch as note * 128
    int16_t pitch = static_cast<int16_t>(midiNote * 128.0f);
    
    // Clamp to reasonable range
    return std::max(static_cast<int16_t>(0), std::min(static_cast<int16_t>(16383), pitch));
}

uint16_t BraidsReference::parameterToUint16(float param) {
    // Convert 0.0-1.0 range to 0-65535 range
    return static_cast<uint16_t>(param * 65535.0f);
}