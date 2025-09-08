# Braidy Audio Fix - Technical Approach Document
## JUCE Synthesizer Inspired by Braids - Supporting Modern DAW Requirements

## The Problem

We have a JUCE-based synthesizer inspired by Mutable Instruments Braids that produces **unusable audio** - either silence or terrible noise. Despite implementing all 48 synthesis algorithms, the sound is completely wrong.

### Why It's Broken

We missed critical DSP details from Braids while trying to adapt it to JUCE:

1. **Sample Rate Mismatch**
   - **Braids**: Runs internally at **96kHz** with decimation to 48kHz output
   - **Our Code**: Mixed rates (44.1kHz, 48kHz) without consistent processing
   - **Impact**: All time-based calculations (filters, delays, oscillators) are wrong

2. **Block Processing Architecture** 
   - **Braids**: Processes exactly **24 samples** at a time, always
   - **Our Code**: Variable buffer sizes from JUCE (could be 64, 128, 256, etc.)
   - **Impact**: Parameter interpolation broken, sync timing wrong

3. **Integer vs Float Math**
   - **Braids**: Uses **32-bit integer phase accumulators** with exact bit math
   - **Our Code**: Float calculations everywhere, losing precision
   - **Impact**: Phase drift, pitch instability, wrong frequencies

4. **Lookup Tables vs Runtime Calculation**
   - **Braids**: Pre-computed lookup tables for pitch→frequency, filter coefficients, etc.
   - **Our Code**: Calculates everything at runtime with pow(), exp(), etc.
   - **Impact**: Wrong scaling, CPU waste, imprecise values

5. **Per-Sample Sync**
   - **Braids**: Checks sync input **every sample** in tight loops
   - **Our Code**: Doesn't handle per-sample sync properly
   - **Impact**: Oscillator sync doesn't work, broken hard sync algorithms

## The Solution - JUCE-Compatible Implementation Plan

### Design Goals
- **Support standard DAW sample rates**: 44.1kHz, 48kHz, 88.2kHz, 96kHz, 192kHz
- **Work with any buffer size**: 64, 128, 256, 512, 1024, etc.
- **Maintain Braids sound quality**: Use the core DSP algorithms correctly
- **Be a proper JUCE plugin**: Work in any DAW on Windows/Mac/Linux

### Phase 1: Import Core DSP Components
**Goal**: Get the critical math and tables from Braids, adapt for variable sample rates

1. **Import Lookup Tables** ✅ Priority 1
   - Extract from `eurorack/braids/resources.cc`:
     - `lut_oscillator_increments[]` - pitch to phase increment 
     - `wav_sine[]` - sine wavetable
     - `lut_svf_cutoff[]` - filter cutoff curves
     - All other LUTs
   - Create `BraidyCore/BraidsLookupTables.h`
   - **KEY**: Scale these for different sample rates!

2. **Fix Phase Accumulator Math** ✅ Priority 1
   - Use integer phase accumulator (uint32_t) for precision
   - Adapt `ComputePhaseIncrement()` for current sample rate:
   ```cpp
   uint32_t ComputePhaseIncrement(int16_t midi_pitch, double sampleRate) {
     // Get base increment from LUT (designed for 96kHz)
     uint32_t base_increment = lut_oscillator_increments[pitch_index];
     
     // Scale for actual sample rate
     // If running at 48kHz, increment needs to be 2x larger
     // If running at 44.1kHz, increment needs to be ~2.18x larger
     double scale = 96000.0 / sampleRate;
     return static_cast<uint32_t>(base_increment * scale);
   }
   ```

### Phase 2: Adapt Processing for JUCE
**Goal**: Work correctly at any sample rate and buffer size

3. **Parameter Interpolation Every 24 Samples**
   - Keep the 24-sample parameter update rate (for smooth modulation)
   - But process whatever buffer size JUCE gives us:
   ```cpp
   void processBlock(AudioBuffer<float>& buffer, int numSamples) {
     for (int i = 0; i < numSamples; ++i) {
       // Update parameters every 24 samples
       if (++parameterCounter >= 24) {
         updateParameters();
         parameterCounter = 0;
       }
       
       // Process sample
       renderSample();
     }
   }
   ```

4. **Sample Rate Adaptation**
   - Store current sample rate from `prepareToPlay()`
   - Scale all time-based calculations:
     - Phase increments (for oscillators)
     - Filter coefficients  
     - Delay line lengths
     - Envelope rates
   - Example:
   ```cpp
   void prepareToPlay(double sampleRate, int samplesPerBlock) {
     currentSampleRate = sampleRate;
     sampleRateScale = 96000.0 / sampleRate; // Braids designed for 96kHz
     
     // Adjust all timing
     updatePhaseIncrements();
     updateFilterCoefficients();
     updateDelayLineLengths();
   }
   ```

