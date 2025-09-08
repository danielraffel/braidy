# CSAW Algorithm Fix Plan

## Problem Summary
The CSAW algorithm produces "tiny clicks making high pitched short squawks" instead of a proper sawtooth wave. Analysis reveals fundamental issues:

1. **No anti-aliasing** - Using naive linear ramps creates massive aliasing
2. **Incorrect wavetable generation** - Values overflow int16_t range
3. **Sample rate mismatch** - Hardcoded 48kHz vs actual plugin sample rate
4. **Poor interpolation** - 8-bit lookup resolution too coarse

## The Original Braids Approach

The original Braids CSAW uses sophisticated **PolyBLEP** (Polynomial Bandlimited Step) anti-aliasing:

```cpp
// Original approach (simplified)
1. Generate naive sawtooth ramp
2. Detect discontinuities (phase wrap)
3. Apply PolyBLEP correction at discontinuities
4. Result: Bandlimited sawtooth without aliasing
```

## Immediate Fix: Proper Sawtooth Generation

### Step 1: Fix Wavetable Generation
```cpp
// Current (BROKEN):
saw_temp[i] = static_cast<int16_t>((i * 65535 / 256) - 32767);  // Overflows!

// Fixed:
saw_temp[i] = static_cast<int16_t>((i - 128) * 256);  // Proper range -32768 to 32512
```

### Step 2: Implement Basic PolyBLEP
```cpp
class PolyBlepOscillator {
    float phase = 0.0f;
    float phase_increment;
    
    float polyBlep(float t) {
        if (t < phase_increment) {
            t /= phase_increment;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - phase_increment) {
            t = (t - 1.0f) / phase_increment;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
    
    float generateSaw() {
        float value = 2.0f * phase - 1.0f;  // Naive saw
        value -= polyBlep(phase);           // Apply correction
        value += polyBlep(fmod(phase + 1.0f, 1.0f));
        
        phase += phase_increment;
        if (phase >= 1.0f) phase -= 1.0f;
        
        return value;
    }
};
```

### Step 3: Replace RenderCSaw Implementation

Instead of using lookup tables, implement proper bandlimited synthesis:

```cpp
void MacroOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Use actual sample rate from plugin
    float sample_rate = getSampleRate();  
    float frequency = ComputeFrequency(pitch_);
    float phase_increment = frequency / sample_rate;
    
    for (size_t i = 0; i < size; ++i) {
        // Check sync
        if (sync && sync[i]) {
            phase_ = 0;
        }
        
        // Generate bandlimited sawtooth
        float saw = 2.0f * phase_ - 1.0f;
        
        // Apply PolyBLEP at discontinuity
        float t = phase_;
        if (t < phase_increment) {
            t /= phase_increment;
            saw -= (t + t - t * t - 1.0f);
        }
        
        // Update phase
        phase_ += phase_increment;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
            // Apply PolyBLEP at wrap
            t = phase_ / phase_increment;
            saw += (t * t + t + t + 1.0f);
        }
        
        // Convert to int16
        buffer[i] = static_cast<int16_t>(saw * 32767.0f);
    }
}
```

## Testing Plan

1. **Implement basic PolyBLEP sawtooth**
2. **Test at different pitches** - Should sound clean from 20Hz to 20kHz
3. **Check aliasing** - No harsh digital artifacts
4. **Verify amplitude** - Consistent level across frequency range
5. **Test sync input** - Phase resets should work cleanly

## Success Criteria

- CSAW produces a clean, alias-free sawtooth wave
- No clicks or digital artifacts
- Consistent amplitude across frequency range
- Proper response to TIMBRE and COLOR parameters
- Sync input works correctly

## Next Steps After CSAW Works

Once CSAW is working properly, we can:
1. Apply the same PolyBLEP approach to other analog waveforms
2. Fix the digital oscillator algorithms
3. Ensure proper parameter mapping for all algorithms
4. Test the complete synthesizer