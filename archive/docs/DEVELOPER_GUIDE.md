# Braidy Developer Guide
## Technical Implementation Documentation

---

## Architecture Overview

Braidy is built using the JUCE framework and implements a faithful recreation of the Mutable Instruments Braids macro oscillator. The architecture consists of several key components:

```
┌─────────────────────────────────────┐
│         PluginProcessor             │
│  ┌─────────────────────────────┐    │
│  │     VoiceManager (16)       │    │
│  │  ┌───────────────────────┐  │    │
│  │  │   BraidyVoice (x16)   │  │    │
│  │  │  ┌─────────────────┐  │  │    │
│  │  │  │ MacroOscillator │  │  │    │
│  │  │  │   ├─ Analog     │  │  │    │
│  │  │  │   ├─ Digital    │  │  │    │
│  │  │  │   └─ Resources  │  │  │    │
│  │  │  └─────────────────┘  │  │    │
│  │  └───────────────────────┘  │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────┐
│         PluginEditor                │
│  ┌─────────────────────────────┐    │
│  │   Authentic Braids UI       │    │
│  │   - OLED Display            │    │
│  │   - Encoders & Knobs        │    │
│  │   - CV Jack Visual          │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
```

## Core Components

### 1. PluginProcessor (`Source/PluginProcessor.cpp`)

Main audio processing class that:
- Manages audio parameters
- Handles MIDI input
- Coordinates voice allocation
- Processes audio buffers

Key methods:
```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
void prepareToPlay(double sampleRate, int samplesPerBlock);
AudioProcessorValueTreeState& getValueTreeState();
```

### 2. VoiceManager (`Source/BraidyVoice/VoiceManager.cpp`)

Manages polyphonic voice allocation:
- 16-voice polyphony
- Last-note priority
- Voice stealing (oldest first)
- Parameter smoothing

```cpp
class VoiceManager {
    std::array<BraidyVoice, 16> voices_;
    void noteOn(int note, float velocity);
    void noteOff(int note);
    void processBlock(AudioBuffer<float>& buffer);
};
```

### 3. BraidyVoice (`Source/BraidyVoice/BraidyVoice.cpp`)

Individual synthesizer voice containing:
- MacroOscillator instance
- Envelope generators
- Parameter interpolation
- Voice state management

```cpp
class BraidyVoice {
    MacroOscillator oscillator_;
    ADSREnvelope envelope_;
    void startNote(int midiNote, float velocity);
    void stopNote();
    void renderNextBlock(AudioBuffer<float>& buffer);
};
```

### 4. MacroOscillator (`Source/BraidyCore/MacroOscillator.cpp`)

Core synthesis engine managing:
- Algorithm selection
- Parameter routing
- Sub-oscillator coordination

```cpp
class MacroOscillator {
    AnalogOscillator analog_;
    DigitalOscillator digital_;
    void SetShape(BraidsAlgorithm algorithm);
    void SetParameter(int param, float value);
    void Render(int16_t* buffer, size_t size);
};
```

### 5. Synthesis Oscillators

#### AnalogOscillator (`Source/BraidyCore/AnalogOscillator.cpp`)
Implements analog-style synthesis:
- Classic waveforms (saw, square, triangle)
- Waveshaping and folding
- Hard sync and ring modulation
- Super saw/square algorithms

#### DigitalOscillator (`Source/BraidyCore/DigitalOscillator.cpp`)
Implements digital synthesis algorithms:
- Wavetable synthesis (using real Braids data)
- FM synthesis (2-op, feedback, chaotic)
- Formant synthesis (VOSIM, VOWEL, FOF)
- Physical modeling (Karplus-Strong variants)
- Granular and particle synthesis
- Noise generators

### 6. Resources (`Source/BraidyCore/BraidyResources.cpp`)

Manages wavetable and resource data:
- 33KB of authentic Braids wavetable data
- Lookup tables for efficient processing
- Interpolation utilities

```cpp
// Wavetable data structure
const int16_t braids_wavetable_data[16512] = { /* ... */ };

// Efficient lookup with interpolation
template<typename T>
class LookupTable {
    T LookupInterpolated(uint32_t phase);
};
```

## Synthesis Algorithms

### Algorithm Categories

1. **Analog (0-8)**: Classic subtractive synthesis
2. **Wavetable (9-16)**: Wavetable scanning and morphing
3. **Formant (17-19)**: Vocal and formant synthesis
4. **Additive (20-26)**: Harmonic and additive techniques
5. **FM (27-29)**: Frequency modulation variants
6. **Physical (30-38)**: Physical modeling
7. **Experimental (39-47)**: Digital noise and effects

### Algorithm Implementation Pattern

Each algorithm follows this pattern:

```cpp
void RenderAlgorithm(int16_t* buffer, size_t size) {
    // 1. Update oscillator state
    UpdatePhase();
    
    // 2. Apply timbre parameter
    float shape = timbre_parameter_ * scale_factor;
    
    // 3. Apply color parameter
    float brightness = color_parameter_ * filter_range;
    
    // 4. Generate samples
    for (size_t i = 0; i < size; ++i) {
        int32_t sample = GenerateSample(phase_, shape, brightness);
        buffer[i] = Clip16(sample);
        phase_ += phase_increment_;
    }
}
```

## User Interface

### UI Architecture (`Source/PluginEditor.cpp`)

The UI faithfully recreates the Braids hardware:

