# Braidy User Manual
## Macro Oscillator Plugin

Version 1.0

---

## Overview

Braidy is a faithful software recreation of the Mutable Instruments Braids macro oscillator module. It provides 48 different synthesis algorithms ranging from classic analog-style waveforms to complex digital synthesis techniques, physical modeling, and experimental noise generators.

## Installation

### Audio Unit (AU) - for Logic Pro, GarageBand
- Build location: `~/Library/Audio/Plug-Ins/Components/braidy.component`

### VST3 - for Ableton Live, Reaper, etc.
- Build location: `~/Library/Audio/Plug-Ins/VST3/braidy.vst3`

### Standalone Application
- Run directly from the build folder or Applications folder

## Interface Overview

The Braidy interface faithfully recreates the original Braids hardware layout:

```
    [====== BRAIDS ======]
    [  macro oscillator  ]
    
    [    OLED DISPLAY    ]
    
    [   MAIN ENCODER    ]
    
    [FINE] [COARSE] [FM]
    
      ●        ●
    [TIMBRE] [COLOR]
      ◯        ◯
    
    [CV JACKS ROW]
```

## Main Controls

### OLED Display
- **4-character display** showing the current synthesis algorithm or menu parameter
- Green text on black background mimics the original hardware

### Main EDIT Encoder
- **Rotate**: Select synthesis algorithm or navigate menus
- **Click**: Enter/exit edit mode for menu parameters
- **Long Press** (>500ms): Enter/exit menu system

### Pitch Controls
- **FINE**: ±2 semitones fine tuning
- **COARSE**: ±5 octaves coarse tuning  
- **FM**: Frequency modulation attenuverter (-100% to +100%)

### Timbre Controls
- **TIMBRE**: Main synthesis parameter (algorithm-specific)
- **COLOR**: Secondary synthesis parameter (algorithm-specific)
- **Modulation knobs** (small): Attenuverters for CV modulation

### LED Indicators
- **Green LED** (above TIMBRE): Shows timbre parameter activity
- **Red LED** (above COLOR): Shows color parameter activity

## Synthesis Algorithms

Braidy provides 48 synthesis models organized into categories:

### Analog-Style Oscillators
1. **CSAW** - Saw wave with waveshaping
2. **MORPH** - Morphing between waveforms
3. **SAW^2** - Supersaw with detuning
4. **FOLD** - Wavefolding oscillator
5. **BUZZ** - Buzz oscillator
6. **SQR^2** - Dual square with hard sync
7. **RING** - Ring modulation
8. **SYN^2** - Dual oscillator sync
9. **SYN^4** - Quad oscillator sync

### Wavetable & Digital
10. **WAV^2** - Dual wavetable
11. **WAV^4** - Quad wavetable  
12. **WMAP** - Wavetable mapper
13. **WLIN** - Linear wavetable scanning
14. **WTX^4** - 4-voice wavetable
15. **NOIS** - Filtered noise
16. **TWNQ** - Twin peaks filter
17. **CLKN** - Clocked noise

### Formant & Vocal
18. **VOSIM** - Voice simulation
19. **VOWEL** - Vowel formants
20. **FOF** - Fonction d'onde formantique

### Granular & Additive
21. **HARM** - Additive harmonics
22. **CHMB** - Comb filter
23. **DIGI** - 8-bit digital
24. **CHIP** - Chiptune oscillator
25. **GRAIN** - Granular synthesis
26. **CLOUD** - Granular cloud
27. **PARTICLE** - Particle synthesis

### FM Synthesis
28. **FM** - 2-operator FM
29. **FBFM** - Feedback FM
30. **WTFM** - Wavetable FM
31. **PLUK** - Plucked string
32. **BOWD** - Bowed string
33. **BLOW** - Wind instrument
34. **FLUT** - Flute model
35. **BELL** - Bell synthesis
36. **DRUM** - Drum synthesis
37. **KICK** - Kick drum
38. **CYMB** - Cymbal
39. **SNAR** - Snare drum

### Experimental & Noise
40. **WTBL** - Wavetable bell
41. **MAZE** - Random maze
42. **RFRC** - Recursive fractal
43. **QPSK** - Phase shift keying
44. **ZLPF** - Zero-delay filter
45. **ZPDF** - Zero-phase distortion
46. **WTFX** - Wavetable effects
47. **CLOU** - Cloud reverb
48. **PRTC** - Particle system

## Menu System

Long-press the main encoder to access the menu system:

