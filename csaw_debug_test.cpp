#include "Source/BraidyCore/AnalogOscillator.h"
#include "Source/BraidyCore/BraidyMath.h"
#include <cstdio>
#include <cstdint>

using namespace braidy;

int main() {
    printf("=== CSAW DEBUG TEST ===\n");
    
    // Initialize the analog oscillator
    AnalogOscillator osc;
    osc.Init();
    osc.set_shape(AnalogOscillatorShape::CSAW);
    
    // Test 1: Set a pitch (this should trigger our debug output)
    printf("\n--- Setting pitch to C4 ---\n");
    osc.set_pitch(kPitchC4);  // This should trigger our debug output
    
    // Test 2: Try different pitches
    printf("\n--- Setting pitch to C5 ---\n");
    osc.set_pitch(kPitchC4 + kOctave);
    
    printf("\n--- Setting pitch to C3 ---\n");
    osc.set_pitch(kPitchC4 - kOctave);
    
    // Test 3: Render some samples
    printf("\n--- Rendering CSAW samples ---\n");
    int16_t buffer[24];  // One block
    uint8_t sync[24] = {0};  // No sync
    uint8_t aux[24] = {0};   // Aux output
    
    osc.Render(sync, buffer, aux, 24);
    
    printf("CSAW output samples: ");
    for (int i = 0; i < 8; i++) {
        printf("%d ", buffer[i]);
    }
    printf("...\n");
    
    // Check for silence
    bool all_zero = true;
    for (int i = 0; i < 24; i++) {
        if (buffer[i] != 0) {
            all_zero = false;
            break;
        }
    }
    
    printf("CSAW produces %s\n", all_zero ? "SILENCE (BUG)" : "AUDIO (OK)");
    
    // Test 4: Render a few more blocks to see phase progression
    printf("\n--- Additional render blocks ---\n");
    for (int block = 0; block < 3; block++) {
        osc.Render(sync, buffer, aux, 24);
        printf("Block %d: first sample = %d\n", block, buffer[0]);
    }
    
    printf("\n=== CSAW TEST COMPLETE ===\n");
    return 0;
}