# Braidy Parameter Mappings - Complete Algorithm Reference

## Critical: TIMBRE and COLOR Parameter Mappings

Each algorithm in Braids has specific TIMBRE and COLOR parameter behaviors that must be exactly replicated for authentic sound.

## ANALOG WAVEFORMS (0-14)

### 0. CSAW (Bandlimited sawtooth with phase randomizer)
- **TIMBRE**: Detune amount (0 = no detune, 32767 = maximum detune)
- **COLOR**: Low-pass filter cutoff frequency
- **Implementation**: Phase randomization on each cycle for analog warmth

### 1. MRPH (Morphing waveform)
- **TIMBRE**: Morph position (0 = triangle, 16384 = saw, 32767 = square)
- **COLOR**: Symmetry/pulse width when approaching square
- **Implementation**: Smooth morphing between waveshapes

### 2. S/SQ (Sawtooth to square with hard sync)
- **TIMBRE**: Pulse width (0 = narrow, 32767 = 50% square)
- **COLOR**: Hard sync frequency ratio
- **Implementation**: Variable pulse width with optional hard sync

### 3. S/TR (Sine to triangle with wavefolder)
- **TIMBRE**: Wave shape morph (0 = sine, 32767 = triangle)
- **COLOR**: Wavefold amount
- **Implementation**: Sine-triangle morph with analog-style wavefolding

### 4. BUZZ (Comb-filtered sawtooth)
- **TIMBRE**: Number of harmonics (0 = 1 harmonic, 32767 = all harmonics)
- **COLOR**: Low-pass filter cutoff
- **Implementation**: Additive synthesis of harmonics

### 5. +SUB (Square with sub-oscillator)
- **TIMBRE**: Sub-oscillator level (0 = no sub, 32767 = full sub)
- **COLOR**: Pulse width of main oscillator
- **Implementation**: Main square + -1 octave square

### 6. SAW+ (Sawtooth with sub-oscillator)
- **TIMBRE**: Sub-oscillator level
- **COLOR**: Detune between main and sub
- **Implementation**: Main saw + -1 octave saw with detune

### 7. +SYN (Square with sync oscillator)
- **TIMBRE**: Sync frequency ratio (0 = 1:1, 32767 = 8:1)
- **COLOR**: Pulse width
- **Implementation**: Hard sync with variable ratio

### 8. SAW* (Sawtooth with sync)
- **TIMBRE**: Sync frequency ratio
- **COLOR**: Detune amount
- **Implementation**: Hard-synced sawtooth

### 9. TRI3 (Triple sawtooth)
- **TIMBRE**: Detune spread (0 = unison, 32767 = wide)
- **COLOR**: Voice spread pattern
- **Implementation**: 3 detuned saws

### 10. SQ3 (Triple square)
- **TIMBRE**: Detune spread
- **COLOR**: Pulse width (all voices)
- **Implementation**: 3 detuned squares

### 11. TR3 (Triple triangle)
- **TIMBRE**: Detune spread
- **COLOR**: Voice spread pattern
- **Implementation**: 3 detuned triangles

### 12. SI3 (Triple sine)
- **TIMBRE**: Detune spread
- **COLOR**: Phase offset between voices
- **Implementation**: 3 detuned sines

### 13. RI3 (Triple ring modulation)
- **TIMBRE**: Ring modulator 1 frequency offset
- **COLOR**: Ring modulator 2 frequency offset
- **Implementation**: Carrier × Mod1 × Mod2

### 14. SWRM (Saw swarm)
- **TIMBRE**: Swarm density (3-7 voices)
- **COLOR**: Detune spread amount
- **Implementation**: Multiple detuned saws

## DIGITAL SYNTHESIS (15-24)

### 15. COMB (Comb filter)
- **TIMBRE**: Delay time / comb frequency
- **COLOR**: Feedback amount
- **Implementation**: Comb filter on saw wave

### 16. TOY* (Circuit bent)
- **TIMBRE**: Sample rate reduction (48kHz to 1.5kHz)
- **COLOR**: Bit depth reduction (16 to 2 bits)
- **Implementation**: Lo-fi digital degradation

### 17. FLTR (Digital filter LP)
- **TIMBRE**: Filter cutoff frequency
- **COLOR**: Filter resonance
- **Implementation**: State-variable filter on noise

### 18. PEAK (Digital filter peak)
- **TIMBRE**: Peak frequency
- **COLOR**: Peak resonance/Q
- **Implementation**: Bandpass filter on noise

### 19. BAND (Digital filter BP)
- **TIMBRE**: Center frequency
- **COLOR**: Bandwidth/Q
- **Implementation**: Bandpass filter

### 20. HIGH (Digital filter HP)
- **TIMBRE**: Highpass cutoff
- **COLOR**: Resonance
- **Implementation**: Highpass filter on noise

### 21. VOSM (VOSIM synthesis)
- **TIMBRE**: Formant 1 frequency
- **COLOR**: Formant 2 frequency
- **Implementation**: Voice simulation technique

### 22. VOWL (Vowel synthesis)
- **TIMBRE**: Vowel selection (a, e, i, o, u)
- **COLOR**: Formant shift / brightness
- **Implementation**: Formant filter synthesis

### 23. VOW2 (FOF vowel synthesis)
- **TIMBRE**: Vowel morph position
- **COLOR**: Formant frequency scaling
- **Implementation**: Fonction d'Onde Formantique

