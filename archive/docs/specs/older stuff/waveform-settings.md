# Braids Waveform Settings Reference
## Complete Algorithm Specifications from Original Source

This document provides the exact implementation details for all 48 Braids algorithms based on the original source code analysis.

## Algorithm Routing Table

| Index | Shape Enum | Display | Render Function | Implementation Details |
|-------|------------|---------|-----------------|------------------------|
| 0 | MACRO_OSC_SHAPE_CSAW | CSAW | RenderCSaw | Analog bandlimited saw with phase randomizer |
| 1 | MACRO_OSC_SHAPE_MORPH | MRPH | RenderMorph | Triangle→Saw→Square→PWM morph with LP filter |
| 2 | MACRO_OSC_SHAPE_SAW_SQUARE | S/SQ | RenderSawSquare | Variable saw mixed with square |
| 3 | MACRO_OSC_SHAPE_SINE_TRIANGLE | S/TR | RenderSineTriangle | Sine→Triangle morph with fold |
| 4 | MACRO_OSC_SHAPE_BUZZ | BUZZ | RenderBuzz | Comb-filtered saw with harmonics |
| 5 | MACRO_OSC_SHAPE_SQUARE_SUB | +SUB | RenderSub | Square + square sub (-1 oct) |
| 6 | MACRO_OSC_SHAPE_SAW_SUB | SAW+ | RenderSub | Saw + saw sub (-1 oct) |
| 7 | MACRO_OSC_SHAPE_SQUARE_SYNC | +SYN | RenderDualSync | Square with hard sync |
| 8 | MACRO_OSC_SHAPE_SAW_SYNC | SAW* | RenderDualSync | Saw with hard sync |
| 9 | MACRO_OSC_SHAPE_TRIPLE_SAW | TRI3 | RenderTriple | 3 detuned saws |
| 10 | MACRO_OSC_SHAPE_TRIPLE_SQUARE | SQ3 | RenderTriple | 3 detuned squares |
| 11 | MACRO_OSC_SHAPE_TRIPLE_TRIANGLE | TR3 | RenderTriple | 3 detuned triangles |
| 12 | MACRO_OSC_SHAPE_TRIPLE_SINE | SI3 | RenderTriple | 3 detuned sines |
| 13 | MACRO_OSC_SHAPE_TRIPLE_RING_MOD | RI3 | RenderDigital→TripleRingMod | Carrier × Mod1 × Mod2 |
| 14 | MACRO_OSC_SHAPE_SAW_SWARM | SWRM | RenderDigital→SawSwarm | 3-7 detuned saws |
| 15 | MACRO_OSC_SHAPE_SAW_COMB | COMB | RenderSawComb | Saw through comb filter |
| 16 | MACRO_OSC_SHAPE_TOY | TOY | RenderDigital→Toy | Bit crush + sample rate reduction |
| 17 | MACRO_OSC_SHAPE_DIGITAL_FILTER_LP | FLTR | RenderDigital→FilterLP | LP filtered noise |
| 18 | MACRO_OSC_SHAPE_DIGITAL_FILTER_PK | PEAK | RenderDigital→FilterPK | Peak filtered noise |
| 19 | MACRO_OSC_SHAPE_DIGITAL_FILTER_BP | BAND | RenderDigital→FilterBP | BP filtered noise |
| 20 | MACRO_OSC_SHAPE_DIGITAL_FILTER_HP | HIGH | RenderDigital→FilterHP | HP filtered noise |
| 21 | MACRO_OSC_SHAPE_VOSIM | VOSM | RenderDigital→Vosim | Voice simulation |
| 22 | MACRO_OSC_SHAPE_VOWEL | VOWL | RenderDigital→Vowel | Formant filter vowels |
| 23 | MACRO_OSC_SHAPE_VOWEL_FOF | VOW2 | RenderDigital→VowelFOF | FOF synthesis |
| 24 | MACRO_OSC_SHAPE_HARMONICS | HARM | RenderDigital→Harmonics | Additive harmonics |
| 25 | MACRO_OSC_SHAPE_FM | FM | RenderDigital→FM | 2-op FM |
| 26 | MACRO_OSC_SHAPE_FEEDBACK_FM | FBFM | RenderDigital→FeedbackFM | FM with feedback |
| 27 | MACRO_OSC_SHAPE_CHAOTIC_FEEDBACK_FM | WTFM | RenderDigital→ChaoticFM | Chaotic FM |
| 28 | MACRO_OSC_SHAPE_PLUCKED | PLUK | RenderDigital→Plucked | Karplus-Strong |
| 29 | MACRO_OSC_SHAPE_BOWED | BOWD | RenderDigital→Bowed | Bowed string model |
| 30 | MACRO_OSC_SHAPE_BLOWN | BLOW | RenderDigital→Blown | Wind instrument |
| 31 | MACRO_OSC_SHAPE_FLUTED | FLUT | RenderDigital→Fluted | Flute model |
| 32 | MACRO_OSC_SHAPE_STRUCK_BELL | BELL | RenderDigital→StruckBell | Modal bell synthesis |
| 33 | MACRO_OSC_SHAPE_STRUCK_DRUM | DRUM | RenderDigital→StruckDrum | Modal drum |
| 34 | MACRO_OSC_SHAPE_KICK | KICK | RenderDigital→Kick | Kick drum |
| 35 | MACRO_OSC_SHAPE_CYMBAL | CYMB | RenderDigital→Cymbal | Metallic noise |
| 36 | MACRO_OSC_SHAPE_SNARE | SNAR | RenderDigital→Snare | Snare drum |
| 37 | MACRO_OSC_SHAPE_WAVETABLES | WTBL | RenderDigital→Wavetables | Wavetable oscillator |
| 38 | MACRO_OSC_SHAPE_WAVE_MAP | WMAP | RenderDigital→WaveMap | 2D wavetable map |
| 39 | MACRO_OSC_SHAPE_WAVE_LINE | WLIN | RenderDigital→WaveLine | Linear wavetable scan |
| 40 | MACRO_OSC_SHAPE_WAVE_PARAPHONIC | WPAR | RenderDigital→WaveParaphonic | Paraphonic chords |
| 41 | MACRO_OSC_SHAPE_FILTERED_NOISE | NOIS | RenderDigital→FilteredNoise | Filtered white noise |
| 42 | MACRO_OSC_SHAPE_TWIN_PEAKS_NOISE | TWLN | RenderDigital→TwinPeaks | Dual peak filters |
| 43 | MACRO_OSC_SHAPE_CLOCKED_NOISE | CLKN | RenderDigital→ClockedNoise | S&H noise |
| 44 | MACRO_OSC_SHAPE_GRANULAR_CLOUD | CLDS | RenderDigital→GranularCloud | Granular synthesis |
| 45 | MACRO_OSC_SHAPE_PARTICLE_NOISE | PART | RenderDigital→ParticleNoise | Particle synthesis |
| 46 | MACRO_OSC_SHAPE_DIGITAL_MODULATION | DIGI | RenderDigital→DigitalMod | Bit operations |
| 47 | MACRO_OSC_SHAPE_QUESTION_MARK | QPSK | RenderDigital→QuestionMark | Easter egg/random |

