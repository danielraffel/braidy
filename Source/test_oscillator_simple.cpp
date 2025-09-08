// Simple test to debug why oscillator produces DC instead of oscillating waveform
#include <iostream>
#include <cmath>
#include <iomanip>
#include "BraidyCore/AnalogOscillator.h"
#include "BraidyCore/BraidyResources.h"
#include "BraidyCore/BraidyMath.h"

using namespace braidy;

int main() {
    std::cout << "\n=== OSCILLATOR PHASE DEBUG TEST ===" << std::endl;
    
    // Initialize resources
    InitializeResources();
    
    // Create oscillator
    AnalogOscillator osc;
    osc.Init();
    osc.set_shape(AnalogOscillatorShape::SAW);  // Simple saw
    
    // Test MIDI note 60 (middle C)
    int midiNote = 60;
    int16_t pitch = midiNote << 7;  // Convert to Braids pitch format
    
    std::cout << "\nTesting MIDI note " << midiNote << " (pitch = " << pitch << ")" << std::endl;
    
    // Calculate expected phase increment
    uint32_t expectedPhaseInc = ComputePhaseIncrement(pitch);
    std::cout << "ComputePhaseIncrement(" << pitch << ") = " << expectedPhaseInc << std::endl;
    
    // Set pitch
    osc.set_pitch(pitch);
    
    // Generate 100 samples and watch phase progression
    int16_t buffer[100];
    uint8_t aux[100];
    
    std::cout << "\nGenerating 100 samples..." << std::endl;
    std::cout << "Sample | Value | Phase (approx)" << std::endl;
    std::cout << "-------+-------+---------------" << std::endl;
    
    // We need to peek inside the oscillator to see phase
    // Since we can't directly access private members, let's look at output pattern
    
    for (int block = 0; block < 10; ++block) {
        int16_t miniBuffer[10];
        osc.Render(nullptr, miniBuffer, nullptr, 10);
        
        for (int i = 0; i < 10; ++i) {
            int sampleNum = block * 10 + i;
            buffer[sampleNum] = miniBuffer[i];
            
            // Show first 20 samples and last 10
            if (sampleNum < 20 || sampleNum >= 90) {
                std::cout << std::setw(6) << sampleNum << " | " 
                         << std::setw(6) << miniBuffer[i] << " | ";
                
                // Estimate phase from sawtooth value (should go from -32768 to 32767)
                float normalizedValue = miniBuffer[i] / 32768.0f;
                float estimatedPhase = (normalizedValue + 1.0f) / 2.0f;
                std::cout << std::fixed << std::setprecision(4) << estimatedPhase << std::endl;
            }
        }
    }
    
    // Analyze the pattern
    std::cout << "\n=== ANALYSIS ===" << std::endl;
    
    // Check if values are changing
    bool allSame = true;
    for (int i = 1; i < 100; ++i) {
        if (buffer[i] != buffer[0]) {
            allSame = false;
            break;
        }
    }
    
    if (allSame) {
        std::cout << "❌ ERROR: All samples have the same value (" << buffer[0] << ")" << std::endl;
        std::cout << "   This means phase is NOT advancing!" << std::endl;
    } else {
        // Calculate differences between consecutive samples
        int totalDiff = 0;
        int maxDiff = 0;
        int minDiff = INT32_MAX;
        
        for (int i = 1; i < 100; ++i) {
            int diff = buffer[i] - buffer[i-1];
            totalDiff += std::abs(diff);
            if (diff > maxDiff) maxDiff = diff;
            if (diff < minDiff) minDiff = diff;
        }
        
        float avgDiff = totalDiff / 99.0f;
        
        std::cout << "✓ Samples are changing" << std::endl;
        std::cout << "  Average difference: " << avgDiff << std::endl;
        std::cout << "  Max difference: " << maxDiff << std::endl;
        std::cout << "  Min difference: " << minDiff << std::endl;
        
        // For a sawtooth at ~261Hz (C4) at 48kHz sample rate:
        // We expect ~184 samples per cycle
        // So each sample should increase by ~356 (65536/184)
        float expectedDiff = 65536.0f / (48000.0f / 261.626f);
        std::cout << "  Expected difference for 261Hz: ~" << expectedDiff << std::endl;
        
        if (avgDiff < 10) {
            std::cout << "  ⚠️  Phase increment is too small!" << std::endl;
        }
    }
    
    // Check the actual phase increment calculation
    std::cout << "\n=== PHASE INCREMENT DEBUG ===" << std::endl;
    
    // Let's manually trace through ComputePhaseIncrement
    int16_t testPitch = pitch;
    std::cout << "Input pitch: " << testPitch << std::endl;
    
    // The function should calculate: freq = 440 * 2^((pitch - 69*128) / (12*128))
    // For MIDI 60: freq = 440 * 2^((60*128 - 69*128) / (12*128)) = 261.626 Hz
    // Phase inc = freq * 2^32 / 48000 = 261.626 * 89478.485 = 23,405,858
    
    uint32_t calculatedInc = ComputePhaseIncrement(testPitch);
    std::cout << "Calculated phase increment: " << calculatedInc << std::endl;
    std::cout << "Expected for 261.626 Hz: ~23,405,858" << std::endl;
    
    float ratio = calculatedInc / 23405858.0f;
    std::cout << "Ratio (actual/expected): " << ratio << std::endl;
    
    if (ratio < 0.5f || ratio > 2.0f) {
        std::cout << "❌ Phase increment is WAY off!" << std::endl;
    } else if (ratio < 0.9f || ratio > 1.1f) {
        std::cout << "⚠️  Phase increment is somewhat off" << std::endl;
    } else {
        std::cout << "✓ Phase increment looks correct" << std::endl;
    }
    
    return 0;
}