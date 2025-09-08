// Test to verify oscillator frequencies are correct
#include <iostream>
#include <cmath>
#include <iomanip>
#include <vector>
#include "BraidyCore/AnalogOscillator.h"
#include "BraidyCore/BraidyResources.h"
#include "BraidyCore/BraidyMath.h"

using namespace braidy;

// Find zero crossings to measure frequency
double measureFrequency(int16_t* buffer, size_t size, double sampleRate) {
    int zeroCrossings = 0;
    int16_t lastSample = buffer[0];
    
    for (size_t i = 1; i < size; ++i) {
        int16_t currentSample = buffer[i];
        if ((lastSample < 0 && currentSample >= 0) || 
            (lastSample >= 0 && currentSample < 0)) {
            zeroCrossings++;
        }
        lastSample = currentSample;
    }
    
    // Each complete cycle has 2 zero crossings
    double cycles = zeroCrossings / 2.0;
    double duration = size / sampleRate;
    return cycles / duration;
}

int main() {
    std::cout << "\n=== FREQUENCY VERIFICATION TEST ===\n" << std::endl;
    
    // Initialize resources
    InitializeResources();
    
    // Test various MIDI notes
    struct TestNote {
        int midi;
        double expectedHz;
        const char* name;
    };
    
    TestNote testNotes[] = {
        {60, 261.626, "C4"},
        {69, 440.000, "A4"},
        {57, 220.000, "A3"},
        {72, 523.251, "C5"},
        {48, 130.813, "C3"},
        {84, 1046.50, "C6"}
    };
    
    std::cout << "Note | Expected Hz | Measured Hz | Error %" << std::endl;
    std::cout << "-----+-------------+-------------+---------" << std::endl;
    
    bool allPassed = true;
    
    for (const auto& note : testNotes) {
        AnalogOscillator osc;
        osc.Init();
        osc.set_shape(AnalogOscillatorShape::SAW);
        
        int16_t pitch = note.midi << 7;  // Braids pitch format
        osc.set_pitch(pitch);
        
        // Generate enough samples for accurate measurement
        const size_t bufferSize = 48000;  // 1 second of samples
        int16_t* buffer = new int16_t[bufferSize];
        
        // Render in blocks
        size_t samplesRendered = 0;
        while (samplesRendered < bufferSize) {
            size_t blockSize = std::min(size_t(24), bufferSize - samplesRendered);
            osc.Render(nullptr, buffer + samplesRendered, nullptr, blockSize);
            samplesRendered += blockSize;
        }
        
        double measuredHz = measureFrequency(buffer, bufferSize, 48000.0);
        double error = ((measuredHz - note.expectedHz) / note.expectedHz) * 100.0;
        
        std::cout << std::left << std::setw(4) << note.name << " | "
                  << std::fixed << std::setprecision(2) << std::setw(11) << note.expectedHz << " | "
                  << std::setw(11) << measuredHz << " | "
                  << std::showpos << error << "%" << std::noshowpos << std::endl;
        
        // Check if error is within 1%
        if (std::abs(error) > 1.0) {
            allPassed = false;
            std::cout << "  ⚠️  Error exceeds 1% threshold!" << std::endl;
        }
        
        delete[] buffer;
    }
    
    std::cout << "\n=== WAVEFORM SHAPE TEST ===" << std::endl;
    
    // Test that different waveforms produce different output
    AnalogOscillator osc;
    osc.Init();
    osc.set_pitch(60 << 7);  // C4
    
    const size_t testSize = 100;
    int16_t sawBuffer[testSize];
    int16_t sineBuffer[testSize];
    int16_t squareBuffer[testSize];
    
    // Generate saw
    osc.set_shape(AnalogOscillatorShape::SAW);
    osc.Render(nullptr, sawBuffer, nullptr, testSize);
    
    // Generate sine  
    osc.Init();  // Reset phase
    osc.set_pitch(60 << 7);
    osc.set_shape(AnalogOscillatorShape::SINE);
    osc.Render(nullptr, sineBuffer, nullptr, testSize);
    
    // Generate square
    osc.Init();  // Reset phase
    osc.set_pitch(60 << 7);
    osc.set_shape(AnalogOscillatorShape::SQUARE);
    osc.Render(nullptr, squareBuffer, nullptr, testSize);
    
    // Calculate RMS differences
    double sawSineDiff = 0, sawSquareDiff = 0, sineSquareDiff = 0;
    for (size_t i = 0; i < testSize; ++i) {
        sawSineDiff += std::pow(sawBuffer[i] - sineBuffer[i], 2);
        sawSquareDiff += std::pow(sawBuffer[i] - squareBuffer[i], 2);
        sineSquareDiff += std::pow(sineBuffer[i] - squareBuffer[i], 2);
    }
    
    sawSineDiff = std::sqrt(sawSineDiff / testSize);
    sawSquareDiff = std::sqrt(sawSquareDiff / testSize);
    sineSquareDiff = std::sqrt(sineSquareDiff / testSize);
    
    std::cout << "\nRMS differences between waveforms:" << std::endl;
    std::cout << "Saw vs Sine:   " << sawSineDiff << std::endl;
    std::cout << "Saw vs Square: " << sawSquareDiff << std::endl;
    std::cout << "Sine vs Square: " << sineSquareDiff << std::endl;
    
    if (sawSineDiff < 1000 || sawSquareDiff < 1000 || sineSquareDiff < 1000) {
        std::cout << "⚠️  Waveforms are too similar!" << std::endl;
        allPassed = false;
    } else {
        std::cout << "✓ Waveforms are distinct" << std::endl;
    }
    
    std::cout << "\n=== RESULT ===" << std::endl;
    if (allPassed) {
        std::cout << "✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "The oscillator is generating correct frequencies and waveforms." << std::endl;
    } else {
        std::cout << "❌ Some tests failed. See warnings above." << std::endl;
    }
    
    return allPassed ? 0 : 1;
}