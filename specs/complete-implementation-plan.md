# Braidy Complete Implementation Plan
## Clean Port with Full Parity Testing

### Overview
Complete reimplementation using actual Braids DSP code, with comprehensive cleanup of failed attempts and rigorous parity testing.

### End Goal
- **Clean, public-ready codebase** - No confusing remnants from failed attempts
- **Full parity with Braids** - All 47 algorithms sound identical
- **Comprehensive testing** - Automated validation for every algorithm
- **Production ready** - AU, VST3, and Standalone all working perfectly

---

## Phase 0: Code Cleanup (4 hours)
### Remove All Failed Implementation Attempts

#### 0.1 Archive Old Code
```bash
# Create archive of old attempts for reference
mkdir -p archive/old_implementations
mv Source/BraidyCore/* archive/old_implementations/
mv Source/BraidyVoice/* archive/old_implementations/
mv testing/code/* archive/old_implementations/
git add archive/
git commit -m "Archive old implementation attempts"
```

#### 0.2 Clean Project Structure
```bash
# Remove old test files
rm -rf test_wavs/
rm -f test_all_47 test_algos* test_csaw* debug_* braidy_wav_gen*
rm -f *.wav *.cpp *.py
rm -f Source/test_*.cpp Source/*.py

# Clean up testing directory
rm -rf testing/juce_wavs/*
rm -rf testing/code/*
mkdir -p testing/harness
mkdir -p testing/references
mkdir -p testing/results
```

#### 0.3 Update CMakeLists.txt
```cmake
# Remove all references to old BraidyCore files
# Remove these lines:
# Source/BraidyCore/MacroOscillator.cpp
# Source/BraidyCore/AnalogOscillator.cpp
# Source/BraidyCore/DigitalOscillator.cpp
# etc.

# Keep only:
# Source/PluginProcessor.cpp
# Source/PluginEditor.cpp
# Source/adapters/* (to be created)
```

#### Deliverables
- Clean project with no old DSP code
- Archive folder with old attempts (not in main source)
- Updated CMakeLists.txt ready for Braids integration

---

## Phase 1: Braids Integration Setup (4 hours)
### Import and Build Original Braids as Library

#### 1.1 Verify Dependencies
```bash
# Ensure we have stmlib
cd eurorack
git clone https://github.com/pichenettes/stmlib.git  # if not present
```

#### 1.2 Create Braids Static Library Build
```cmake
# CMakeLists.txt additions
add_library(braids_dsp STATIC
    # Core oscillators
    eurorack/braids/macro_oscillator.cc
    eurorack/braids/analog_oscillator.cc
    eurorack/braids/digital_oscillator.cc
    
    # Support files
    eurorack/braids/quantizer.cc
    eurorack/braids/resources.cc
    
    # NO settings.cc, NO ui.cc, NO drivers
)

target_include_directories(braids_dsp PUBLIC
    eurorack
    eurorack/braids
    eurorack/stmlib
)

target_compile_definitions(braids_dsp PUBLIC
    TEST  # Disables hardware dependencies
)

# Set proper flags
target_compile_options(braids_dsp PRIVATE
    -fno-fast-math
    -fno-unsafe-math-optimizations
    -ffp-contract=off  # No FMA
)
```

#### 1.3 Create Adapter Structure
```
Source/
├── adapters/
│   ├── BraidsEngine.h       # Main wrapper
│   ├── BraidsEngine.cpp
│   ├── BraidsVoice.h        # JUCE voice
│   ├── BraidsVoice.cpp
│   ├── ModeRegistry.h       # Algorithm mapping
│   └── ModeRegistry.cpp
├── PluginProcessor.cpp      # Updated to use adapters
└── PluginEditor.cpp         # Updated UI
```

#### Deliverables
- Braids compiling as static library
- Clean adapter structure created
- No modifications to eurorack/ files

---

## Phase 2: Test Harness Implementation (6 hours)
### Build Comprehensive Testing Infrastructure