## Detailed Algorithm Specifications

### ANALOG ALGORITHMS (0-12)

#### 0. CSAW - Bandlimited Sawtooth with Phase Randomizer
```cpp
Shape: OSC_SHAPE_CSAW
Parameter[0] (TIMBRE): Phase randomization amount (0-32767)
Parameter[1] (COLOR): Low-pass filter cutoff (-32767 to 32767)
Processing:
  - Uses analog_oscillator with CSAW shape
  - Adds DC offset based on COLOR
  - Scales output by 13/8 for gain compensation
```

#### 1. MRPH - Morphing Waveform
```cpp
Morphing stages based on TIMBRE (parameter[0]):
  0-10922: Triangle → Saw (balance = param * 6)
  10923-21845: Square → Saw (balance = 65535 - (param - 10923) * 6)
  21846-32767: Square with PWM → Sine (PWM = (param - 21846) * 3)
Parameter[1] (COLOR): LP filter cutoff + fuzz amount
Processing:
  - Two oscillators with crossfade
  - SVF lowpass filter
  - Violent overdrive waveshaping when COLOR high
```

#### 2. S/SQ - Saw/Square Mix
```cpp
Parameter[0] (TIMBRE): Pulse width for both oscillators
Parameter[1] (COLOR): Mix balance (interpolated)
  - 0 = pure saw
  - 32767 = pure square (attenuated to 148/256)
Oscillators:
  - OSC1: VARIABLE_SAW
  - OSC2: SQUARE
```

#### 3. S/TR - Sine/Triangle with Fold
```cpp
Parameter[0] (TIMBRE): Shape morph
  0-10922: Triangle fold amount
  10923-21845: Triangle → Square morph
  21846-32767: Sine → Triangle morph
Parameter[1] (COLOR): Wavefold amount
Processing:
  - Single oscillator with shape morphing
  - Tri-fold waveshaping applied
```

#### 4. BUZZ - Comb Filtered Sawtooth
```cpp
Parameter[0] (TIMBRE): Comb frequency/harmonics
Parameter[1] (COLOR): Filter amount
Processing:
  - Sawtooth through resonant comb filter
  - Creates harmonic emphasis effects
```

