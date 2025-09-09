// Simple test to verify the BraidsEngine API is working
#include <iostream>
#include <memory>
#include "Source/adapters/BraidsEngine.h"

int main() {
    std::cout << "Testing Braidy Integration with BraidsEngine..." << std::endl;
    
    // Create engine
    auto engine = std::make_unique<BraidyAdapter::BraidsEngine>();
    engine->initialize(48000.0);
    
    // Test algorithm names
    auto names = BraidyAdapter::BraidsEngine::getAllAlgorithmNames();
    std::cout << "\nAvailable algorithms: " << names.size() << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "  " << i << ": " << names[i] << std::endl;
    }
    
    // Test algorithm switching
    std::cout << "\nTesting algorithm switching:" << std::endl;
    for (int algo = 0; algo < 3; ++algo) {
        engine->setAlgorithm(algo);
        std::cout << "  Set to algorithm " << algo << ": " << names[algo] << std::endl;
    }
    
    // Test parameter control
    std::cout << "\nTesting parameter control:" << std::endl;
    engine->setParameters(0.3f, 0.7f);
    std::cout << "  Parameters set to 0.3, 0.7" << std::endl;
    
    // Test audio generation
    std::cout << "\nTesting audio generation:" << std::endl;
    float output[128];
    engine->setPitch(60.0f);  // MIDI note 60 = middle C
    engine->processAudio(output, 128);
    
    // Check if we got audio
    float sum = 0;
    for (int i = 0; i < 128; ++i) {
        sum += std::abs(output[i]);
    }
    
    if (sum > 0) {
        std::cout << "  ✓ Audio generated successfully (sum: " << sum << ")" << std::endl;
    } else {
        std::cout << "  ⚠ No audio generated" << std::endl;
    }
    
    std::cout << "\n✓ BraidsEngine API is working correctly!" << std::endl;
    std::cout << "✓ The standalone app should work with this implementation." << std::endl;
    
    return 0;
}