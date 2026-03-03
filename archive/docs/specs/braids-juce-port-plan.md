# Braids to JUCE Port - Implementation Plan

## Executive Summary
Port the actual Braids DSP code to JUCE, preserving the original algorithms exactly. No reimplementations, no guessing - use the real code with proper adapters.

## Key Principles
1. **Use the actual Braids/stmlib code** - No reimplementations
2. **Minimal modifications** - Only adapt interfaces, don't change DSP
3. **Clean architecture** - Clear separation between original code and adapters
4. **Stereo & polyphony ready** - Design for multiple voices from the start

## Project Structure
```
braidy/
├── eurorack/
│   ├── braids/          # Original Braids code (untouched)
│   └── stmlib/          # Original stmlib (untouched)
├── Source/
│   ├── adapters/        # Wrappers to connect Braids to JUCE
│   │   ├── BraidsEngine.h/cpp
│   │   ├── BraidsVoice.h/cpp
│   │   └── ModeRegistry.h/cpp
│   ├── core/           # JUCE plugin infrastructure
│   └── tests/          # Golden reference tests
└── CMakeLists.txt      # Build configuration
```

## Step-by-Step Implementation Plan

### Phase 1: Setup Build System (2 hours)

#### 1.1 CMakeLists.txt Configuration
```cmake
# Vendor Braids and stmlib
add_subdirectory(eurorack/braids EXCLUDE_FROM_ALL)
add_subdirectory(eurorack/stmlib EXCLUDE_FROM_ALL)

# Create Braids library target
add_library(braids_dsp STATIC
    # Core DSP files only (no hardware/UI)
    eurorack/braids/macro_oscillator.cc
    eurorack/braids/analog_oscillator.cc
    eurorack/braids/digital_oscillator.cc
    eurorack/braids/quantizer.cc
    eurorack/braids/resources.cc
    
    # stmlib DSP utilities
    eurorack/stmlib/dsp/parameter_interpolator.cc
    eurorack/stmlib/dsp/polyblep.cc
    eurorack/stmlib/utils/random.cc
)

target_include_directories(braids_dsp PUBLIC
    eurorack
    eurorack/braids
    eurorack/stmlib
)

# Disable hardware-specific code
target_compile_definitions(braids_dsp PUBLIC
    TEST         # Disables STM32 hardware code
    BRAIDS_DSP   # Custom flag for our port
)

# Main plugin target
juce_add_plugin(${PROJECT_NAME}
    PLUGIN_MANUFACTURER_CODE Brai
    PLUGIN_CODE Brdy
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Braidy"
)

target_sources(${PROJECT_NAME} PRIVATE
    Source/adapters/BraidsEngine.cpp
    Source/adapters/BraidsVoice.cpp
    Source/adapters/ModeRegistry.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    braids_dsp
    juce::juce_audio_utils
    juce::juce_dsp
)
```

### Phase 2: Core DSP Extraction (4 hours)

