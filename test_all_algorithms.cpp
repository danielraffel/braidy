// Test program to verify all 48 algorithms produce audio
#include <iostream>
#include <cstring>
#include "Source/BraidyCore/MacroOscillator.h"
#include "Source/BraidyCore/BraidySettings.h"

using namespace braidy;

bool TestAlgorithm(MacroOscillatorShape shape, const char* name) {
    MacroOscillator osc;
    osc.Init();
    osc.set_shape(shape);
    osc.set_pitch(60 << 7); // Middle C
    
    // Test buffer
    int16_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Render audio
    osc.Render(nullptr, buffer, 256);
    
    // Check if any non-zero samples were produced
    bool hasAudio = false;
    int32_t sum = 0;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) {
            hasAudio = true;
            sum += abs(buffer[i]);
        }
    }
    
    float avgLevel = sum / 256.0f;
    
    std::cout << "[" << (int)shape << "] " << name << ": ";
    if (hasAudio) {
        std::cout << "✓ AUDIO (avg level: " << avgLevel << ")" << std::endl;
    } else {
        std::cout << "✗ NO AUDIO" << std::endl;
    }
    
    return hasAudio;
}

int main() {
    std::cout << "=== Testing All 48 Braidy Algorithms ===" << std::endl;
    
    int working = 0;
    int total = 0;
    
    // Test all algorithms
    for (int i = 0; i <= (int)MacroOscillatorShape::LAST_ALGORITHM; i++) {
        MacroOscillatorShape shape = static_cast<MacroOscillatorShape>(i);
        const char* name = GetAlgorithmName(shape);
        
        if (TestAlgorithm(shape, name)) {
            working++;
        }
        total++;
    }
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Working: " << working << "/" << total << std::endl;
    std::cout << "Broken: " << (total - working) << "/" << total << std::endl;
    
    if (working == total) {
        std::cout << "✓ All algorithms producing audio!" << std::endl;
    } else {
        std::cout << "✗ Some algorithms need fixing" << std::endl;
    }
    
    return (working == total) ? 0 : 1;
}