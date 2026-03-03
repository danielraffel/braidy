# Braidy Algorithm Analysis - Sound Parity Issues

## Critical Issues Found

### 1. **Most Digital Algorithms Are Placeholders**
- 32 out of 48 algorithms are using generic `RenderDigital` function
- These algorithms don't have proper DSP implementations
- They're all falling back to basic wavetable synthesis

### 2. **Missing Proper Parameter Mappings**
- TIMBRE and COLOR parameters are not properly mapped for each algorithm
- Original Braids has specific parameter behaviors for each algorithm
- Current implementation uses generic parameter handling

### 3. **Algorithm Comparison Table**

| Index | Original Name | Our Name | Implementation Status | TIMBRE Mapping | COLOR Mapping | Issues |
|-------|--------------|----------|----------------------|----------------|---------------|---------|
| 0 | CSAW | CSAW | ✅ Basic | Detune amount | Filter | Missing proper filter implementation |
| 1 | MORPH | MRPH | ✅ Basic | Morph amount | Brightness | Missing wave morphing tables |
| 2 | SAW_SQUARE | S/SQ | ✅ Basic | Pulse width | Sync | Missing hard sync |
| 3 | SINE_TRIANGLE | S/TR | ✅ Basic | Wave shape | Wavefold | Missing wavefolder |
| 4 | BUZZ | BUZZ | ✅ Basic | Harmonics | Filter | Missing harmonic control |
| 5 | SQUARE_SUB | +SUB | ✅ Basic | Sub level | Pulse width | Missing sub oscillator |
| 6 | SAW_SUB | SAW+ | ✅ Basic | Sub level | Detune | Missing sub oscillator |
| 7 | SQUARE_SYNC | +SYN | ✅ Basic | Sync amount | Pulse width | Missing proper sync |
| 8 | SAW_SYNC | SAW* | ✅ Basic | Sync amount | Detune | Missing proper sync |
| 9 | TRIPLE_SAW | TRI3 | ✅ Basic | Detune | Spread | Missing triple voice |
| 10 | TRIPLE_SQUARE | SQ3 | ✅ Basic | Detune | Spread | Missing triple voice |
| 11 | TRIPLE_TRIANGLE | TR3 | ✅ Basic | Detune | Spread | Missing triple voice |
| 12 | TRIPLE_SINE | SI3 | ✅ Basic | Detune | Spread | Missing triple voice |
| 13 | TRIPLE_RING_MOD | RI3 | ❌ Placeholder | Ring mod freq | Ring mod amount | No implementation |
| 14 | SAW_SWARM | SWRM | ❌ Placeholder | Swarm density | Detune spread | No implementation |
| 15 | SAW_COMB | COMB | ❌ Placeholder | Delay time | Feedback | No implementation |
| 16 | TOY | TOY | ❌ Placeholder | Sample rate | Bit depth | No implementation |
| 17 | DIGITAL_FILTER_LP | FLTR | ❌ Placeholder | Cutoff | Resonance | No implementation |
| 18 | DIGITAL_FILTER_PK | PEAK | ❌ Placeholder | Frequency | Resonance | No implementation |
| 19 | DIGITAL_FILTER_BP | BAND | ❌ Placeholder | Frequency | Bandwidth | No implementation |
| 20 | DIGITAL_FILTER_HP | HIGH | ❌ Placeholder | Cutoff | Resonance | No implementation |
| 21 | VOSIM | VOSM | ❌ Placeholder | Formant | Carrier shape | No implementation |
| 22 | VOWEL | VOWL | ❌ Placeholder | Vowel | Formant shift | No implementation |
| 23 | VOWEL_FOF | VOW2 | ❌ Placeholder | Vowel morph | Formant freq | No implementation |
| 24 | HARMONICS | HARM | ❌ Placeholder | Harmonic content | Harmonic balance | No implementation |
| 25 | FM | FM | ❌ Basic FM | FM ratio | FM amount | Missing proper ratios |
| 26 | FEEDBACK_FM | FBFM | ❌ Placeholder | Feedback amount | FM ratio | No implementation |
| 27 | CHAOTIC_FEEDBACK_FM | WTFM | ❌ Placeholder | Chaos amount | FM ratio | No implementation |
| 28 | PLUCKED | PLUK | ❌ Placeholder | Damping | Brightness | No Karplus-Strong |
| 29 | BOWED | BOWD | ❌ Placeholder | Bow pressure | Bow position | No physical model |
| 30 | BLOWN | BLOW | ❌ Placeholder | Breath pressure | Embouchure | No physical model |
| 31 | FLUTED | FLUT | ❌ Placeholder | Breath | Tone | No physical model |
| 32 | STRUCK_BELL | BELL | ❌ Placeholder | Mallet hardness | Inharmonicity | No modal synthesis |
| 33 | STRUCK_DRUM | DRUM | ❌ Placeholder | Decay | Tone | No drum synthesis |
| 34 | KICK | KICK | ❌ Placeholder | Punch | Decay | No kick synthesis |
| 35 | CYMBAL | CYMB | ❌ Placeholder | Decay | Tone | No metallic noise |
| 36 | SNARE | SNAR | ❌ Placeholder | Snappy | Tone | No snare synthesis |
| 37 | WAVETABLES | WTBL | ❌ Placeholder | Wave position | Wave bank | No wavetables |
| 38 | WAVE_MAP | WMAP | ❌ Placeholder | X position | Y position | No 2D wavetable |
| 39 | WAVE_LINE | WLIN | ❌ Placeholder | Wave position | Interpolation | No wavetable line |
| 40 | WAVE_PARAPHONIC | WPAR | ❌ Placeholder | Chord type | Inversion | No paraphonic |
| 41 | FILTERED_NOISE | NOIS | ❌ Placeholder | Filter cutoff | Filter type | No filtered noise |
| 42 | TWIN_PEAKS_NOISE | TWLN | ❌ Placeholder | Peak 1 freq | Peak 2 freq | No twin peaks |
| 43 | CLOCKED_NOISE | CLKN | ❌ Placeholder | Clock rate | Randomness | No clocked noise |
| 44 | GRANULAR_CLOUD | CLDS | ❌ Placeholder | Grain density | Grain size | No granular |
| 45 | PARTICLE_NOISE | PART | ❌ Placeholder | Particle density | Decay | No particle synthesis |
| 46 | DIGITAL_MODULATION | DIGI | ❌ Placeholder | Bit crushing | Sample rate | No digital effects |
| 47 | QUESTION_MARK | QPSK | ❌ Placeholder | Random param 1 | Random param 2 | No implementation |

