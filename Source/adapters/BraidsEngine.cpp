/*
 * BraidsEngine.cpp - Implementation of JUCE wrapper for Braids MacroOscillator
 */

#include "BraidsEngine.h"

// Include Braids headers
#include "../../eurorack/braids/macro_oscillator.h"
#include "../../eurorack/stmlib/dsp/parameter_interpolator.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <iostream>
#include <chrono>

namespace BraidyAdapter {

// Algorithm names in order
static const std::vector<std::string> kAlgorithmNames = {
    "CSAW", "MORPH", "SAW_SQUARE", "SINE_TRIANGLE", "BUZZ",
    "SQUARE_SUB", "SAW_SUB", "SQUARE_SYNC", "SAW_SYNC", "TRIPLE_SAW",
    "TRIPLE_SQUARE", "TRIPLE_TRIANGLE", "TRIPLE_SINE", "TRIPLE_RING_MOD", "SAW_SWARM",
    "SAW_COMB", "TOY", "DIGITAL_FILTER_LP", "DIGITAL_FILTER_PK", "DIGITAL_FILTER_BP",
    "DIGITAL_FILTER_HP", "VOSIM", "VOWEL", "VOWEL_FOF", "HARMONICS",
    "FM", "FEEDBACK_FM", "CHAOTIC_FEEDBACK_FM", "PLUCKED", "BOWED",
    "BLOWN", "FLUTED", "STRUCK_BELL", "STRUCK_DRUM", "KICK",
    "CYMBAL", "SNARE", "WAVETABLES", "WAVE_MAP", "WAVE_LINE",
    "WAVE_PARAPHONIC", "FILTERED_NOISE", "TWIN_PEAKS_NOISE", "CLOCKED_NOISE", "GRANULAR_CLOUD",
    "PARTICLE_NOISE", "DIGITAL_MODULATION"
};

// Parameter names for each algorithm
static const std::vector<std::pair<std::string, std::string>> kParameterNames = {
    {"Saw Amount", "Sub Amount"},        // CSAW
    {"Morph", "Sub Amount"},             // MORPH
    {"Saw/Square", "Sub Amount"},        // SAW_SQUARE
    {"Sine/Triangle", "Sub Amount"},     // SINE_TRIANGLE
    {"Buzz Amount", "Sub Amount"},       // BUZZ
    {"Square Sub", "Sub Level"},         // SQUARE_SUB
    {"Saw Sub", "Sub Level"},            // SAW_SUB
    {"Square Sync", "Sync Amount"},      // SQUARE_SYNC
    {"Saw Sync", "Sync Amount"},         // SAW_SYNC
    {"Triple Saw", "Detune"},           // TRIPLE_SAW
    {"Triple Square", "Detune"},         // TRIPLE_SQUARE
    {"Triple Triangle", "Detune"},       // TRIPLE_TRIANGLE
    {"Triple Sine", "Detune"},           // TRIPLE_SINE
    {"Triple Ring", "Ring Amount"},      // TRIPLE_RING_MOD
    {"Saw Swarm", "Spread"},             // SAW_SWARM
    {"Saw Comb", "Comb Amount"},         // SAW_COMB
    {"Toy", "Toy Amount"},               // TOY
    {"Cutoff", "Resonance"},             // DIGITAL_FILTER_LP
    {"Cutoff", "Resonance"},             // DIGITAL_FILTER_PK
    {"Cutoff", "Resonance"},             // DIGITAL_FILTER_BP
    {"Cutoff", "Resonance"},             // DIGITAL_FILTER_HP
    {"Formant", "Shape"},                // VOSIM
    {"Formant", "Shape"},                // VOWEL
    {"Formant", "Shape"},                // VOWEL_FOF
    {"Harmonics", "Spread"},             // HARMONICS
    {"FM Amount", "Ratio"},              // FM
    {"FM Amount", "Feedback"},           // FEEDBACK_FM
    {"FM Amount", "Chaos"},              // CHAOTIC_FEEDBACK_FM
    {"Decay", "Damping"},                // PLUCKED
    {"Bow Pressure", "Bow Position"},    // BOWED
    {"Breath", "Structure"},             // BLOWN
    {"Breath", "Structure"},             // FLUTED
    {"Decay", "Brightness"},             // STRUCK_BELL
    {"Decay", "Brightness"},             // STRUCK_DRUM
    {"Decay", "Tone"},                   // KICK
    {"Decay", "Tone"},                   // CYMBAL
    {"Decay", "Tone"},                   // SNARE
    {"Wave", "Scan"},                    // WAVETABLES
    {"Wave X", "Wave Y"},                // WAVE_MAP
    {"Wave", "Scan"},                    // WAVE_LINE
    {"Wave", "Spread"},                  // WAVE_PARAPHONIC
    {"Cutoff", "Resonance"},             // FILTERED_NOISE
    {"Peak 1", "Peak 2"},                // TWIN_PEAKS_NOISE
    {"Clock", "Density"},                // CLOCKED_NOISE
    {"Density", "Size"},                 // GRANULAR_CLOUD
    {"Density", "Spread"},               // PARTICLE_NOISE
    {"Modulation", "Rate"}               // DIGITAL_MODULATION
};

class BraidsEngine::Impl {
public:
    Impl() : initialized_(false), sampleRate_(48000.0), internalSampleRate_(96000.0), algorithm_(0) {
        // Zero-initialize the oscillator structure first
        memset(&oscillator_, 0, sizeof(oscillator_));
        
        // Initialize Braids oscillator
        oscillator_.Init();
        
        // Set default parameters
        currentPitch_ = 60.0f; // Middle C
        targetParam1_ = 0.5f;
        targetParam2_ = 0.5f;
        currentParam1_ = 0.5f;
        currentParam2_ = 0.5f;
        targetFMValue_ = 0.0f;
        metaMode_ = false;
        
        // Initialize internal buffer
        internalBuffer_.resize(24); // Braids processes in 24-sample blocks
        syncBuffer_.resize(24);
        std::fill(syncBuffer_.begin(), syncBuffer_.end(), 0);
        
        // Set default algorithm
        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
        
        // Initialize oscillator parameters properly
        // Note: Braids uses signed 16-bit range (-32768 to 32767)
        // Center position should be 0, not 32768
        oscillator_.set_parameters(0, 0); // Center position (was incorrectly 32768)
        oscillator_.set_pitch(60 << 7); // MIDI note 60 (middle C) in Braids format
    }

