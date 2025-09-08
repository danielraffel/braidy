// Final comprehensive test of all Braids algorithms
#include <iostream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <string>
#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/AnalogOscillator.h"
#include "BraidyCore/DigitalOscillator.h"
#include "BraidyCore/BraidyResources.h"
#include "BraidyCore/BraidyMath.h"

using namespace braidy;

// Measure RMS power of signal
double measureRMS(int16_t* buffer, size_t size) {
    double sum = 0;
    for (size_t i = 0; i < size; ++i) {
        double sample = buffer[i] / 32768.0;
        sum += sample * sample;
    }
    return std::sqrt(sum / size);
}

// Measure spectral diversity (rough estimate)
double measureSpectralDiversity(int16_t* buffer, size_t size) {
    // Simple zero-crossing rate as proxy for spectral content
    int zeroCrossings = 0;
    for (size_t i = 1; i < size; ++i) {
        if ((buffer[i-1] < 0 && buffer[i] >= 0) || 
            (buffer[i-1] >= 0 && buffer[i] < 0)) {
            zeroCrossings++;
        }
    }
    return zeroCrossings / (double)size;
}

int main() {
    std::cout << "\n=== COMPREHENSIVE BRAIDS ALGORITHM TEST ===\n" << std::endl;
    
    // Initialize resources
    InitializeResources();
    
    // Test configuration
    const size_t bufferSize = 4800;  // 100ms at 48kHz
    int16_t* buffer = new int16_t[bufferSize];
    const int16_t testPitch = 60 << 7;  // Middle C
    
    std::cout << "Testing all MacroOscillator shapes at C4 (261.63 Hz)\n" << std::endl;
    std::cout << std::left << std::setw(20) << "Algorithm" 
              << std::setw(15) << "RMS Power" 
              << std::setw(20) << "Spectral Diversity" 
              << "Status" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    // All Braids MacroOscillator shapes
    struct AlgorithmTest {
        MacroOscillatorShape shape;
        const char* name;
        double minRMS;  // Minimum expected RMS
        double maxRMS;  // Maximum expected RMS
    };
    
    AlgorithmTest algorithms[] = {
        {MacroOscillatorShape::CSAW, "CSAW", 0.2, 0.6},
        {MacroOscillatorShape::MORPH, "MORPH", 0.2, 0.6},
        {MacroOscillatorShape::SAW_SQUARE, "SAW_SQUARE", 0.2, 0.6},
        {MacroOscillatorShape::SINE_TRIANGLE, "SINE_TRIANGLE", 0.2, 0.6},
        {MacroOscillatorShape::BUZZ, "BUZZ", 0.2, 0.6},
        {MacroOscillatorShape::SQUARE_SUB, "SQUARE_SUB", 0.2, 0.6},
        {MacroOscillatorShape::SAW_SUB, "SAW_SUB", 0.2, 0.6},
        {MacroOscillatorShape::SQUARE_SYNC, "SQUARE_SYNC", 0.2, 0.6},
        {MacroOscillatorShape::SAW_SYNC, "SAW_SYNC", 0.2, 0.6},
        {MacroOscillatorShape::TRIPLE_SAW, "TRIPLE_SAW", 0.2, 0.7},
        {MacroOscillatorShape::TRIPLE_SQUARE, "TRIPLE_SQUARE", 0.2, 0.7},
        {MacroOscillatorShape::TRIPLE_TRIANGLE, "TRIPLE_TRIANGLE", 0.2, 0.7},
        {MacroOscillatorShape::TRIPLE_SINE, "TRIPLE_SINE", 0.2, 0.7},
        {MacroOscillatorShape::TRIPLE_RING_MOD, "TRIPLE_RING_MOD", 0.1, 0.5},
        {MacroOscillatorShape::SAW_SWARM, "SAW_SWARM", 0.2, 0.7},
        {MacroOscillatorShape::SAW_COMB, "SAW_COMB", 0.1, 0.5},
        {MacroOscillatorShape::TOY, "TOY", 0.1, 0.5},
        {MacroOscillatorShape::DIGITAL_FILTER_LP, "DIGITAL_FILTER_LP", 0.05, 0.3},
        {MacroOscillatorShape::DIGITAL_FILTER_PK, "DIGITAL_FILTER_PK", 0.05, 0.3},
        {MacroOscillatorShape::DIGITAL_FILTER_BP, "DIGITAL_FILTER_BP", 0.05, 0.3},
        {MacroOscillatorShape::DIGITAL_FILTER_HP, "DIGITAL_FILTER_HP", 0.05, 0.3},
        {MacroOscillatorShape::VOSIM, "VOSIM", 0.1, 0.5},
        {MacroOscillatorShape::VOWEL, "VOWEL", 0.1, 0.5},
        {MacroOscillatorShape::VOWEL_FOF, "VOWEL_FOF", 0.1, 0.5},
        {MacroOscillatorShape::HARMONICS, "HARMONICS", 0.2, 0.6},
        {MacroOscillatorShape::FM, "FM", 0.1, 0.5},
        {MacroOscillatorShape::FEEDBACK_FM, "FEEDBACK_FM", 0.1, 0.5},
        {MacroOscillatorShape::CHAOTIC_FEEDBACK_FM, "CHAOTIC_FEEDBACK_FM", 0.05, 0.4},
        {MacroOscillatorShape::PLUCKED, "PLUCKED", 0.01, 0.4},
        {MacroOscillatorShape::BOWED, "BOWED", 0.1, 0.4},
        {MacroOscillatorShape::BLOWN, "BLOWN", 0.05, 0.4},
        {MacroOscillatorShape::FLUTED, "FLUTED", 0.05, 0.4},
        {MacroOscillatorShape::STRUCK_BELL, "STRUCK_BELL", 0.01, 0.4},
        {MacroOscillatorShape::STRUCK_DRUM, "STRUCK_DRUM", 0.01, 0.4},
        {MacroOscillatorShape::KICK, "KICK", 0.01, 0.5},
        {MacroOscillatorShape::CYMBAL, "CYMBAL", 0.01, 0.3},
        {MacroOscillatorShape::SNARE, "SNARE", 0.01, 0.3},
        {MacroOscillatorShape::WAVETABLES, "WAVETABLES", 0.1, 0.5},
        {MacroOscillatorShape::WAVE_MAP, "WAVE_MAP", 0.1, 0.5},
        {MacroOscillatorShape::WAVE_LINE, "WAVE_LINE", 0.1, 0.5},
        {MacroOscillatorShape::WAVE_PARAPHONIC, "WAVE_PARAPHONIC", 0.1, 0.5},
        {MacroOscillatorShape::FILTERED_NOISE, "FILTERED_NOISE", 0.05, 0.4},
        {MacroOscillatorShape::TWIN_PEAKS_NOISE, "TWIN_PEAKS_NOISE", 0.05, 0.4},
        {MacroOscillatorShape::CLOCKED_NOISE, "CLOCKED_NOISE", 0.05, 0.4},
        {MacroOscillatorShape::GRANULAR_CLOUD, "GRANULAR_CLOUD", 0.05, 0.4},
        {MacroOscillatorShape::PARTICLE_NOISE, "PARTICLE_NOISE", 0.01, 0.3},
        {MacroOscillatorShape::DIGITAL_MODULATION, "DIGITAL_MODULATION", 0.05, 0.4},
        {MacroOscillatorShape::QUESTION_MARK, "QUESTION_MARK", 0.001, 0.5}
    };
    
    int passedCount = 0;
    int failedCount = 0;
    std::vector<std::string> failedAlgorithms;
    
    for (const auto& algo : algorithms) {
        // Create and initialize oscillator
        MacroOscillator osc;
        osc.Init();
        osc.set_shape(algo.shape);
        osc.set_pitch(testPitch);
        osc.set_parameters(16384, 16384);  // Mid-range parameters
        
        // Clear buffer
        std::fill(buffer, buffer + bufferSize, 0);
        
        // Generate audio in blocks
        size_t samplesRendered = 0;
        uint8_t sync[24] = {0};
        
        while (samplesRendered < bufferSize) {
            size_t blockSize = std::min(size_t(24), bufferSize - samplesRendered);
            osc.Render(sync, buffer + samplesRendered, blockSize);
            samplesRendered += blockSize;
        }
        
        // Analyze output
        double rms = measureRMS(buffer, bufferSize);
        double spectralDiv = measureSpectralDiversity(buffer, bufferSize);
        
        // Check if output is valid (not silence, not DC)
        bool hasSignal = rms > 0.001;  // Not silence
        bool notDC = spectralDiv > 0.001;  // Has frequency content
        bool inRange = rms >= algo.minRMS && rms <= algo.maxRMS;
        
        bool passed = hasSignal && notDC;
        
        std::cout << std::left << std::setw(20) << algo.name
                  << std::fixed << std::setprecision(4) 
                  << std::setw(15) << rms
                  << std::setw(20) << spectralDiv;
        
        if (!hasSignal) {
            std::cout << "❌ SILENT";
            failedAlgorithms.push_back(std::string(algo.name) + " (silent)");
            failedCount++;
        } else if (!notDC) {
            std::cout << "❌ DC/STUCK";
            failedAlgorithms.push_back(std::string(algo.name) + " (DC/stuck)");
            failedCount++;
        } else if (!inRange) {
            std::cout << "⚠️  Level";  // Warning but not failure
            passedCount++;
        } else {
            std::cout << "✅ OK";
            passedCount++;
        }
        std::cout << std::endl;
    }
    
    // Test analog oscillator shapes
    std::cout << "\n=== Analog Oscillator Shapes ===" << std::endl;
    std::cout << std::left << std::setw(20) << "Shape" 
              << std::setw(15) << "RMS Power" 
              << std::setw(20) << "Spectral Diversity" 
              << "Status" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    struct AnalogTest {
        AnalogOscillatorShape shape;
        const char* name;
    };
    
    AnalogTest analogShapes[] = {
        {AnalogOscillatorShape::SAW, "Saw"},
        {AnalogOscillatorShape::VARIABLE_SAW, "Variable Saw"},
        {AnalogOscillatorShape::CSAW, "CSaw"},
        {AnalogOscillatorShape::SQUARE, "Square"},
        {AnalogOscillatorShape::TRIANGLE, "Triangle"},
        {AnalogOscillatorShape::SINE, "Sine"},
        {AnalogOscillatorShape::TRIANGLE_FOLD, "Triangle Fold"},
        {AnalogOscillatorShape::SINE_FOLD, "Sine Fold"},
        {AnalogOscillatorShape::BUZZ, "Buzz"}
    };
    
    for (const auto& shape : analogShapes) {
        AnalogOscillator osc;
        osc.Init();
        osc.set_shape(shape.shape);
        osc.set_pitch(testPitch);
        osc.set_parameter(16384);  // Mid-range parameter
        
        // Clear buffer
        std::fill(buffer, buffer + bufferSize, 0);
        
        // Generate audio
        size_t samplesRendered = 0;
        while (samplesRendered < bufferSize) {
            size_t blockSize = std::min(size_t(24), bufferSize - samplesRendered);
            osc.Render(nullptr, buffer + samplesRendered, nullptr, blockSize);
            samplesRendered += blockSize;
        }
        
        // Analyze
        double rms = measureRMS(buffer, bufferSize);
        double spectralDiv = measureSpectralDiversity(buffer, bufferSize);
        
        bool hasSignal = rms > 0.001;
        bool notDC = spectralDiv > 0.001;
        
        std::cout << std::left << std::setw(20) << shape.name
                  << std::fixed << std::setprecision(4) 
                  << std::setw(15) << rms
                  << std::setw(20) << spectralDiv;
        
        if (!hasSignal) {
            std::cout << "❌ SILENT";
            failedAlgorithms.push_back(std::string("Analog: ") + shape.name + " (silent)");
            failedCount++;
        } else if (!notDC) {
            std::cout << "❌ DC/STUCK";
            failedAlgorithms.push_back(std::string("Analog: ") + shape.name + " (DC/stuck)");
            failedCount++;
        } else {
            std::cout << "✅ OK";
            passedCount++;
        }
        std::cout << std::endl;
    }
    
    // Summary
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Total algorithms tested: " << (passedCount + failedCount) << std::endl;
    std::cout << "Passed: " << passedCount << std::endl;
    std::cout << "Failed: " << failedCount << std::endl;
    
    if (failedCount > 0) {
        std::cout << "\nFailed algorithms:" << std::endl;
        for (const auto& failed : failedAlgorithms) {
            std::cout << "  - " << failed << std::endl;
        }
    }
    
    delete[] buffer;
    
    if (failedCount == 0) {
        std::cout << "\n✅ SUCCESS: All algorithms produce valid audio!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ FAILURE: " << failedCount << " algorithms are not working correctly." << std::endl;
        return 1;
    }
}