### 24. HARM (Additive harmonics)
- **TIMBRE**: Harmonic content (1-16 harmonics)
- **COLOR**: Harmonic balance/brightness
- **Implementation**: Additive synthesis

## FM SYNTHESIS (25-27)

### 25. FM (2-operator FM)
- **TIMBRE**: FM ratio (quantized to musical ratios)
- **COLOR**: FM amount/index
- **Implementation**: Sine carrier × sine modulator

### 26. FBFM (Feedback FM)
- **TIMBRE**: Feedback amount
- **COLOR**: FM ratio
- **Implementation**: FM with feedback path

### 27. WTFM (Chaotic feedback FM)
- **TIMBRE**: Chaos amount
- **COLOR**: FM ratio
- **Implementation**: FM with chaotic feedback

## PHYSICAL MODELS (28-36)

### 28. PLUK (Karplus-Strong plucked string)
- **TIMBRE**: Damping/decay time
- **COLOR**: Brightness/tone
- **Implementation**: Karplus-Strong algorithm

### 29. BOWD (Bowed string)
- **TIMBRE**: Bow pressure
- **COLOR**: Bow position
- **Implementation**: Waveguide model

### 30. BLOW (Wind instrument)
- **TIMBRE**: Breath pressure
- **COLOR**: Embouchure/brightness
- **Implementation**: Waveguide with breath noise

### 31. FLUT (Flute)
- **TIMBRE**: Breath amount
- **COLOR**: Tone/brightness
- **Implementation**: Waveguide flute model

### 32. BELL (Struck bell)
- **TIMBRE**: Mallet hardness
- **COLOR**: Inharmonicity amount
- **Implementation**: Modal synthesis

### 33. DRUM (Struck drum)
- **TIMBRE**: Decay time
- **COLOR**: Tone/pitch
- **Implementation**: Modal drum synthesis

### 34. KICK (Kick drum)
- **TIMBRE**: Punch/pitch envelope
- **COLOR**: Decay time
- **Implementation**: Sine with pitch envelope

### 35. CYMB (Cymbal)
- **TIMBRE**: Decay time
- **COLOR**: Tone/brightness
- **Implementation**: Metallic noise synthesis

### 36. SNAR (Snare drum)
- **TIMBRE**: Snappiness (noise amount)
- **COLOR**: Tone frequency
- **Implementation**: Tonal + noise components

## WAVETABLES (37-40)

### 37. WTBL (Wavetable oscillator)
- **TIMBRE**: Wavetable position
- **COLOR**: Wavetable bank selection
- **Implementation**: 256-sample wavetables

### 38. WMAP (2D wavetable map)
- **TIMBRE**: X position in map
- **COLOR**: Y position in map
- **Implementation**: 2D wavetable morphing

### 39. WLIN (Wavetable line)
- **TIMBRE**: Position along line
- **COLOR**: Interpolation type
- **Implementation**: Linear wavetable scanning

### 40. WPAR (Wave paraphonic)
- **TIMBRE**: Chord type (maj, min, 7th, etc)
- **COLOR**: Inversion
- **Implementation**: Paraphonic wavetable

## NOISE & PARTICLES (41-46)

### 41. NOIS (Filtered noise)
- **TIMBRE**: Filter cutoff
- **COLOR**: Filter type (LP/BP/HP morph)
- **Implementation**: Filtered white noise

### 42. TWLN (Twin peaks noise)
- **TIMBRE**: Peak 1 frequency
- **COLOR**: Peak 2 frequency
- **Implementation**: Dual resonant filters

### 43. CLKN (Clocked noise)
- **TIMBRE**: Clock rate
- **COLOR**: Randomness amount
- **Implementation**: Sample & hold noise

### 44. CLDS (Granular cloud)
- **TIMBRE**: Grain density
- **COLOR**: Grain size
- **Implementation**: Granular synthesis

### 45. PART (Particle noise)
- **TIMBRE**: Particle density
- **COLOR**: Decay time
- **Implementation**: Particle synthesis

### 46. DIGI (Digital modulation)
- **TIMBRE**: Bit crushing amount
- **COLOR**: Sample rate reduction
- **Implementation**: Digital degradation

### 47. QPSK (Question mark / easter egg)
- **TIMBRE**: Random parameter 1
- **COLOR**: Random parameter 2
- **Implementation**: Experimental/random

## Implementation Notes

1. **Parameter Ranges**: All parameters use signed 16-bit values (-32768 to 32767)
2. **Interpolation**: Parameters should be smoothly interpolated to avoid clicks
3. **Modulation**: LFO and envelope modulation should add to base parameter values
4. **Calibration**: Each algorithm needs specific parameter curves for authentic response

## Critical Fixes Needed

1. **Wavetable Support**: Currently missing entirely - need to load wavetables
2. **Physical Models**: Most are placeholders - need proper waveguide implementations
3. **Parameter Curves**: Many parameters need non-linear response curves
4. **Formant Data**: Vowel algorithms need proper formant frequency tables
5. **FM Ratios**: Need quantized ratio tables for musical FM sounds

## Testing Checklist

For each algorithm, verify:
- [ ] TIMBRE parameter changes the correct aspect
- [ ] COLOR parameter changes the correct aspect
- [ ] Parameter ranges match hardware (full CCW to full CW)
- [ ] Sound character matches original Braids
- [ ] Modulation (LFO/ENV) works correctly
- [ ] No clicks or pops when changing parameters
- [ ] CPU usage is reasonable