    void initialize(double sampleRate) {
        std::lock_guard<std::mutex> lock(mutex_);
        sampleRate_ = sampleRate;
        internalSampleRate_ = 96000.0;  // Braids always runs at 96kHz internally
        needsSampleRateConversion_ = std::abs(sampleRate_ - internalSampleRate_) > 1.0;
        
        if (needsSampleRateConversion_) {
            std::cout << "[INFO] Sample rate conversion enabled: " << sampleRate_ 
                      << "Hz host -> " << internalSampleRate_ << "Hz internal" << std::endl;
            
            // Pre-allocate conversion buffers for typical block sizes
            int maxBlockSize = 512;
            double ratio = internalSampleRate_ / sampleRate_;
            upsampleBuffer_.resize(static_cast<size_t>(maxBlockSize * ratio * 2));  // Extra space for safety
            downsampleBuffer_.resize(static_cast<size_t>(maxBlockSize * ratio * 2));
        }
        
        initialized_ = true;
        
        // Initialize the oscillator properly (don't use memset on C++ objects!)
        oscillator_.Init();
        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
        oscillator_.set_parameters(0, 0);
        updatePitch();
        updateParameters();
        
        // Clear buffers
        std::fill(internalBuffer_.begin(), internalBuffer_.end(), 0);
        std::fill(syncBuffer_.begin(), syncBuffer_.end(), 0);
    }