#### 2.1 BraidsEngine Wrapper
```cpp
// Source/adapters/BraidsEngine.h
#pragma once
#include "braids/macro_oscillator.h"
#include "stmlib/dsp/parameter_interpolator.h"

class BraidsEngine {
public:
    BraidsEngine();
    ~BraidsEngine();
    
    // Lifecycle
    void prepare(double sampleRate);
    void reset();
    
    // Parameters
    void setMode(int modeId);
    void setTimbre(float value);  // 0-1 mapped to 0-32767
    void setColor(float value);   // 0-1 mapped to 0-32767
    void setPitch(float midiNote, float pitchBend = 0.0f);
    
    // Rendering
    void render(float* outL, float* outR, int numSamples);
    
private:
    braids::MacroOscillator oscillator_;
    stmlib::ParameterInterpolator timbre_interpolator_;
    stmlib::ParameterInterpolator color_interpolator_;
    
    // Internal buffers for 24-sample processing
    int16_t render_buffer_[24];
    uint8_t sync_buffer_[24];
    
    // Sample rate handling
    double sample_rate_;
    double sr_scale_;  // 48000.0 / sample_rate_
    
    // Current parameters
    int16_t pitch_;
    int16_t timbre_;
    int16_t color_;
};

// Source/adapters/BraidsEngine.cpp
#include "BraidsEngine.h"

BraidsEngine::BraidsEngine() {
    oscillator_.Init();
    timbre_interpolator_.Init();
    color_interpolator_.Init();
}

void BraidsEngine::prepare(double sampleRate) {
    sample_rate_ = sampleRate;
    sr_scale_ = 48000.0 / sampleRate;
    
    // Braids expects 48kHz timing
    oscillator_.set_sample_rate(48000);
}

void BraidsEngine::render(float* outL, float* outR, int numSamples) {
    int processed = 0;
    
    while (processed < numSamples) {
        // Process in 24-sample blocks (Braids standard)
        int blockSize = std::min(24, numSamples - processed);
        
        // Update interpolated parameters
        oscillator_.set_parameters(
            timbre_interpolator_.Update(timbre_, blockSize),
            color_interpolator_.Update(color_, blockSize)
        );
        
        // Render to internal buffer
        memset(sync_buffer_, 0, blockSize);
        oscillator_.Render(sync_buffer_, render_buffer_, blockSize);
        
        // Convert int16 to float and output
        for (int i = 0; i < blockSize; ++i) {
            float sample = render_buffer_[i] / 32768.0f;
            outL[processed + i] = sample;
            outR[processed + i] = sample;  // Mono for now
        }
        
        processed += blockSize;
    }
}
```

### Phase 3: JUCE Voice Integration (3 hours)

#### 3.1 BraidsVoice Implementation
```cpp
// Source/adapters/BraidsVoice.h
class BraidsVoice : public juce::SynthesiserVoice {
public:
    BraidsVoice();
    
    bool canPlaySound(SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, 
                   SynthesiserSound*, int pitchWheel) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controller, int newValue) override;
    void renderNextBlock(AudioBuffer<float>&, int start, int numSamples) override;
    
    // Parameter updates from processor
    void setMode(int modeId);
    void setTimbre(float value);
    void setColor(float value);
    
private:
    BraidsEngine engine_;
    float currentLevel_;
    float targetLevel_;
    int currentNote_;
    float pitchBend_;
};
```

### Phase 4: Mode Registry (2 hours)

#### 4.1 Mode Mapping
```cpp
// Source/adapters/ModeRegistry.h
class ModeRegistry {
public:
    struct Mode {
        int id;
        const char* name;
        braids::MacroOscillatorShape shape;
    };
    
    static const std::vector<Mode>& getModes() {
        static const std::vector<Mode> modes = {
            {0, "CSAW", braids::MACRO_OSC_SHAPE_CSAW},
            {1, "MORPH", braids::MACRO_OSC_SHAPE_MORPH},
            {2, "SAW_SQUARE", braids::MACRO_OSC_SHAPE_SAW_SQUARE},
            // ... all 47 modes
        };
        return modes;
    }
};
```

### Phase 5: Parameter System (2 hours)

#### 5.1 AudioProcessorValueTreeState Setup
```cpp
// In PluginProcessor constructor
parameters = std::make_unique<AudioProcessorValueTreeState>(
    *this, nullptr, "Parameters",
    {
        std::make_unique<AudioParameterChoice>(
            "mode", "Mode", 
            ModeRegistry::getModeNames(), 0),
        
        std::make_unique<AudioParameterFloat>(
            "timbre", "Timbre", 
            0.0f, 1.0f, 0.5f),
            
        std::make_unique<AudioParameterFloat>(
            "color", "Color",
            0.0f, 1.0f, 0.5f),
            
        std::make_unique<AudioParameterFloat>(
            "level", "Level",
            0.0f, 1.0f, 0.7f),
            
        std::make_unique<AudioParameterInt>(
            "voices", "Polyphony",
            1, 16, 4),
            
        std::make_unique<AudioParameterBool>(
            "oversampling", "2x Oversampling",
            false)
    }
);
```

