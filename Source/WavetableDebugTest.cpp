#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/WavetableManager.h" 
#include "BraidyCore/DSPDispatcher.h"
#include "BraidyCore/BraidyMath.h"
#include <cstdio>
#include <cstdint>

using namespace braidy;

int main() {
    printf("=== WAVETABLE DEBUG TEST ===\n");
    
    // Test 1: Check if wavetable data loads
    WavetableManager wm;
    wm.Init();
    
    // Test 2: Check ComputePhaseIncrement for a simple note
    int16_t test_pitch = kPitchC4; // Middle C
    uint32_t phase_inc = ComputePhaseIncrement(test_pitch);
    printf("Phase increment for C4 (%d): %u (0x%08X)\n", test_pitch, phase_inc, phase_inc);
    
    // Test 3: Try to render a single wavetable sample
    uint32_t test_phase = 0;
    uint16_t table_index = 0;  // First table
    int16_t sample = wm.RenderWavetable(test_phase, table_index);
    printf("Single wavetable sample at phase 0, table 0: %d\n", sample);
    
    // Test 4: Try WLIN algorithm directly
    printf("\n--- Testing WLIN algorithm ---\n");
    MacroOscillator osc;
    osc.Init();
    osc.set_shape(MacroOscillatorShape::WAVE_LINE);
    osc.set_pitch(kPitchC4);
    osc.set_parameters(16384, 16384);  // Mid-range parameters
    
    int16_t buffer[24];  // One block
    uint8_t sync[24] = {0};  // No sync
    
    osc.Render(sync, buffer, 24);
    
    printf("WLIN output samples: ");
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
    
    printf("WLIN produces %s\n", all_zero ? "SILENCE (BUG)" : "AUDIO (OK)");
    
    // Test 5: Test WMAP
    printf("\n--- Testing WMAP algorithm ---\n");
    osc.set_shape(MacroOscillatorShape::WAVE_MAP);
    osc.Render(sync, buffer, 24);
    
    all_zero = true;
    for (int i = 0; i < 24; i++) {
        if (buffer[i] != 0) {
            all_zero = false;
            break;
        }
    }
    printf("WMAP produces %s\n", all_zero ? "SILENCE (BUG)" : "AUDIO (OK)");
    
    // Test 6: Test WPAR
    printf("\n--- Testing WPAR algorithm ---\n");
    osc.set_shape(MacroOscillatorShape::WAVE_PARAPHONIC);
    osc.Render(sync, buffer, 24);
    
    all_zero = true;
    for (int i = 0; i < 24; i++) {
        if (buffer[i] != 0) {
            all_zero = false;
            break;
        }
    }
    printf("WPAR produces %s\n", all_zero ? "SILENCE (BUG)" : "AUDIO (OK)");
    
    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}