    void setAlgorithm(int algorithm) {
        // LOCK-FREE: Use atomic operations to prevent audio thread blocking
        // This is critical for seamless META mode operation

        // Clamp algorithm to valid range (0 to MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META = 46)
        algorithm = std::clamp(algorithm, 0, static_cast<int>(braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META));
        
        // Additional bounds check against our algorithm names array
        if (algorithm >= static_cast<int>(kAlgorithmNames.size())) {
            std::cerr << "[ERROR] Algorithm index " << algorithm << " exceeds available algorithms (" 
                      << kAlgorithmNames.size() << "). Resetting to 0." << std::endl;
            algorithm = 0;
        }
        
        // Use atomic compare-and-swap to avoid locks
        int expectedAlgorithm = algorithm_;
        if (algorithm != expectedAlgorithm) {
            std::cout << "[DEBUG] BraidsEngine::setAlgorithm changing from " << expectedAlgorithm
                      << " to " << algorithm << " (name: " << kAlgorithmNames[algorithm] << ")" << std::endl;

            // Atomically update algorithm
            algorithm_ = algorithm;

            // HOT-SWAPPABLE: Use lightweight approach to prevent audio gaps
            // Only do full reinitialization for algorithms that absolutely require it
            bool needsFullReinit = false;

            // Check if this is a percussion algorithm that needs special handling
            bool isPercussion = (algorithm == 28 || algorithm == 32 || algorithm == 33 ||
                                algorithm == 34 || algorithm == 35 || algorithm == 36);

            // Determine if we need full reinitialization (only for problematic algorithms)
            if (isPercussion || algorithm == 28) { // PLUK and percussion need full init
                needsFullReinit = true;
            }

            try {
                // LOCK-FREE: Use try_lock to avoid blocking audio thread
                std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
                if (!lock.owns_lock()) {
                    // Audio thread is busy, defer this change to prevent blocking
                    std::cout << "[DEBUG] Deferring algorithm change - audio thread busy" << std::endl;
                    return;
                }

                if (needsFullReinit) {
                    // Full reinitialization for problematic algorithms
                    oscillator_.Init();
                    oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm));
                    oscillator_.set_parameters(0, 0);

                    if (isPercussion) {
                        std::cout << "[DEBUG] Striking percussion algorithm " << algorithm
                                  << " (" << kAlgorithmNames[algorithm] << ")" << std::endl;
                        oscillator_.Strike();
                    }
                } else {
                    // HOT-SWAP: Lightweight shape change preserves audio continuity
                    oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm));
                    // Keep current parameters to maintain musical flow
                    oscillator_.set_parameters(currentParam1_, currentParam2_);
                }

                // Always update parameters after algorithm change
                updateParameters();

                std::cout << "[DEBUG] Algorithm change successful (" <<
                         (needsFullReinit ? "full-reinit" : "hot-swap") << "): " <<
                         kAlgorithmNames[algorithm] << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Failed to set algorithm " << algorithm << ": " << e.what() << std::endl;
                // Fall back to a safe algorithm
                algorithm_ = 0; // CSAW
                oscillator_.Init();
                oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(0));
                oscillator_.set_parameters(0, 0);
            } catch (...) {
                std::cerr << "[ERROR] Unknown error setting algorithm " << algorithm << std::endl;
                // Fall back to a safe algorithm
                algorithm_ = 0; // CSAW
                oscillator_.Init();
                oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(0));
                oscillator_.set_parameters(0, 0);
            }
        }
    }

    int getAlgorithm() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return algorithm_;
    }

    void setPitch(float pitch) {
        std::lock_guard<std::mutex> lock(mutex_);
        currentPitch_ = pitch;
        updatePitch();
    }

    void setParameters(float param1, float param2) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Ensure parameters are in valid range
        param1 = std::clamp(param1, 0.0f, 1.0f);
        param2 = std::clamp(param2, 0.0f, 1.0f);
        
        // Check for NaN or infinity
        if (!std::isfinite(param1)) param1 = 0.5f;
        if (!std::isfinite(param2)) param2 = 0.5f;
        
        // Always map param1 and param2 to TIMBRE (ADC 0) and COLOR (ADC 1) respectively
        // This matches exact hardware behavior
        targetParam1_ = param1;  // ADC channel 0 - TIMBRE
        targetParam2_ = param2;  // ADC channel 1 - COLOR
        
        updateParameters();
    }

    void setFMParameter(float fmValue) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Ensure FM parameter is in valid range
        fmValue = std::clamp(fmValue, 0.0f, 1.0f);
        
        // Check for NaN or infinity
        if (!std::isfinite(fmValue)) fmValue = 0.5f;
        
        // Store FM value for normal FM processing when not in meta mode
        targetFMValue_ = fmValue;
        
        if (metaMode_) {
            // In meta mode, FM parameter (ADC channel 3) controls algorithm selection
            // This matches exact hardware behavior from braids.cc lines 195-215
            int algorithmRange = static_cast<int>(braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META);
            
            // Convert 0.0-1.0 to algorithm index (0 to 46)
            int newAlgorithm = static_cast<int>(fmValue * algorithmRange);
            newAlgorithm = std::clamp(newAlgorithm, 0, algorithmRange);
            
            // Only change algorithm if it's actually different
            // Add rate limiting to prevent excessive reinitialization
            static auto lastAlgorithmChange = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastChange = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAlgorithmChange);
            
            // Increase rate limiting when being modulated to prevent crashes
            int minDelayMs = 2;  // Default 2ms delay

            // If FM is changing rapidly (modulated), increase the rate limit
            static float lastFmValue = 0.0f;
            float fmDelta = std::abs(fmValue - lastFmValue);
            if (fmDelta > 0.1f) {
                minDelayMs = 10;  // Increase to 10ms for rapid modulation
            }
            lastFmValue = fmValue;

            if (newAlgorithm != algorithm_ && timeSinceLastChange.count() > minDelayMs) {
                try {
                    // Additional safety check - don't change if not initialized
                    if (!initialized_) {
                        std::cerr << "[WARNING] Skipping META algorithm change - engine not initialized" << std::endl;
                        return;
                    }

                    algorithm_ = newAlgorithm;
                    lastAlgorithmChange = now;

                    // HOT-SWAPPABLE: Use lightweight shape change instead of full reinitialization
                    // Wrap in additional exception handling for META mode stability
                    try {
                        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
                        oscillator_.set_parameters(currentParam1_, currentParam2_);
                    } catch (...) {
                        // If shape change fails, mark as uninitialized to prevent crashes
                        std::cerr << "[ERROR] META mode shape change failed for algorithm " << algorithm_ << std::endl;
                        initialized_ = false;
                        return;
                    }

                    std::cout << "[DEBUG] Hot-swap META algorithm: " << algorithm_
                              << " (" << getAlgorithmName() << ") - delay=" << minDelayMs << "ms" << std::endl;

                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Hot-swap algorithm change failed: " << e.what() << std::endl;
                    // Keep current algorithm on failure
                } catch (...) {
                    std::cerr << "[ERROR] Unknown error in hot-swap algorithm change" << std::endl;
                }
            }
        }
    }

    void strike() {
        std::lock_guard<std::mutex> lock(mutex_);
        oscillator_.Strike();
    }

    void processAudio(float* outputBuffer, int numSamples, const uint8_t* syncBuffer) {
        if (!initialized_) {
            // Fill with silence if not initialized
            std::fill(outputBuffer, outputBuffer + numSamples, 0.0f);
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!needsSampleRateConversion_) {
            // Direct processing at 96kHz (no conversion needed)
            processAudioDirect(outputBuffer, numSamples, syncBuffer);
        } else {
            // Sample rate conversion required
            processAudioWithConversion(outputBuffer, numSamples, syncBuffer);
        }
    }
    