### Phase 6: Sample Rate Handling (3 hours)

#### 6.1 Fixed 48kHz Core with Resampling
```cpp
class BraidsProcessor {
private:
    // Option A: Fixed 48kHz with resampling
    std::unique_ptr<juce::dsp::Oversampling<float>> resampler_;
    
    void prepareToPlay(double sampleRate, int blockSize) {
        if (sampleRate != 48000.0) {
            // Setup resampler
            int factor = 1;
            if (sampleRate > 48000) {
                factor = std::round(sampleRate / 48000.0);
            }
            resampler_ = std::make_unique<juce::dsp::Oversampling<float>>(
                2, factor, 
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
            );
        }
        
        // Prepare all voices at 48kHz
        for (auto& voice : voices_) {
            voice->prepare(48000.0);
        }
    }
};
```

### Phase 7: Testing Infrastructure (4 hours)

#### 7.1 Golden Reference Tests
```cpp
// Source/tests/GoldenTests.cpp
class BraidsGoldenTest : public juce::UnitTest {
public:
    void runTest() override {
        beginTest("CSAW at Middle C");
        
        BraidsEngine engine;
        engine.prepare(48000.0);
        engine.setMode(0);  // CSAW
        engine.setPitch(60.0f);  // Middle C
        engine.setTimbre(0.5f);
        engine.setColor(0.5f);
        
        // Render 1 second
        std::vector<float> output(48000);
        engine.render(output.data(), output.data(), 48000);
        
        // Compare with reference
        auto reference = loadReferenceWav("csaw_c4_48k.wav");
        float rms_error = calculateRMS(output, reference);
        
        expect(rms_error < 0.01f);  // 1% tolerance
    }
};
```

### Phase 8: Cleanup Old Code (2 hours)

#### 8.1 Remove Previous Attempts
- Delete all files in `Source/BraidyCore/` that reimplemented algorithms
- Keep only the adapter layer
- Archive old test files for reference

### Phase 9: Stereo Enhancement (Future - 4 hours)
```cpp
// Future enhancement for stereo
class StereoEngine : public BraidsEngine {
    void renderStereo(float* outL, float* outR, int numSamples) {
        // Render mono core
        BraidsEngine::render(outL, outR, numSamples);
        
        // Apply stereo widening
        for (int i = 0; i < numSamples; ++i) {
            float mono = outL[i];
            float wide = delay_line_[delay_pos_] * 0.3f;
            outL[i] = mono + wide;
            outR[i] = mono - wide;
            delay_line_[delay_pos_] = mono;
            delay_pos_ = (delay_pos_ + 1) % delay_length_;
        }
    }
};
```

## Success Metrics
1. ✅ All 47 algorithms produce audio
2. ✅ Frequency accuracy within 1Hz of reference
3. ✅ DC offset < 100 (0.3% of range)
4. ✅ No clipping at default parameters
5. ✅ CPU usage < 10% for 4 voices at 96kHz
6. ✅ Null test passes at 48kHz against original

## Timeline
- Phase 1-2: Day 1 (6 hours) - Core setup
- Phase 3-5: Day 2 (7 hours) - JUCE integration  
- Phase 6-7: Day 3 (7 hours) - Sample rate & testing
- Phase 8: Day 4 (2 hours) - Cleanup
- **Total: 22 hours for working implementation**

## Critical Success Factors
1. **DO NOT MODIFY** the Braids/stmlib source files
2. **USE ADAPTERS** to bridge between JUCE and Braids
3. **TEST EARLY** with simple algorithms like CSAW
4. **PRESERVE NUMERICS** - Keep fixed-point math intact
5. **24-SAMPLE BLOCKS** - Maintain Braids' processing cadence

## Next Steps
1. Clone stmlib if not already present
2. Create CMakeLists.txt as specified
3. Implement BraidsEngine wrapper
4. Test with CSAW algorithm
5. Iterate through all algorithms
6. Add polyphony and parameter smoothing