#### 2.1 Harness Structure
```
testing/
├── harness/
│   ├── BraidsReference.h/cpp     # Wrapper for original Braids
│   ├── TestRunner.h/cpp          # Main test orchestrator
│   ├── AudioComparator.h/cpp     # Comparison metrics
│   ├── SceneGenerator.h/cpp      # Test scene creation
│   └── ReportGenerator.h/cpp     # Results formatting
├── scenes/
│   ├── basic_tests.json          # Simple algorithm tests
│   ├── parameter_sweeps.json     # Full parameter coverage
│   └── edge_cases.json           # Sync, modulation, etc.
├── references/                   # Braids reference outputs
├── candidates/                   # JUCE implementation outputs
└── reports/                      # Test results
```

#### 2.2 Core Test Harness
```cpp
// testing/harness/TestRunner.h
class TestRunner {
public:
    struct TestConfig {
        bool testDirectDSP = true;      // Test wrapper directly
        bool testJUCEPlugin = true;     // Test through JUCE
        bool generateReferences = true;  // Create Braids references
        int sampleRate = 48000;
        int blockSize = 24;             // Braids standard
        int testDuration = 48000;       // 1 second
        uint32_t rngSeed = 42;          // Deterministic
    };
    
    struct TestResult {
        std::string algorithm;
        float rmsError;
        float peakError;
        float snr;
        float dcOffset;
        bool passed;
        std::string failureReason;
    };
    
    void runAllTests();
    void runAlgorithm(const std::string& name);
    void generateReport();
    
private:
    BraidsReference reference_;
    BraidsEngine candidate_;
    AudioComparator comparator_;
    std::vector<TestResult> results_;
};
```

#### 2.3 Reference Generator
```cpp
// testing/harness/BraidsReference.cpp
class BraidsReference {
    braids::MacroOscillator oscillator_;
    
public:
    void init() {
        oscillator_.Init();
        // Set deterministic RNG seed
        stmlib::Random::Init(42);
    }
    
    void render(const std::string& algorithm, 
                float* output, int numSamples) {
        // Map algorithm name to shape
        auto shape = ModeRegistry::getShape(algorithm);
        oscillator_.set_shape(shape);
        oscillator_.set_pitch(60 << 7);  // Middle C
        oscillator_.set_parameters(16384, 16384);  // 50%
        
        // Render in 24-sample blocks
        int16_t buffer[24];
        uint8_t sync[24] = {0};
        
        for (int i = 0; i < numSamples; i += 24) {
            int blockSize = std::min(24, numSamples - i);
            oscillator_.Render(sync, buffer, blockSize);
            
            // Convert to float
            for (int j = 0; j < blockSize; j++) {
                output[i + j] = buffer[j] / 32768.0f;
            }
        }
    }
};
```

#### 2.4 Audio Comparator
```cpp
// testing/harness/AudioComparator.cpp
class AudioComparator {
public:
    struct Metrics {
        float rmsError;
        float peakError;
        float snr;
        float dcOffset;
        float correlation;
        int sampleOffset;  // Alignment offset
    };
    
    Metrics compare(const float* reference, 
                    const float* candidate,
                    int numSamples) {
        // Find best alignment
        int offset = findBestAlignment(reference, candidate, numSamples);
        
        // Calculate metrics at best alignment
        Metrics m;
        m.sampleOffset = offset;
        
        float sumSquaredError = 0;
        float maxError = 0;
        
        for (int i = 0; i < numSamples - abs(offset); i++) {
            float ref = reference[i];
            float cand = candidate[i + offset];
            float error = abs(ref - cand);
            
            sumSquaredError += error * error;
            maxError = std::max(maxError, error);
        }
        
        m.rmsError = sqrt(sumSquaredError / numSamples);
        m.peakError = maxError;
        m.snr = 20 * log10(1.0f / (m.rmsError + 1e-10f));
        
        return m;
    }
};
```

#### Deliverables
- Complete test harness implementation
- Reference generation from original Braids
- Comparison metrics implementation
- JSON scene definitions

---

## Phase 3: BraidsEngine Adapter Implementation (6 hours)
### Create Clean Wrapper for Braids DSP