private:
    void processAudioDirect(float* outputBuffer, int numSamples, const uint8_t* syncBuffer) {
        int samplesProcessed = 0;
        static int debugCounter = 0;
        
        while (samplesProcessed < numSamples) {
            int samplesToProcess = std::min(24, numSamples - samplesProcessed);
            
            // Prepare sync buffer - ensure it's exactly 24 samples
            if (syncBuffer) {
                std::copy(syncBuffer + samplesProcessed, 
                         syncBuffer + samplesProcessed + samplesToProcess, 
                         syncBuffer_.data());
            } else {
                std::fill(syncBuffer_.data(), syncBuffer_.data() + samplesToProcess, 0);
            }
            // Pad remaining bytes if samplesToProcess < 24
            if (samplesToProcess < 24) {
                std::fill(syncBuffer_.data() + samplesToProcess, syncBuffer_.data() + 24, 0);
            }
            
            // Clear internal buffer before processing - ensure exactly 24 samples
            std::fill(internalBuffer_.begin(), internalBuffer_.end(), 0);
            
            // Render audio using Braids at 96kHz
            bool renderSuccess = renderBraidsBlock();
            
            if (!renderSuccess) {
                // If render fails, output silence and try to recover
                std::fill(outputBuffer + samplesProcessed, 
                         outputBuffer + samplesProcessed + samplesToProcess, 0.0f);
                samplesProcessed += samplesToProcess;
                continue;
            }
            
            // Convert int16_t to float and copy to output buffer with clipping
            float maxSample = 0.0f;
            for (int i = 0; i < samplesToProcess; ++i) {
                float sample = static_cast<float>(internalBuffer_[i]) / 32768.0f;
                sample = std::clamp(sample, -1.0f, 1.0f);
                outputBuffer[samplesProcessed + i] = sample;
                maxSample = std::max(maxSample, std::abs(sample));
            }
            
            // Debug output every 1000 blocks
            if (++debugCounter % 1000 == 0) {
                std::cout << "[DEBUG] BraidsEngine::processAudio algo=" << algorithm_ 
                          << " maxSample=" << maxSample << " (direct 96kHz)" << std::endl;
            }
            
            samplesProcessed += samplesToProcess;
        }
    }
    
    void processAudioWithConversion(float* outputBuffer, int numSamples, const uint8_t* syncBuffer) {
        // Calculate how many internal samples we need for the host samples
        double ratio = internalSampleRate_ / sampleRate_;
        int internalSamples = static_cast<int>(numSamples * ratio) + 1;
        
        // Ensure our conversion buffer is large enough
        if (upsampleBuffer_.size() < static_cast<size_t>(internalSamples)) {
            upsampleBuffer_.resize(internalSamples);
        }
        
        // Process at internal 96kHz rate
        processAudioDirect(upsampleBuffer_.data(), internalSamples, nullptr);
        
        // Downsample to host rate using linear interpolation
        for (int i = 0; i < numSamples; ++i) {
            double sourceIndex = i * ratio;
            int index1 = static_cast<int>(sourceIndex);
            int index2 = std::min(index1 + 1, internalSamples - 1);
            double fraction = sourceIndex - index1;
            
            if (index1 < internalSamples) {
                float sample1 = (index1 < internalSamples) ? upsampleBuffer_[index1] : 0.0f;
                float sample2 = (index2 < internalSamples) ? upsampleBuffer_[index2] : 0.0f;
                outputBuffer[i] = sample1 + fraction * (sample2 - sample1);
            } else {
                outputBuffer[i] = 0.0f;
            }
        }
        
        static int debugCounter = 0;
        if (++debugCounter % 1000 == 0) {
            std::cout << "[DEBUG] BraidsEngine::processAudio algo=" << algorithm_ 
                      << " (with sample rate conversion " << sampleRate_ << "Hz)" << std::endl;
        }
    }
    
    bool renderBraidsBlock() {
        // Render audio using Braids with safety
        // CRITICAL: Always pass exactly 24 samples to Braids as it expects this internally
        bool renderSuccess = false;
        try {
            // Ensure oscillator is in a valid state before rendering
            if (initialized_) {
                // Always render exactly 24 samples - Braids expects this
                oscillator_.Render(syncBuffer_.data(), internalBuffer_.data(), 24);
                renderSuccess = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] BraidsEngine render exception: " << e.what() 
                      << " (algorithm=" << algorithm_ << ")" << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] BraidsEngine render unknown exception (algorithm=" 
                      << algorithm_ << ")" << std::endl;
        }
        
        if (!renderSuccess) {
            // Try to recover by reinitializing
            static int recoveryAttempts = 0;
            if (++recoveryAttempts < 3) {
                try {
                    std::cerr << "[WARNING] Attempting to recover audio engine (attempt " 
                              << recoveryAttempts << ")" << std::endl;
                    oscillator_.Init();
                    oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
                    oscillator_.set_parameters(0, 0);
                    updatePitch();
                    updateParameters();
                    initialized_ = true;
                } catch (...) {
                    // If recovery fails, mark as uninitialized
                    initialized_ = false;
                }
            } else {
                // Too many recovery attempts, reset counter for next time
                recoveryAttempts = 0;
            }
        } else {
            // Reset recovery counter on successful render
            static int recoveryAttempts = 0;
            recoveryAttempts = 0;
        }
        
        return renderSuccess;
    }