#### 5-6. SUB Oscillators (+SUB, SAW+)
```cpp
Shape determination:
  - SQUARE_SUB: Main = square, Sub = square
  - SAW_SUB: Main = saw, Sub = saw
Parameter[0] (TIMBRE): Sub level (0 = none, 32767 = equal)
Parameter[1] (COLOR): 
  - For square: Pulse width
  - For saw: Detune amount
Sub oscillator: -1 octave (pitch - 12 * 128)
```

#### 7-8. SYNC Oscillators (+SYN, SAW*)
```cpp
Shape determination:
  - SQUARE_SYNC: Master = square
  - SAW_SYNC: Master = saw
Parameter[0] (TIMBRE): Sync ratio (affects slave pitch)
Parameter[1] (COLOR): Balance between master/slave
Sync implementation:
  - Master oscillator provides sync signal
  - Slave oscillator pitch = master_pitch + sync_amount
```

#### 9-12. TRIPLE Oscillators (TRI3, SQ3, TR3, SI3)
```cpp
Shapes: SAW, SQUARE, TRIANGLE, or SINE (based on algorithm)
Parameter[0] (TIMBRE): Detune spread
  - Maps to interval table (65 entries from -24 to +24 semitones)
Parameter[1] (COLOR): Voice spread pattern
Voice configuration:
  - Voice 1: Base pitch
  - Voice 2: Pitch + detune_interval
  - Voice 3: Pitch - detune_interval
Mixing: Equal amplitude (10922 each ≈ 1/3)
```

### DIGITAL ALGORITHMS (13-47)

#### 13. RI3 - Triple Ring Modulation
```cpp
Parameter[0]: Modulator 1 frequency offset (-16384 to +16384, scaled by 1/4)
Parameter[1]: Modulator 2 frequency offset (-16384 to +16384, scaled by 1/4)
Algorithm:
  carrier = sine(base_pitch + 0x40000000)  // +90° phase
  mod1 = sine(base_pitch + param0_offset)
  mod2 = sine(base_pitch + param1_offset)
  output = carrier * mod1 * mod2
```

#### 14. SWRM - Saw Swarm
```cpp
Parameter[0]: Swarm density (3-7 voices based on value)
Parameter[1]: Detune spread (0-5% maximum)
Voice allocation:
  voices = 3 + (param0 >> 13)  // 3-7 voices
  detune_factor[i] = 1.0 + detune * (i - voices/2) / voices
```

#### 15. COMB - Comb Filter
```cpp
Parameter[0]: Delay time (4-1024 samples)
Parameter[1]: Feedback amount (-32767 to 32767)
Input: Sawtooth wave
Processing: y[n] = input[n] + feedback * y[n-delay]
```

#### 16. TOY - Circuit Bent
```cpp
Parameter[0]: Sample rate reduction (1-32x divider)
Parameter[1]: Bit depth (1-8 bit reduction from 16-bit)
Processing:
  - Sample & hold for rate reduction
  - Bit masking for bit crush
  - Input: Square wave
```

#### 17-20. Digital Filters (FLTR, PEAK, BAND, HIGH)
```cpp
Common: White noise input
Parameter[0]: Filter frequency
Parameter[1]: Resonance/Q
Filter types:
  - LP: 2-pole lowpass
  - PK: Bandpass peak filter
  - BP: 2-pole bandpass
  - HP: 2-pole highpass
```

#### 21. VOSM - VOSIM
```cpp
Parameter[0]: Formant 1 frequency (scaled by 1/2)
Parameter[1]: Formant 2 frequency (scaled by 1/2)
Algorithm:
  - Two formant oscillators (sine)
  - Bell-shaped envelope per cycle
  - Formants reset at carrier period
```

#### 22-23. Vowel Synthesis (VOWL, VOW2)
```cpp
VOWL:
  Parameter[0]: Vowel selection (a,e,i,o,u) + morph
  Parameter[1]: Formant shift/brightness
  Uses formant filter bank

VOW2 (FOF):
  Parameter[0]: Vowel morph position
  Parameter[1]: Formant frequency scaling
  Grain-based formant synthesis
```

#### 24. HARM - Additive Harmonics
```cpp
Parameter[0]: Number of harmonics (1-16)
Parameter[1]: Harmonic brightness/rolloff
Algorithm:
  for h = 1 to num_harmonics:
    output += sin(pitch * h) / h
```

#### 25-27. FM Synthesis
```cpp
FM:
  Parameter[0]: Ratio (quantized to musical ratios)
  Parameter[1]: FM index/amount
  carrier = sin(carrier_phase + modulator * index)

FBFM:
  Parameter[0]: Feedback amount
  Parameter[1]: FM ratio
  Adds feedback path from output to modulator

WTFM (Chaotic):
  Parameter[0]: Chaos amount
  Parameter[1]: FM ratio
  Non-linear feedback creates chaotic behavior
```