### Menu Pages
- **META** - Meta-oscillator settings
- **BITS** - Bit depth (1-16 bits)
- **RATE** - Sample rate reduction
- **TSRC** - Trigger source selection
- **TDLY** - Trigger delay (0-127)
- **ATK** - Attack time (0-127)
- **DEC** - Decay time (0-127)
- **FM** - FM envelope amount
- **TIM** - Timbre envelope amount
- **COL** - Color envelope amount
- **VCA** - VCA envelope amount
- **RANG** - Frequency range
- **OCTV** - Octave transpose (-4 to +4)
- **QNTZ** - Quantizer settings
- **ROOT** - Root note
- **FLAT** - Detuning amount
- **DRFT** - Oscillator drift
- **SIGN** - Signature (time signature)
- **CALI** - Calibration mode
- **VERS** - Firmware version

### Menu Navigation
1. **Long-press** encoder to enter menu
2. **Rotate** to navigate between pages
3. **Click** to edit parameter value
4. **Rotate** while editing to change value
5. **Click** again to confirm
6. **Long-press** to exit menu

## Parameter Mappings

### TIMBRE Parameter
Controls the primary characteristic of each algorithm:
- Analog oscillators: Waveshape, pulse width, or sync amount
- Wavetable: Wavetable position or morph amount
- FM: Modulation index or harmonicity
- Physical models: Excitation, damping, or material
- Granular: Grain size or density

### COLOR Parameter
Controls the secondary characteristic:
- Analog oscillators: Filter cutoff, detuning, or phase
- Wavetable: Interpolation, spread, or chorus
- FM: Feedback amount or ratio
- Physical models: Brightness, position, or decay
- Granular: Grain pitch or texture

## Tips & Tricks

### Creating Classic Sounds

**Analog Bass**
1. Select **CSAW** algorithm
2. Set COARSE to -12 (one octave down)
3. Adjust TIMBRE for waveshape
4. Use COLOR for filter sweep

**FM Bell**
1. Select **BELL** algorithm
2. TIMBRE controls harmonicity
3. COLOR adjusts decay time
4. Add slight FM for shimmer

**8-bit Lead**
1. Select **CHIP** or **DIGI**
2. Use BITS menu to reduce bit depth
3. TIMBRE for pulse width
4. Add vibrato with FM input

### Advanced Techniques

**Wavetable Scanning**
- Use **WLIN** for smooth morphing
- Modulate TIMBRE for wavetable position
- COLOR adds spread/chorus effect

**Granular Textures**
- **GRAIN** or **CLOUD** algorithms
- TIMBRE controls grain size
- Slow modulation creates evolving pads
- Fast modulation for glitch effects

**Physical Modeling**
- **PLUK**, **BOWD**, **BLOW** for realistic instruments
- TIMBRE affects excitation method
- COLOR controls damping/brightness
- Velocity sensitivity via trigger input

## Modulation System

### Overview
Braidy features a comprehensive modulation matrix with dual LFOs for dynamic sound design:
- **2 independent LFOs** with multiple waveforms
- **22+ modulation destinations** covering all synthesis parameters
- **Tempo sync** for rhythmic modulation
- **META mode** for algorithm morphing

### LFO Configuration

#### Available Waveforms
- **Sine**: Smooth, classic modulation
- **Triangle**: Linear sweep modulation
- **Square**: On/off switching effects
- **Saw Up**: Rising ramp modulation
- **Saw Down**: Falling ramp modulation
- **Sample & Hold**: Random stepped modulation
- **Noise**: Continuous random modulation

#### LFO Parameters
- **Rate**: 0.01Hz to 50Hz (or tempo-synced divisions)
- **Depth**: 0-100% modulation amount
- **Tempo Sync**: Lock to DAW tempo (1/32 to 8 bars)
- **Phase**: Starting phase offset

### Modulation Destinations

#### Synthesis Parameters
- **TIMBRE**: Main synthesis parameter modulation
- **COLOR**: Secondary parameter modulation
- **ALGORITHM**: Algorithm selection (META mode)
- **BIT_DEPTH**: Dynamic bit crushing
- **SAMPLE_RATE**: Sample rate modulation

#### Pitch & Tuning
- **PITCH**: Overall pitch modulation (vibrato)
- **FM_AMOUNT**: FM depth modulation
- **VIBRATO_RATE**: Vibrato speed control
- **VIBRATO_AMOUNT**: Vibrato depth control

#### Envelope Parameters
- **ATTACK**: Attack time modulation
- **DECAY**: Decay time modulation
- **ENVELOPE_AMOUNT**: Overall envelope depth