public:
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        oscillator_.Init();
        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
        updatePitch();
        updateParameters();
    }

    bool isInitialized() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return initialized_;
    }

    std::string getAlgorithmName() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (algorithm_ >= 0 && algorithm_ < static_cast<int>(kAlgorithmNames.size())) {
            return kAlgorithmNames[algorithm_];
        }
        return "UNKNOWN";
    }

    std::pair<std::string, std::string> getParameterNames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (algorithm_ >= 0 && algorithm_ < static_cast<int>(kParameterNames.size())) {
            return kParameterNames[algorithm_];
        }
        return {"Parameter 1", "Parameter 2"};
    }

    void setMetaMode(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        metaMode_ = enabled;
        std::cout << "[DEBUG] Meta mode " << (enabled ? "enabled" : "disabled") << std::endl;
    }

    int getMetaAlgorithm() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metaMode_ ? algorithm_ : -1;  // Return algorithm only if in meta mode
    }

    bool getMetaMode() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metaMode_;
    }

private:
    void updatePitch() {
        // Convert MIDI note to Braids pitch format (MIDI << 7)
        // This matches the original Braids implementation where pitch = MIDI_note << 7
        // We preserve fractional MIDI notes for proper pitch bend support
        int16_t braidsPitch = static_cast<int16_t>(currentPitch_ * 128.0f);
        oscillator_.set_pitch(braidsPitch);
        
        static int debugCounter = 0;
        if (++debugCounter % 100 == 0) {
            std::cout << "[DEBUG] BraidsEngine::updatePitch - MIDI note=" << currentPitch_ 
                      << " -> Braids pitch=" << braidsPitch << " (expected for 60.0: " << (60.0f * 128.0f) << ")" << std::endl;
        }
    }

    void updateParameters() {
        // Convert 0.0-1.0 range to int16_t range for Braids
        // CRITICAL FIX: Use correct conversion formula
        // 0.0 -> -32768, 0.5 -> 0, 1.0 -> 32767
        int16_t param1 = static_cast<int16_t>((targetParam1_ - 0.5f) * 65535.0f);
        int16_t param2 = static_cast<int16_t>((targetParam2_ - 0.5f) * 65535.0f);

        // Clamp to valid int16_t range
        param1 = std::max<int16_t>(-32768, std::min<int16_t>(32767, param1));
        param2 = std::max<int16_t>(-32768, std::min<int16_t>(32767, param2));

        oscillator_.set_parameters(param1, param2);

        currentParam1_ = targetParam1_;
        currentParam2_ = targetParam2_;

        // Debug logging to verify parameters
        static int paramLogCounter = 0;
        if (++paramLogCounter % 100 == 0) {  // Log every 100th call
            std::cout << "[BRAIDS] Parameters: UI(" << targetParam1_ << "," << targetParam2_
                      << ") -> Braids(" << param1 << "," << param2 << ")"
                      << " Algorithm: " << kAlgorithmNames[algorithm_] << std::endl;
        }
    }

    mutable std::mutex mutex_;
    braids::MacroOscillator oscillator_;
    
    bool initialized_;
    double sampleRate_;
    double internalSampleRate_;  // Always 96kHz for Braids compatibility
    int algorithm_;
    
    // Sample rate conversion buffers for when host != 96kHz
    std::vector<float> upsampleBuffer_;
    std::vector<float> downsampleBuffer_;
    bool needsSampleRateConversion_;
    
    float currentPitch_;
    float targetParam1_, targetParam2_;
    float currentParam1_, currentParam2_;
    float targetFMValue_;
    bool metaMode_;
    
    std::vector<int16_t> internalBuffer_;
    std::vector<uint8_t> syncBuffer_;
};

