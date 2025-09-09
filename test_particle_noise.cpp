// Test specifically for PARTICLE_NOISE algorithm
#include <iostream>
#include <vector>
#include <cmath>
#include "Source/adapters/BraidsEngine.h"

using namespace BraidyAdapter;

int main() {
    std::cout << "Testing PARTICLE_NOISE algorithm specifically...\n\n";
    
    BraidsEngine engine;
    engine.initialize(48000.0);
    
    // Test buffer
    const int bufferSize = 512;
    float outputBuffer[bufferSize];
    
    // Set to PARTICLE_NOISE (algorithm 45)
    std::cout << "Setting algorithm to PARTICLE_NOISE (45)\n";
    engine.setAlgorithm(45);
    
    // Set middle C pitch
    engine.setPitch(60.0f);
    
    // Try different parameter combinations
    float paramCombos[][2] = {
        {0.0f, 0.0f},   // Min parameters
        {0.5f, 0.5f},   // Middle parameters
        {1.0f, 1.0f},   // Max parameters
        {0.2f, 0.8f},   // Different mix
        {0.8f, 0.2f},   // Another mix
        {0.1f, 0.1f},   // Low density
        {0.9f, 0.9f}    // High density
    };
    
    for (int combo = 0; combo < 7; ++combo) {
        std::cout << "\nTest " << combo << ": param1=" << paramCombos[combo][0] 
                  << ", param2=" << paramCombos[combo][1] << "\n";
        
        engine.setParameters(paramCombos[combo][0], paramCombos[combo][1]);
        
        // Also try striking it (in case it needs a trigger)
        engine.strike();
        
        // Process several blocks
        float maxSample = 0.0f;
        float sumSquared = 0.0f;
        
        for (int block = 0; block < 20; ++block) {
            engine.processAudio(outputBuffer, bufferSize, nullptr);
            
            for (int i = 0; i < bufferSize; ++i) {
                maxSample = std::max(maxSample, std::abs(outputBuffer[i]));
                sumSquared += outputBuffer[i] * outputBuffer[i];
            }
        }
        
        float rms = std::sqrt(sumSquared / (bufferSize * 20));
        
        if (maxSample > 0.001f) {
            std::cout << "  ✅ PASS (peak=" << maxSample << ", RMS=" << rms << ")\n";
        } else {
            std::cout << "  ❌ FAIL (silent, peak=" << maxSample << ")\n";
        }
    }
    
    // Try with different pitches
    std::cout << "\nTrying different pitches:\n";
    engine.setParameters(0.5f, 0.5f);
    
    for (float pitch = 20.0f; pitch <= 100.0f; pitch += 20.0f) {
        std::cout << "Pitch " << pitch << ": ";
        engine.setPitch(pitch);
        engine.strike();
        
        float maxSample = 0.0f;
        for (int block = 0; block < 10; ++block) {
            engine.processAudio(outputBuffer, bufferSize, nullptr);
            for (int i = 0; i < bufferSize; ++i) {
                maxSample = std::max(maxSample, std::abs(outputBuffer[i]));
            }
        }
        
        if (maxSample > 0.001f) {
            std::cout << "✅ (peak=" << maxSample << ")\n";
        } else {
            std::cout << "❌ (silent)\n";
        }
    }
    
    std::cout << "\nPARTICLE_NOISE test complete.\n";
    std::cout << "Note: PARTICLE_NOISE may require specific initialization or RNG seeding.\n";
    std::cout << "It's possible this algorithm has special requirements in the original hardware.\n";
    
    return 0;
}