#### Advanced Destinations
- **QUANTIZE_SCALE**: Scale selection morphing
- **QUANTIZE_ROOT**: Root note shifting
- **DRIFT**: Oscillator drift amount
- **SIGNATURE**: Time signature modulation

### MOD Button & Overlay

Press the **MOD** button to access the modulation settings overlay:

1. **LFO Selection**: Choose LFO 1 or LFO 2
2. **Shape**: Select waveform type
3. **Rate**: Adjust modulation speed
4. **Depth**: Set modulation amount
5. **Destination**: Choose parameter to modulate
6. **Tempo Sync**: Enable/disable DAW sync

### META Mode - Algorithm Morphing

META mode enables smooth morphing between synthesis algorithms:

1. Enter menu system (long-press encoder)
2. Navigate to **META** page
3. Enable META mode
4. Assign LFO to **ALGORITHM_SELECTION** destination
5. The LFO will smoothly morph between adjacent algorithms

**Tips for META Mode:**
- Use slow LFO rates for evolving textures
- Triangle wave creates smooth bidirectional sweeps
- Square wave switches between two algorithms
- Combine with parameter modulation for complex morphing

### Modulation Matrix Routing

Each LFO can be routed to any destination:

1. **Single Routing**: One LFO to one destination
2. **Multiple Destinations**: Route one LFO to multiple parameters
3. **Crossmodulation**: Different LFOs to related parameters

### Practical Examples

**Vibrato Effect**
- LFO 1 → PITCH
- Shape: Sine
- Rate: 5Hz
- Depth: 10%

**Rhythmic Filter**
- LFO 1 → COLOR (filter cutoff)
- Shape: Square
- Tempo Sync: 1/8
- Depth: 75%

**Evolving Pad**
- LFO 1 → TIMBRE (slow)
- LFO 2 → COLOR (medium)
- Both sine waves, different rates

**Glitch Effects**
- LFO 1 → BIT_DEPTH
- Shape: Sample & Hold
- Rate: 10Hz
- Depth: 100%

## Polyphony & Voice Management

Braidy supports up to 16-voice polyphony:
- Voice allocation: Last-note priority
- Voice stealing: Oldest voice first
- Each voice has independent:
  - Oscillator state
  - Envelope generators
  - Parameter interpolation
  - Modulation routing

## MIDI Implementation

### Note Input
- **Note On/Off**: C-2 to G8 range
- **Velocity**: 0-127 (affects amplitude)
- **Pitch Bend**: ±2 semitones default

### CC Mappings
- **CC 1 (Mod Wheel)**: Assignable
- **CC 74**: Timbre parameter
- **CC 71**: Color parameter
- **CC 73**: Attack time
- **CC 72**: Decay time

## Presets

### Factory Presets
Braidy includes 128 factory presets covering:
- Classic analog sounds
- Modern digital textures
- Percussion and drums
- Experimental soundscapes
- Template patches

### User Presets
- Save your own presets via the DAW interface
- Presets store all parameters including menu settings

## Technical Specifications

- **Sample Rate**: 44.1kHz - 192kHz
- **Bit Depth**: 32-bit float internal processing
- **Latency**: < 3ms at 44.1kHz
- **CPU Usage**: ~5-15% per instance (M1 Mac)
- **Polyphony**: 1-16 voices
- **Algorithm Count**: 48

## Troubleshooting

### No Sound
1. Check algorithm selection (not on NOIS without input)
2. Verify MIDI input is connected
3. Check VCA envelope settings in menu

### Clicking/Popping
1. Increase buffer size in DAW
2. Check CPU usage
3. Reduce polyphony if needed

### Display Issues
1. Window may need resizing
2. Check UI scale in DAW settings

### Algorithm Sounds Wrong
1. Reset parameters to default
2. Check BITS and RATE settings
3. Verify COLOR and TIMBRE at center

## Keyboard Shortcuts

- **Space**: Play/stop (standalone only)
- **↑/↓**: Change algorithm
- **←/→**: Adjust selected parameter
- **Enter**: Toggle edit mode
- **Escape**: Exit menu

## Credits

Braidy is based on the original Mutable Instruments Braids hardware module designed by Émilie Gillet.

This software recreation aims to preserve the unique character and innovative synthesis techniques of the original hardware while making them accessible in modern DAW environments.

## Support

For bug reports and feature requests, please visit:
https://github.com/danielraffel/braidy

---

*Braidy is not affiliated with or endorsed by Mutable Instruments.*