### Phase 3: Fix Each Algorithm Category
**Goal**: Correct implementation of each synthesis type

5. **Fix Analog Oscillators (CSAW, MORPH, etc.)**
   - Import bandlimited waveform generation
   - Fix parameter scaling (0-32767 not 0.0-1.0)
   - Implement proper BLEP antialiasing

6. **Fix Digital Oscillators (TRIPLE_RING_MOD, etc.)**
   - Use integer math for all calculations
   - Fix sync handling (per-sample)
   - Correct parameter mappings

7. **Fix Wavetables (WPAR, WLIN, WMAP)**
   - Import wavetable data correctly
   - Fix interpolation (use integer math)
   - Initialize WavetableManager properly

8. **Fix Physical Models (KICK, CYMBAL, SNARE)**
   - Implement Strike() trigger correctly
   - Use proper modal frequencies
   - Fix envelope generators

### Phase 4: Integration & Testing

9. **Parameter System**
   - Ensure all parameters use 0-32767 range internally
   - Fix parameter interpolation (every 24 samples)
   - Remove all float conversions

10. **Testing Protocol**
    - Test each algorithm against original Braids
    - Verify pitch tracking across 8 octaves
    - Check parameter response curves
    - Measure CPU usage

## Technical Details

### Critical Constants (adapted for JUCE)
```cpp
// From original Braids (for reference)
const size_t kParameterUpdateRate = 24;  // Update parameters every 24 samples
const uint32_t kBraidsRefSampleRate = 96000;  // Braids' reference rate
const int16_t kOctave = 12 << 7;        // Octave in pitch units  
const int16_t kHighestNote = 128 << 7;  // Highest MIDI note

// For JUCE adaptation
double currentSampleRate;  // Set from prepareToPlay()
double sampleRateScale;    // kBraidsRefSampleRate / currentSampleRate
```

### Phase Accumulator Format
- 32-bit unsigned integer
- Upper 24 bits = integer part (sample index)
- Lower 8 bits = fractional part (for interpolation)
- Wraps naturally at 0xFFFFFFFF

### Parameter Format
- All parameters: int16_t (signed 16-bit)
- Range: -32768 to 32767
- No floating point conversions!

## Why This Will Work

The original Braids code contains **excellent DSP algorithms** that we need to use correctly. By:

1. **Using Braids' lookup tables** - We get the exact tuning and response curves
2. **Keeping integer phase math** - We maintain phase precision and stability
3. **Scaling for sample rate** - We adapt correctly to any DAW setting
4. **Per-sample processing** - We maintain sync accuracy
5. **24-sample parameter updates** - We get smooth modulation without clicks

This approach gives us:
- **DAW compatibility**: Works at 44.1/48/88.2/96kHz
- **Braids accuracy**: Sounds like the original
- **JUCE integration**: Works in any host
- **CPU efficiency**: Integer math where it matters

## Files to Modify

1. **Create New**:
   - `BraidyCore/BraidsLookupTables.h` - All LUTs from resources.cc
   - `BraidyCore/BraidsConstants.h` - Core constants

2. **Major Changes**:
   - `MacroOscillator.cpp` - Use LUTs, fix phase math, 24-sample processing
   - `DigitalOscillator.cpp` - Integer math, proper sync
   - `PluginProcessor.cpp` - 96kHz internal processing, decimation
   - `BraidyVoice.cpp` - 24-sample render blocks

3. **Remove/Replace**:
   - All `std::pow()`, `std::exp()` calculations
   - All float frequency variables
   - All runtime parameter scaling

## Success Criteria

✅ All 48 algorithms produce correct characteristic sound
✅ Pitch tracking works across full MIDI range
✅ Parameters respond correctly (TIMBRE, COLOR)
✅ No clicks, pops, or digital artifacts
✅ CPU usage reasonable (<10% for single voice)
✅ Sounds identical to hardware Braids

## Timeline Estimate

- Phase 1 (Import LUTs): 2 hours
- Phase 2 (Fix Processing): 4 hours  
- Phase 3 (Fix Algorithms): 6 hours
- Phase 4 (Testing): 2 hours

**Total: ~14 hours of focused work**

## The Core Insight

**We're building a proper JUCE synthesizer that uses Braids' excellent DSP algorithms.** This means:
- Adapting the algorithms to work at any sample rate (not just 96kHz)
- Processing any buffer size (not just 24 samples)
- Supporting modern DAW requirements
- While preserving the sound quality through proper DSP implementation

The key is understanding WHERE precision matters (phase accumulators, pitch tables) and WHERE we can adapt (sample rate scaling, buffer processing).