## Key Problems to Fix

### Phase 1: Core Analog Algorithms (0-14)
1. **CSAW**: Missing proper filter, detune needs calibration
2. **MORPH**: Missing wavetable morphing implementation
3. **SAW_SQUARE**: Missing hard sync implementation
4. **SINE_TRIANGLE**: Missing wavefolder
5. **BUZZ**: Missing harmonic series generator
6. **SUB oscillators**: Missing sub oscillator implementation
7. **SYNC algorithms**: Missing proper hard sync
8. **TRIPLE voices**: Missing unison/spread implementation
9. **RING_MOD**: Completely missing
10. **SAW_SWARM**: Completely missing

### Phase 2: Digital Filters & Formants (15-24)
- All completely missing proper implementations
- Need SVF filters, formant filters, VOSIM, FOF synthesis

### Phase 3: FM & Physical Models (25-36)
- FM exists but needs proper ratio tables
- All physical models missing (Karplus-Strong, waveguide models)
- Percussion synthesis completely missing

### Phase 4: Wavetables & Noise (37-47)
- No wavetable support at all
- No granular synthesis
- No noise generators beyond white noise

## Action Plan

1. **Import Original DSP Code**: Copy the actual DSP implementations from `/eurorack/braids/`
2. **Fix Parameter Mappings**: Each algorithm needs specific TIMBRE/COLOR behavior
3. **Implement Missing Algorithms**: 32+ algorithms need complete reimplementation
4. **Calibrate Parameters**: Match exact parameter ranges and curves from hardware
5. **Add Wavetable Support**: Load and interpolate wavetables
6. **Physical Modeling**: Implement Karplus-Strong and waveguide models
7. **Granular Synthesis**: Implement grain clouds and particles

## Files to Import from Original Braids

- `/eurorack/braids/digital_oscillator.cc` - All digital algorithms
- `/eurorack/braids/analog_oscillator.cc` - Analog wave generators  
- `/eurorack/braids/resources.cc` - Wavetables and lookup tables
- `/eurorack/braids/signature_waveshaper.cc` - Waveshaping algorithms
- `/eurorack/braids/quantizer.cc` - Scale quantization
- `/eurorack/braids/vco_jitter_source.cc` - Analog drift simulation

## Parameter Range Issues

The original Braids uses specific parameter ranges and curves:
- TIMBRE: -32768 to 32767 (signed 16-bit)
- COLOR: -32768 to 32767 (signed 16-bit)
- Many algorithms use non-linear parameter curves
- Some parameters are quantized to specific values

## Next Steps

1. Start by importing the complete original `digital_oscillator.cc`
2. Map all the DSP functions to our framework
3. Test each algorithm individually against hardware recordings
4. Fine-tune parameter ranges and curves