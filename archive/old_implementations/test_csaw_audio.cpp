// Automated test to verify CSAW audio generation without manual testing
// This will generate audio and analyze it to ensure proper waveform generation

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/BraidyTypes.h"
#include "BraidyCore/BraidyResources.h"
#include "BraidyCore/BraidyMath.h"

using namespace braidy;

// Simple FFT for frequency analysis
class SimpleFFT {
public:
    static float findDominantFrequency(const std::vector<float>& samples, float sampleRate) {
        // Use zero-crossing detection for simple frequency estimation
        int zeroCrossings = 0;
        for (size_t i = 1; i < samples.size(); ++i) {
            if ((samples[i-1] < 0 && samples[i] >= 0) || 
                (samples[i-1] >= 0 && samples[i] < 0)) {
                zeroCrossings++;
            }
        }
        
        float duration = samples.size() / sampleRate;
        float frequency = (zeroCrossings / 2.0f) / duration;
        return frequency;
    }
    
    static float computeRMS(const std::vector<float>& samples) {
        float sum = 0;
        for (float s : samples) {
            sum += s * s;
        }
        return std::sqrt(sum / samples.size());
    }
    
    static float computePeakToRMS(const std::vector<float>& samples) {
        float peak = 0;
        for (float s : samples) {
            float abs_s = std::abs(s);
            if (abs_s > peak) peak = abs_s;
        }
        float rms = computeRMS(samples);
        return rms > 0 ? peak / rms : 0;
    }
};

class CSawTest {
private:
    MacroOscillator oscillator;
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr size_t BLOCK_SIZE = 24;  // Braids uses 24-sample blocks
    
public:
    CSawTest() {
        InitializeResources();  // Initialize wavetables
        oscillator.Init();
        // Note: MacroOscillator doesn't have set_sample_rate, it uses fixed 48kHz internally
    }
    
    struct TestResult {
        float measuredFrequency;
        float expectedFrequency;
        float frequencyError;
        float rms;
        float peakToRMS;
        bool hasAudio;
        bool frequencyCorrect;
        std::string diagnosis;
    };
    
    TestResult testNote(int midiNote) {
        TestResult result;
        
        // Set up oscillator for CSAW
        oscillator.set_shape(MacroOscillatorShape::CSAW);
        
        // Convert MIDI note to Braids pitch format
        int16_t pitch = (midiNote << 7);
        oscillator.set_pitch(pitch);
        
        // Set neutral parameters
        oscillator.set_parameters(16384, 16384);  // Mid-range TIMBRE and COLOR
        
        // Generate 1 second of audio
        size_t totalSamples = SAMPLE_RATE;
        std::vector<int16_t> audioBuffer;
        audioBuffer.reserve(totalSamples);
        
        // Render in 24-sample blocks like Braids
        for (size_t i = 0; i < totalSamples / BLOCK_SIZE; ++i) {
            int16_t block[BLOCK_SIZE];
            oscillator.Render(nullptr, block, BLOCK_SIZE);
            
            for (size_t j = 0; j < BLOCK_SIZE; ++j) {
                audioBuffer.push_back(block[j]);
            }
        }
        
        // Convert to float for analysis
        std::vector<float> floatBuffer;
        floatBuffer.reserve(audioBuffer.size());
        for (int16_t sample : audioBuffer) {
            floatBuffer.push_back(sample / 32768.0f);
        }
        
        // Analyze the audio
        result.rms = SimpleFFT::computeRMS(floatBuffer);
        result.hasAudio = result.rms > 0.001f;  // Check if we have any signal
        
        if (result.hasAudio) {
            result.measuredFrequency = SimpleFFT::findDominantFrequency(floatBuffer, SAMPLE_RATE);
            result.expectedFrequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
            result.frequencyError = std::abs(result.measuredFrequency - result.expectedFrequency) / result.expectedFrequency;
            result.frequencyCorrect = result.frequencyError < 0.05f;  // Within 5% of expected
            result.peakToRMS = SimpleFFT::computePeakToRMS(floatBuffer);
            
            // Diagnose issues
            if (!result.frequencyCorrect) {
                result.diagnosis = "Frequency error too large - phase increment calculation likely wrong";
            } else if (result.peakToRMS > 3.0f) {
                result.diagnosis = "High peak-to-RMS ratio - likely producing clicks/impulses instead of sawtooth";
            } else if (result.peakToRMS < 1.2f) {
                result.diagnosis = "Low peak-to-RMS ratio - waveform might be too sinusoidal";
            } else {
                result.diagnosis = "Waveform appears correct";
            }
        } else {
            result.diagnosis = "No audio output detected - oscillator not generating signal";
            result.measuredFrequency = 0;
            result.expectedFrequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
            result.frequencyError = 1.0f;
            result.frequencyCorrect = false;
            result.peakToRMS = 0;
        }
        
        return result;
    }
    
    void runFullTest() {
        std::cout << "\n=== CSAW AUDIO GENERATION TEST ===\n" << std::endl;
        
        // Test several MIDI notes
        int testNotes[] = {48, 60, 69, 72};  // C3, C4, A4, C5
        const char* noteNames[] = {"C3", "C4", "A4", "C5"};
        
        bool allTestsPassed = true;
        
        for (int i = 0; i < 4; ++i) {
            TestResult result = testNote(testNotes[i]);
            
            std::cout << "Testing MIDI Note " << testNotes[i] 
                      << " (" << noteNames[i] << "):" << std::endl;
            std::cout << "  Expected Frequency: " << result.expectedFrequency << " Hz" << std::endl;
            std::cout << "  Measured Frequency: " << result.measuredFrequency << " Hz" << std::endl;
            std::cout << "  Frequency Error: " << (result.frequencyError * 100) << "%" << std::endl;
            std::cout << "  RMS Level: " << result.rms << std::endl;
            std::cout << "  Peak-to-RMS Ratio: " << result.peakToRMS << std::endl;
            std::cout << "  Has Audio: " << (result.hasAudio ? "YES" : "NO") << std::endl;
            std::cout << "  Frequency Correct: " << (result.frequencyCorrect ? "YES" : "NO") << std::endl;
            std::cout << "  Diagnosis: " << result.diagnosis << std::endl;
            std::cout << std::endl;
            
            if (!result.hasAudio || !result.frequencyCorrect) {
                allTestsPassed = false;
            }
        }
        
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        if (allTestsPassed) {
            std::cout << "✅ ALL TESTS PASSED - CSAW is generating correct audio" << std::endl;
        } else {
            std::cout << "❌ TESTS FAILED - CSAW audio generation is broken" << std::endl;
            std::cout << "\nLikely issues:" << std::endl;
            std::cout << "1. Phase increment calculation is wrong" << std::endl;
            std::cout << "2. Wavetable lookup is broken" << std::endl;
            std::cout << "3. Sample rate mismatch" << std::endl;
            std::cout << "4. Integer overflow in phase accumulator" << std::endl;
        }
    }
};

int main() {
    CSawTest test;
    test.runFullTest();
    return 0;
}