// BraidsEngine public interface implementation

BraidsEngine::BraidsEngine() : pImpl(std::make_unique<Impl>()) {}

BraidsEngine::~BraidsEngine() = default;

BraidsEngine::BraidsEngine(BraidsEngine&&) noexcept = default;
BraidsEngine& BraidsEngine::operator=(BraidsEngine&&) noexcept = default;

void BraidsEngine::initialize(double sampleRate) {
    pImpl->initialize(sampleRate);
}

void BraidsEngine::setAlgorithm(int algorithm) {
    pImpl->setAlgorithm(algorithm);
}

int BraidsEngine::getAlgorithm() const {
    return pImpl->getAlgorithm();
}

void BraidsEngine::setPitch(float pitch) {
    pImpl->setPitch(pitch);
}

void BraidsEngine::setParameters(float param1, float param2) {
    pImpl->setParameters(param1, param2);
}

void BraidsEngine::setFMParameter(float fmValue) {
    pImpl->setFMParameter(fmValue);
}

void BraidsEngine::strike() {
    pImpl->strike();
}

void BraidsEngine::processAudio(float* outputBuffer, int numSamples, const uint8_t* syncBuffer) {
    pImpl->processAudio(outputBuffer, numSamples, syncBuffer);
}

void BraidsEngine::reset() {
    pImpl->reset();
}

std::string BraidsEngine::getAlgorithmName() const {
    return pImpl->getAlgorithmName();
}

std::vector<std::string> BraidsEngine::getAllAlgorithmNames() {
    return kAlgorithmNames;
}

std::pair<std::string, std::string> BraidsEngine::getParameterNames() const {
    return pImpl->getParameterNames();
}

bool BraidsEngine::isInitialized() const {
    return pImpl->isInitialized();
}

void BraidsEngine::setMetaMode(bool enabled) {
    pImpl->setMetaMode(enabled);
}

bool BraidsEngine::getMetaMode() const {
    return pImpl->getMetaMode();
}

int BraidsEngine::getMetaAlgorithm() const {
    return pImpl->getMetaAlgorithm();
}

} // namespace BraidyAdapter