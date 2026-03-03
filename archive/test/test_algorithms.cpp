// Simple test to verify all 47 Braids algorithms produce audio
#include <iostream>
#include <vector>
#include <cmath>
#include "Source/adapters/BraidsEngine.h"

using namespace BraidyAdapter;

const std::vector<std::string> algorithmNames = {
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

int main() {
    std::cout << "Testing all 47 Braids algorithms...\n\n";
    
    BraidsEngine engine;
    engine.initialize(48000.0);
    
    // Test buffer
    const int bufferSize = 512;
    float outputBuffer[bufferSize];
    
    for (int algo = 0; algo < 47; ++algo) {
        std::cout << "Testing algorithm " << algo << ": " << algorithmNames[algo] << " ... ";
        
        // Set algorithm
        engine.setAlgorithm(algo);
        
        // Set middle C pitch
        engine.setPitch(60.0f);
        
        // Set moderate parameters
        engine.setParameters(0.5f, 0.5f);
        
        // For percussion algorithms, trigger a strike
        if (algo >= 32 && algo <= 37) {
            engine.strike();
        }
        
        // Process a few blocks to let the algorithm settle
        for (int block = 0; block < 10; ++block) {
            engine.processAudio(outputBuffer, bufferSize, nullptr);
        }
        
        // Check if any audio was generated
        float maxSample = 0.0f;
        float sumSquared = 0.0f;
        for (int i = 0; i < bufferSize; ++i) {
            maxSample = std::max(maxSample, std::abs(outputBuffer[i]));
            sumSquared += outputBuffer[i] * outputBuffer[i];
        }
        
        float rms = std::sqrt(sumSquared / bufferSize);
        
        if (maxSample > 0.001f) {
            std::cout << "✅ PASS (peak=" << maxSample << ", RMS=" << rms << ")\n";
        } else {
            std::cout << "❌ FAIL (silent, peak=" << maxSample << ")\n";
        }
    }
    
    std::cout << "\nTest complete!\n";
    return 0;
}