#### 3.1 BraidsEngine Implementation
```cpp
// Source/adapters/BraidsEngine.cpp
#include "BraidsEngine.h"
#include "braids/macro_oscillator.h"
#include "stmlib/dsp/parameter_interpolator.h"

class BraidsEngine::Impl {
public:
    braids::MacroOscillator oscillator;
    stmlib::ParameterInterpolator timbre_smoother;
    stmlib::ParameterInterpolator color_smoother;
    
    int16_t render_buffer[24];
    uint8_t sync_buffer[24];
    
    void init() {
        oscillator.Init();
        timbre_smoother.Init();
        color_smoother.Init();
        memset(sync_buffer, 0, sizeof(sync_buffer));
    }
};

BraidsEngine::BraidsEngine() : pimpl(std::make_unique<Impl>()) {
    pimpl->init();
}

void BraidsEngine::setAlgorithm(int algorithmId) {
    auto shape = static_cast<braids::MacroOscillatorShape>(algorithmId);
    pimpl->oscillator.set_shape(shape);
}

void BraidsEngine::render(float* outL, float* outR, int numSamples) {
    int processed = 0;
    
    while (processed < numSamples) {
        int blockSize = std::min(24, numSamples - processed);
        
        // Update smoothed parameters
        int16_t timbre = pimpl->timbre_smoother.Update(timbre_, blockSize);
        int16_t color = pimpl->color_smoother.Update(color_, blockSize);
        pimpl->oscillator.set_parameters(timbre, color);
        
        // Render
        pimpl->oscillator.Render(
            pimpl->sync_buffer,
            pimpl->render_buffer,
            blockSize
        );
        
        // Convert to float
        for (int i = 0; i < blockSize; i++) {
            float sample = pimpl->render_buffer[i] / 32768.0f;
            outL[processed + i] = sample;
            outR[processed + i] = sample;  // Mono for now
        }
        
        processed += blockSize;
    }
}
```

#### 3.2 Mode Registry
```cpp
// Source/adapters/ModeRegistry.cpp
const std::vector<AlgorithmInfo> ModeRegistry::algorithms = {
    {0,  "CSAW",       braids::MACRO_OSC_SHAPE_CSAW},
    {1,  "MORPH",      braids::MACRO_OSC_SHAPE_MORPH},
    {2,  "SAW_SQUARE", braids::MACRO_OSC_SHAPE_SAW_SQUARE},
    // ... all 47 algorithms
    {46, "DIGITAL_MODULATION", braids::MACRO_OSC_SHAPE_DIGITAL_MODULATION}
};
```

#### Deliverables
- Clean BraidsEngine wrapper
- Complete algorithm registry
- No reimplementation, just wrapping

---

## Phase 4: Initial Testing (4 hours)
### Validate Wrapper Against Original

#### 4.1 Run Basic Tests
```bash
cd testing/harness
./test_runner --algorithm CSAW --generate-reference
./test_runner --algorithm CSAW --compare

# Expected output:
# CSAW: RMS Error: 0.00001, SNR: 90dB ✅ PASS
```

#### 4.2 Test Critical Algorithms
```bash
# Test each category
./test_runner --algorithm CSAW        # Analog
./test_runner --algorithm TOY         # Digital  
./test_runner --algorithm FM          # FM
./test_runner --algorithm WAVETABLES  # Wavetable
./test_runner --algorithm KICK        # Drums
./test_runner --algorithm PLUCKED     # Physical
```

#### 4.3 Fix Any Issues
- If RMS > 0.001: Check conversion
- If DC offset: Check initialization
- If silent: Check algorithm mapping

#### Deliverables
- CSAW working with < 0.001 RMS error
- At least one algorithm from each category working
- Issues documented with fixes

---

## Phase 5: JUCE Integration (6 hours)
### Connect BraidsEngine to Plugin

