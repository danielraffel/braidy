/*
 * BraidsEngine.cpp - Implementation of JUCE wrapper for Braids MacroOscillator
 */

#include "BraidsEngine.h"

// Include Braids headers
#include "../../eurorack/braids/macro_oscillator.h"
#include "../../eurorack/braids/settings.h"
#include "../../eurorack/stmlib/dsp/parameter_interpolator.h"

#include <algorithm>
#include <cmath>
#include <mutex>

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
    Impl() : initialized_(false), sampleRate_(48000.0), algorithm_(0) {
        // Initialize Braids oscillator
        oscillator_.Init();
        
        // Set default parameters
        currentPitch_ = 60.0f; // Middle C
        targetParam1_ = 0.5f;
        targetParam2_ = 0.5f;
        currentParam1_ = 0.5f;
        currentParam2_ = 0.5f;
        
        // Initialize internal buffer
        internalBuffer_.resize(24); // Braids processes in 24-sample blocks
        syncBuffer_.resize(24);
        
        // Set default algorithm
        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
    }

    void initialize(double sampleRate) {
        std::lock_guard<std::mutex> lock(mutex_);
        sampleRate_ = sampleRate;
        initialized_ = true;
        
        // Reset the oscillator
        oscillator_.Init();
        oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm_));
        updatePitch();
        updateParameters();
    }

    void setAlgorithm(int algorithm) {
        std::lock_guard<std::mutex> lock(mutex_);
        algorithm = std::clamp(algorithm, 0, static_cast<int>(braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META));
        
        if (algorithm != algorithm_) {
            algorithm_ = algorithm;
            oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(algorithm));
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
        targetParam1_ = std::clamp(param1, 0.0f, 1.0f);
        targetParam2_ = std::clamp(param2, 0.0f, 1.0f);
        updateParameters();
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
        
        int samplesProcessed = 0;
        
        while (samplesProcessed < numSamples) {
            int samplesToProcess = std::min(24, numSamples - samplesProcessed);
            
            // Prepare sync buffer
            if (syncBuffer) {
                std::copy(syncBuffer + samplesProcessed, 
                         syncBuffer + samplesProcessed + samplesToProcess, 
                         syncBuffer_.data());
            } else {
                std::fill(syncBuffer_.data(), syncBuffer_.data() + samplesToProcess, 0);
            }
            
            // Render audio using Braids
            oscillator_.Render(syncBuffer_.data(), internalBuffer_.data(), samplesToProcess);
            
            // Convert int16_t to float and copy to output buffer
            for (int i = 0; i < samplesToProcess; ++i) {
                outputBuffer[samplesProcessed + i] = static_cast<float>(internalBuffer_[i]) / 32768.0f;
            }
            
            samplesProcessed += samplesToProcess;
        }
    }

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

private:
    void updatePitch() {
        // Convert MIDI note to Braids pitch format (MIDI << 7)
        int16_t braidsPitch = static_cast<int16_t>(currentPitch_ * 128.0f);
        oscillator_.set_pitch(braidsPitch);
    }

    void updateParameters() {
        // Convert 0.0-1.0 range to int16_t range for Braids
        int16_t param1 = static_cast<int16_t>((targetParam1_ * 2.0f - 1.0f) * 32767.0f);
        int16_t param2 = static_cast<int16_t>((targetParam2_ * 2.0f - 1.0f) * 32767.0f);
        
        oscillator_.set_parameters(param1, param2);
        
        currentParam1_ = targetParam1_;
        currentParam2_ = targetParam2_;
    }

    mutable std::mutex mutex_;
    braids::MacroOscillator oscillator_;
    
    bool initialized_;
    double sampleRate_;
    int algorithm_;
    
    float currentPitch_;
    float targetParam1_, targetParam2_;
    float currentParam1_, currentParam2_;
    
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

} // namespace BraidyAdapter