#### 28-36. Physical Models
```cpp
PLUK (Karplus-Strong):
  Parameter[0]: Damping (decay time)
  Parameter[1]: Brightness (LP filter)
  Excitation: Noise burst
  Delay line with filtered feedback

BOWD:
  Parameter[0]: Bow pressure
  Parameter[1]: Bow position
  Non-linear waveguide model

BLOW/FLUT:
  Parameter[0]: Breath pressure
  Parameter[1]: Embouchure/tone
  Waveguide with turbulence noise

BELL:
  Parameter[0]: Mallet hardness
  Parameter[1]: Inharmonicity
  Modal synthesis with inharmonic partials:
  [1.0, 2.76, 5.40, 8.93, 13.34]

DRUM:
  Parameter[0]: Decay time
  Parameter[1]: Tone/pitch
  Modal synthesis with membrane modes

KICK:
  Parameter[0]: Punch (pitch envelope)
  Parameter[1]: Decay time
  Sine with exponential pitch drop

CYMB:
  Parameter[0]: Decay time
  Parameter[1]: Tone/brightness
  Metallic noise synthesis

SNAR:
  Parameter[0]: Snappiness (noise level)
  Parameter[1]: Tone frequency
  Mixed tonal (2 detuned sines) + noise
```

#### 37-40. Wavetables
```cpp
WTBL:
  Parameter[0]: Wavetable position (0-255)
  Parameter[1]: Wavetable bank (0-15)
  256-sample wavetables with linear interpolation

WMAP:
  Parameter[0]: X position in 2D map
  Parameter[1]: Y position in 2D map
  Bilinear interpolation across 4 wavetables

WLIN:
  Parameter[0]: Position along wavetable line
  Parameter[1]: Interpolation type (linear/smooth)
  Scans through wavetable sequence

WPAR:
  Parameter[0]: Chord type (maj/min/7th/etc)
  Parameter[1]: Inversion (root/1st/2nd/3rd)
  Paraphonic with multiple wavetable voices
```

#### 41-46. Noise Generators
```cpp
NOIS:
  Parameter[0]: Filter cutoff
  Parameter[1]: Filter type morph (LP→BP→HP)
  
TWLN:
  Parameter[0]: Peak 1 frequency
  Parameter[1]: Peak 2 frequency
  Two resonant bandpass filters

CLKN:
  Parameter[0]: Clock rate
  Parameter[1]: Randomness (0 = periodic, 32767 = random)
  Sample & hold with variable randomness

CLDS:
  Parameter[0]: Grain density
  Parameter[1]: Grain size
  Granular cloud synthesis

PART:
  Parameter[0]: Particle density
  Parameter[1]: Decay time
  Stochastic particle synthesis

DIGI:
  Parameter[0]: Bit operations type
  Parameter[1]: Operation amount
  Digital modulation/glitch effects
```

#### 47. QPSK - Question Mark
```cpp
Parameter[0]: Random parameter 1
Parameter[1]: Random parameter 2
Easter egg - implementation varies
Often includes data/modem sounds
```

## Critical Implementation Notes

### Parameter Scaling
- All parameters are int16_t (-32768 to 32767)
- Many use specific scaling factors (documented above)
- Parameter interpolation uses 24-sample blocks

### Phase Accumulator
- 32-bit phase accumulator (0 to 0xFFFFFFFF)
- Phase increment calculation uses pitch lookup tables
- Sample rate assumed 48kHz (96kHz for some algorithms)

### Mixing and Gain
- Multiple voices use equal power mixing
- Many algorithms include gain compensation
- Soft limiting applied to prevent clipping

### Lookup Tables Required
- lut_oscillator_increments[] - Pitch to phase increment
- lut_svf_cutoff[] - Filter cutoff curves
- ws_violent_overdrive[] - Waveshaping table
- lut_bell[] - Bell envelope shape
- wav_sine[] - Sine wavetable
- lut_fm_frequency_quantizer[] - FM ratio quantization

### Sync and Strike
- Sync input triggers phase reset
- Strike triggers envelope reset for percussive sounds
- Some algorithms ignore sync (physical models)

## Verification Checklist

For each algorithm, verify:
- [ ] Correct render function is called
- [ ] Parameter mappings match specification
- [ ] Parameter scaling is correct
- [ ] Output amplitude is normalized
- [ ] No aliasing at high frequencies
- [ ] Sync behavior matches hardware
- [ ] Strike behavior (if applicable)
- [ ] CPU usage is acceptable