#### 5.1 Update PluginProcessor
```cpp
// Source/PluginProcessor.cpp
class BraidyAudioProcessor : public AudioProcessor {
private:
    std::vector<std::unique_ptr<BraidsVoice>> voices_;
    Synthesiser synth_;
    
    AudioProcessorValueTreeState parameters_;
    
public:
    void prepareToPlay(double sampleRate, int blockSize) override {
        synth_.clearVoices();
        
        // Add voices
        for (int i = 0; i < 8; ++i) {
            auto voice = std::make_unique<BraidsVoice>();
            voice->prepare(sampleRate);
            synth_.addVoice(voice.release());
        }
        
        synth_.setCurrentPlaybackSampleRate(sampleRate);
    }
    
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override {
        // Update voice parameters from APVTS
        auto algorithm = parameters_.getRawParameterValue("algorithm");
        auto timbre = parameters_.getRawParameterValue("timbre");
        auto color = parameters_.getRawParameterValue("color");
        
        for (int i = 0; i < synth_.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<BraidsVoice*>(synth_.getVoice(i))) {
                voice->setAlgorithm(algorithm->load());
                voice->setTimbre(timbre->load());
                voice->setColor(color->load());
            }
        }
        
        // Render
        synth_.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    }
};
```

#### 5.2 BraidsVoice Implementation
```cpp
// Source/adapters/BraidsVoice.cpp
class BraidsVoice : public SynthesiserVoice {
private:
    BraidsEngine engine_;
    float level_ = 0.0f;
    int currentNote_ = -1;
    
public:
    void startNote(int midiNote, float velocity, 
                   SynthesiserSound*, int) override {
        currentNote_ = midiNote;
        level_ = velocity;
        
        // Convert MIDI to Braids pitch
        int16_t pitch = (midiNote << 7);
        engine_.setPitch(pitch);
        engine_.trigger();
    }
    
    void renderNextBlock(AudioBuffer<float>& buffer,
                        int startSample, int numSamples) override {
        if (currentNote_ < 0) return;
        
        auto* left = buffer.getWritePointer(0, startSample);
        auto* right = buffer.getWritePointer(1, startSample);
        
        engine_.render(left, right, numSamples);
        
        // Apply velocity
        for (int i = 0; i < numSamples; ++i) {
            left[i] *= level_;
            right[i] *= level_;
        }
    }
};
```

#### Deliverables
- JUCE plugin using BraidsEngine
- MIDI working correctly
- Parameters connected to UI

---

## Phase 6: Full Algorithm Testing (8 hours)
### Test All 47 Algorithms

#### 6.1 Automated Test Suite
```python
# testing/harness/run_all_tests.py
import subprocess
import json

ALGORITHMS = [
    "CSAW", "MORPH", "SAW_SQUARE", "SINE_TRIANGLE",
    "BUZZ", "SQUARE_SUB", "SAW_SUB", "SQUARE_SYNC",
    # ... all 47
]

def run_full_matrix():
    results = {}
    
    for algo in ALGORITHMS:
        # Generate reference
        subprocess.run(["./test_runner", 
                       "--algorithm", algo,
                       "--generate-reference"])
        
        # Test wrapper
        result = subprocess.run(["./test_runner",
                                "--algorithm", algo,
                                "--test-wrapper"],
                               capture_output=True)
        
        # Test JUCE plugin
        result_juce = subprocess.run(["./test_runner",
                                     "--algorithm", algo,
                                     "--test-plugin"],
                                    capture_output=True)
        
        results[algo] = parse_results(result, result_juce)
    
    generate_report(results)
```

#### 6.2 Test Report Format
```markdown
# Braidy Parity Test Report

## Summary
- **Total Algorithms**: 47
- **Passing**: 45 (95.7%)
- **Failing**: 2 (4.3%)

## Results by Algorithm

| Algorithm | Wrapper RMS | Plugin RMS | SNR (dB) | Status |
|-----------|-------------|------------|----------|--------|
| CSAW      | 0.00001    | 0.00002    | 89.5     | ✅ PASS |
| MORPH     | 0.00001    | 0.00001    | 91.2     | ✅ PASS |
| TOY       | 0.00003    | 0.00003    | 85.3     | ✅ PASS |
| KICK      | 0.00002    | 0.00002    | 88.7     | ✅ PASS |
...

## Failing Algorithms
1. **GRANULAR_CLOUD**: RMS 0.02 - Issue with grain envelope
2. **PARTICLE_NOISE**: Silent output - RNG seed mismatch

## Next Steps
- Fix GRANULAR_CLOUD envelope calculation
- Match RNG sequence for PARTICLE_NOISE
```

