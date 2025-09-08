#include <cstdio>
#include <cstdint>

// Include just the core types and math we need
#include "Source/BraidyCore/BraidyTypes.h"
#include "Source/BraidyCore/BraidyConstants.h"

namespace braidy {

// Minimal test of ComputePhaseIncrement function
extern uint32_t ComputePhaseIncrement(int16_t midi_pitch);

void TestComputePhaseIncrement() {
    printf("=== Testing ComputePhaseIncrement directly ===\n");
    
    // Test various pitches
    int16_t test_pitches[] = {
        kPitchC4,           // Middle C
        kPitchC4 + kOctave, // C5  
        kPitchC4 - kOctave, // C3
        0,                  // Very low
        32767,              // Very high
        kPitchC4 + 64       // Slightly sharp C4
    };
    
    for (int i = 0; i < 6; i++) {
        int16_t pitch = test_pitches[i];
        printf("Testing pitch: %d\n", pitch);
        uint32_t phase_inc = ComputePhaseIncrement(pitch);
        printf("Result: %u (0x%08X)\n\n", phase_inc, phase_inc);
    }
}

}

int main() {
    braidy::TestComputePhaseIncrement();
    return 0;
}