```cpp
class BraidyAudioProcessorEditor {
    // Custom components
    class BraidsEncoder;   // Main rotary encoder
    class BraidsKnob;      // Parameter knobs
    class CvJack;          // Visual CV jacks
    class SimpleOLEDDisplay; // 4-char display
    
    // Layout matching 16HP Eurorack format
    void resized() {
        // Precise positioning for authentic look
    }
};
```

### Display System

4-character OLED display with modes:
- **Algorithm**: Shows current algorithm name
- **Menu**: Shows menu page name
- **Value**: Shows parameter value
- **Startup**: Shows "BRDY" splash

### Interaction Model

```cpp
// Encoder interactions
void handleEncoderRotation(int delta);  // Change values
void handleEncoderClick();              // Enter/exit edit
void handleEncoderLongPress();          // Enter/exit menu

// Menu navigation
enum class MenuPage { META, BITS, RATE, /* ... */ };
void navigateMenu(int delta);
void editMenuValue(int delta);
```

## Parameter System

### Audio Parameters

Parameters are managed via JUCE's AudioProcessorValueTreeState:

```cpp
AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    return {
        std::make_unique<AudioParameterChoice>("algorithm", "Algorithm", 
            algorithmNames, 0),
        std::make_unique<AudioParameterFloat>("timbre", "Timbre", 
            0.0f, 1.0f, 0.5f),
        std::make_unique<AudioParameterFloat>("color", "Color", 
            0.0f, 1.0f, 0.5f),
        // ... more parameters
    };
}
```

### Parameter Mapping

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Algorithm | 0-47 | 0 | Synthesis algorithm selection |
| Timbre | 0.0-1.0 | 0.5 | Primary synthesis parameter |
| Color | 0.0-1.0 | 0.5 | Secondary synthesis parameter |
| Coarse | -60-60 | 0 | Coarse pitch (semitones) |
| Fine | -100-100 | 0 | Fine pitch (cents) |
| FM Amount | -1.0-1.0 | 0 | FM attenuverter |
| Attack | 0-127 | 0 | Envelope attack |
| Decay | 0-127 | 64 | Envelope decay |

## Build System

### CMake Configuration

The project uses CMake with JUCE:

```cmake
juce_add_plugin(braidy
    COMPANY_NAME "Generous Corp"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "braidy"
)

# Feature flags
option(BRAIDY_ENABLE_ALL_MODELS "Enable all 45 synthesis models" ON)
option(BRAIDY_ENABLE_POLYPHONY "Enable polyphonic operation" ON)
```

### Build Commands

```bash
# Generate Xcode project
./scripts/generate_and_open_xcode.sh

# Build specific format
./scripts/build.sh au          # Audio Unit
./scripts/build.sh vst3        # VST3
./scripts/build.sh standalone  # Standalone app

# Full build with testing
./scripts/build.sh all test
```

## Performance Optimization

### Key Optimizations

1. **Lookup Tables**: Pre-calculated waveforms and interpolation
2. **Fixed-Point Math**: Using int32_t for phase accumulation
3. **SIMD Operations**: Vectorized processing where applicable
4. **Branch Prediction**: Algorithm-specific render paths
5. **Memory Layout**: Cache-friendly data structures

### Profiling Results

Typical CPU usage (M1 Mac, 44.1kHz):
- Single voice: ~1-2%
- 8 voices: ~5-8%
- 16 voices: ~10-15%

## Testing

### Unit Tests

Test coverage for core components:
```cpp
TEST(MacroOscillator, AlgorithmSelection) {
    MacroOscillator osc;
    osc.SetShape(ALGORITHM_CSAW);
    // Verify output characteristics
}

TEST(VoiceManager, Polyphony) {
    VoiceManager manager;
    // Test voice allocation and stealing
}
```

### Integration Tests

End-to-end testing via PluginVal:
```bash
./scripts/validate_plugin.sh
```

## Debugging

### Debug Macros

```cpp
#if DEBUG
    #define DBG_OSCILLATOR(msg) DBG("Oscillator: " << msg)
    #define DBG_VOICE(voice, msg) DBG("Voice " << voice << ": " << msg)
#else
    #define DBG_OSCILLATOR(msg)
    #define DBG_VOICE(voice, msg)
#endif
```

### Common Issues

1. **Clicking/Popping**: Check envelope smoothing
2. **Wrong Pitch**: Verify sample rate conversion
3. **No Sound**: Check voice allocation and MIDI routing
4. **CPU Spikes**: Profile algorithm-specific paths

## Future Enhancements

### Planned Features
- [ ] MPE support for expressive control
- [ ] Microtuning and alternate scales
- [ ] Additional synthesis algorithms
- [ ] Preset morphing
- [ ] Built-in effects (reverb, delay)
- [ ] CV modulation visualization

### Performance Improvements
- [ ] AVX/NEON optimizations
- [ ] GPU-accelerated wavetable synthesis
- [ ] Lazy evaluation for unused parameters
- [ ] Dynamic voice allocation

## Contributing

### Code Style

Follow the existing code style:
```cpp
// Class names: PascalCase
class MacroOscillator {
    // Member variables: underscore suffix
    float timbre_parameter_;
    
    // Methods: PascalCase
    void RenderSamples(int16_t* buffer);
    
    // Constants: UPPER_CASE
    static constexpr float TWO_PI = 6.28318530718f;
};
```

### Submission Guidelines

1. Fork the repository
2. Create a feature branch
3. Write tests for new features
4. Ensure all tests pass
5. Submit a pull request

## License

This project is based on the original Mutable Instruments Braids code, which is released under the MIT license. See LICENSE file for details.

---

*For user documentation, see USER_MANUAL.md*
*For build instructions, see README.md*