#### Deliverables
- All 47 algorithms tested
- > 95% passing with SNR > 60dB
- Clear documentation of any issues

---

## Phase 7: Plugin Formats Testing (4 hours)
### Validate AU, VST3, and Standalone

#### 7.1 Build All Formats
```bash
./scripts/build.sh au
./scripts/build.sh vst3
./scripts/build.sh standalone
```

#### 7.2 Format-Specific Testing
```bash
# Test each format
./test_runner --format au --all-algorithms
./test_runner --format vst3 --all-algorithms
./test_runner --format standalone --all-algorithms
```

#### 7.3 DAW Testing
- Logic Pro: Load AU, test all algorithms
- Ableton: Load VST3, test all algorithms
- Reaper: Load both, compare

#### Deliverables
- All formats building correctly
- Identical output across formats
- No DAW-specific issues

---

## Phase 8: Final Cleanup & Documentation (4 hours)
### Prepare for Public Release

#### 8.1 Remove Debug Code
```cpp
// Remove all printf/cout statements
// Remove test-only code paths
// Clean up comments
```

#### 8.2 Documentation
```markdown
# Braidy - Accurate JUCE Port of Mutable Instruments Braids

## Features
- All 47 original algorithms
- Bit-accurate DSP (verified with automated tests)
- 8-voice polyphony
- AU, VST3, and Standalone formats

## Implementation
This is a clean-room port using the original Braids DSP code
with minimal adaptation layers for JUCE integration.

## Testing
Comprehensive test suite validates audio parity with original:
- RMS error < 0.001 for all algorithms
- SNR > 60dB minimum
- Automated regression testing
```

#### 8.3 Archive Cleanup
```bash
# Remove archive folder
rm -rf archive/old_implementations

# Clean git history (optional)
git filter-branch --tree-filter 'rm -rf old_code' HEAD
```

#### Deliverables
- Clean, commented code
- Complete documentation
- No traces of failed attempts

---

## Success Criteria

### Must Have (100% Required)
- ✅ All 47 algorithms produce correct audio
- ✅ RMS error < 0.001 vs original Braids
- ✅ No crashes or undefined behavior
- ✅ Clean codebase with no old implementations

### Should Have (Target 95%)
- ✅ SNR > 80dB for analog algorithms
- ✅ SNR > 60dB for digital algorithms
- ✅ Identical behavior across AU/VST3/Standalone
- ✅ < 10% CPU for 8 voices at 96kHz

### Nice to Have
- ✅ Stereo enhancement options
- ✅ Additional polyphony (16+ voices)
- ✅ Parameter modulation sources

---

## Timeline

- **Phase 0**: 4 hours - Cleanup
- **Phase 1**: 4 hours - Braids setup
- **Phase 2**: 6 hours - Test harness
- **Phase 3**: 6 hours - BraidsEngine
- **Phase 4**: 4 hours - Initial testing
- **Phase 5**: 6 hours - JUCE integration
- **Phase 6**: 8 hours - Full testing
- **Phase 7**: 4 hours - Format testing
- **Phase 8**: 4 hours - Final cleanup

**Total: 46 hours (5-6 days)**

---

## Critical Rules

1. **NEVER MODIFY** files in `eurorack/`
2. **ONLY CREATE** adapter code in `Source/adapters/`
3. **DELETE ALL** old implementation attempts
4. **TEST EVERYTHING** with automated harness
5. **DOCUMENT ALL** deviations or issues

## End State

- Public repository with clean, professional code
- Full parity with original Braids
- Comprehensive test suite proving accuracy
- Production-ready plugin in all formats
- No confusion from multiple implementation attempts