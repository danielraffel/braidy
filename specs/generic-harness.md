# Braidy Audio Parity Harness Plan v6 (Implementation-Ready)

Purpose: A bulletproof, unambiguous plan to verify audio parity between the original eurorack Braids module and the JUCE Braidy implementation, with precise diagnostics for achieving bit-accurate (or perceptually transparent) audio output.

## Critical Principles
- **Determinism**: Fixed RNG seed with identical sequence, FTZ/DAZ on, no FMA, no fast-math, single-threaded offline
- **Dual-Path Testing**: Test BOTH direct BraidsDigitalDSP AND full PluginProcessor path
- **48kHz Priority**: Braids LUTs/curves are 48k-based; prioritize pre-resampler 48kHz parity
- **Pre-Resampler Tap**: PluginProcessor must expose pre-resampler 48kHz output in harness mode
- **24-Sample Cadence**: Match Braids' 24-sample parameter update cadence (not 128)
- **Sample-Accurate Events**: All parameter changes scheduled by exact sample offset
- **Local Reference**: Use checked-in eurorack/braids copy for repeatability

## Phase 0 — Braids Reference Setup (0.5 day)

### Deliverables
1. **Local Braids Build**
   ```bash
   # Use local copy of eurorack (check into repo for repeatability)
   cd eurorack/braids
   make -f test/makefile
   ```

2. **Reference Wrapper with RNG Parity**
   ```cpp
   // braids_reference_render.cpp
   class BraidsReference {
       MacroOscillator osc;
       Random rng;  // Must match Braids' Random::GetSample() sequence
       
       void Configure(uint16_t shape, int16_t pitch, 
                     uint16_t timbre, uint16_t color) {
           rng.Init(42);  // Same seed as JUCE harness
           // Ensure FTZ/DAZ enabled, no FMA
       }
       void Render(int32_t* output, size_t samples);
   };
   ```

3. **Canonical Algorithm Mapping**
   ```cpp
   // Map Braids MacroOscillatorShape to Braidy names (exact from your code)
   const std::map<std::string, MacroOscillatorShape> ALGORITHM_MAP = {
       {"CSAW", MACRO_OSC_SHAPE_CSAW},
       {"VOSIM", MACRO_OSC_SHAPE_VOSIM},          // not VOSM
       {"VOWEL", MACRO_OSC_SHAPE_VOWEL},          // not VOWL
       {"VOWEL_FOF", MACRO_OSC_SHAPE_VOWEL_FOF},  // separate from VOWEL
       {"HARMONICS", MACRO_OSC_SHAPE_HARMONICS},  // not HARM
       {"WAVETABLES", MACRO_OSC_SHAPE_WAVETABLES},// not WTBL
       {"WAVE_MAP", MACRO_OSC_SHAPE_WAVE_MAP},    // not WMAP
       {"WAVE_LINE", MACRO_OSC_SHAPE_WAVE_LINE},  // not WLIN
       {"WAVE_TERRAIN", MACRO_OSC_SHAPE_WAVE_TERRAIN}, // if custom, document
       // ... complete mapping for all 47
   };
   ```

### Acceptance
- Renders CSAW at 48kHz with deterministic output
- RNG sequence matches between Braids and JUCE (verified with test pattern)
- FTZ/DAZ verified active, FMA disabled

## Phase 1 — Direct DSP Comparison at 48kHz (1 day)

### Deliverables

1. **Dual-Path Test Harness with Pre-Resampler Tap**
   ```cpp
   class BraidyTestHarness {
       // Path A: Direct DSP
       BraidsDigitalDSP directDSP;
       
       // Path B: Full Plugin with pre-resampler tap
       class TestableProcessor : public BraidyAudioProcessor {
       public:
           float* getPreResamplerOutput() { 
               // Return 48kHz core output before resampling
               return preResamplerBuffer; 
           }
       } processor;
       
       void renderDirectDSP(Scene& scene, float* output);
       void renderPluginPath(Scene& scene, float* output, bool preResampler);
   };
   ```

2. **Scene Runner with Gain-Normalized Metrics**
   ```cpp
   class SceneRunner {
       float computeGainNormalization(float* ref, float* cand);
       
       struct Metrics {
           // Absolute metrics (before gain normalization)
           float peakError;
           float rmsError;
           float snr;
           
           // Normalized metrics (after applying g)
           float gainFactor;  // The computed g
           float normalizedPeakError;
           float normalizedRmsError;
           float normalizedSnr;
           bool gainNormalized;  // Flag indicating normalized metrics
       };
       
       AlignmentResult alignSignals(float* ref, float* cand);
       // Returns: integerOffset, subSampleOffset, alignedSignal
   };
   ```

3. **Sub-Sample Alignment**
   - Integer offset via cross-correlation
   - Parabolic interpolation for sub-sample peak
   - Fractional-delay FIR for precise alignment

4. **Initial Algorithm Coverage** (exact names from MacroOscillatorShape)
   - Priority 1: CSAW, WAVETABLES, WAVE_MAP, WAVE_LINE, WAVE_TERRAIN
   - Priority 2: VOSIM, VOWEL, VOWEL_FOF, HARMONICS
   - Priority 3: FM, FEEDBACK_FM, WAVE_TERRAIN_FM (if exists)
   - Priority 4: PLUCKED, BOWED, BLOWN, FLUTED

5. **Baseline Scenes**
   ```cpp
   // Silence scene for DC/noise baseline
   Scene silenceScene = {
       .id = "silence_baseline",
       .algorithm = "CSAW",
       .noteOn = false,  // No note triggered
       .durationSamples = 48000
   };
   
   // Long sustain for drift detection
   Scene sustainScene = {
       .id = "csaw_30s_sustain",
       .algorithm = "CSAW",
       .durationSamples = 48000 * 30  // 30 seconds
   };
   ```

### Scene JSON Format
```json
{
  "id": "csaw_sweep_48k",
  "algorithm": "CSAW",
  "parameters": {
    "timbre": 16384,      // Braids range: 0-32767 (canonical)
    "color": 16384,
    "engine_params": {}    // Algorithm-specific extras
  },
  "pitch": 60,             // MIDI note
  "parameterSmoothing": {
    "enabled": false,      // Disable for initial tests
    "cadenceSamples": 24   // When enabled, match Braids
  },
  "parameterHysteresis": {
    "testBoundaries": true,
    "wavetableIndex": 15    // Test near hysteresis boundaries
  },
  "modeChanges": [
    {"sampleOffset": 24000, "algorithm": "VOSIM", "params": {...}}
  ],
  "sync": {
    "generateHardSync": false,
    "periodSamples": 480,     // When enabled
    "phaseResetMode": "pre",  // Phase resets before increment
    "envelopeRetrig": true    // Envelope retrigs on sync
  },
  "render": {
    "durationSamples": 48000,
    "sampleRate": 48000,      // Priority: 48kHz
    "blockSize": 24,          // Match Braids cadence
    "coreMode": "fixed48k",   // or "nativeRate"
    "outputChannels": 1,
    "noteOn": true            // Can be false for silence test
  },
  "platform": {
    "rngSeed": 42,
    "rngSequence": "braids",  // Must match Braids Random::GetSample()
    "ftzDaz": true,
    "useFMA": false,
    "dither": "none"
  }
}
```

### Metrics & Gates

**Core Metrics (both absolute and normalized):**
- Peak difference (samples)
- RMS difference
- SNR (dB)
- DC offset
- Gain normalization factor `g`
- Integer + sub-sample offset

**48kHz Pre-Resampler Gates (Applied after gain normalization):**
- Normalized peak ≤ 2 LSB
- Normalized RMS ≤ 1 LSB  
- Normalized SNR ≥ 80 dB for steady tones
- Average spectral Δmag ≤ 0.1 dB (post-normalization)

**Report Format:**
```json
{
  "metrics": {
    "absolute": {
      "peak": 0.003,
      "rms": 0.001,
      "snr": 65.2
    },
    "normalized": {
      "gainFactor": 0.998,
      "peak": 0.0001,
      "rms": 0.00005,
      "snr": 82.1,
      "gainNormalized": true,
      "preResampler": true
    }
  }
}
```

### Acceptance
- CSAW passes all gates at 48kHz (both paths)
- Silence baseline shows < -90dB noise floor
- At least 3 other algorithms within tolerance
- Both direct-DSP and plugin pre-resampler paths tested

## Phase 2 — Advanced Diagnostics (1 day)

### Spectral Analysis (Post-Normalization)
```cpp
class SpectralDiagnostics {
    BandedMetrics computeBandedMagnitude(float* signal, float gainFactor);
    // Apply gain normalization before spectral analysis
    // Low: 0-0.2·Fs, Mid: 0.2-0.45·Fs, High: 0.45-0.5·Fs
    
    void generateSpectrogram(float* residual, const char* pngPath);
    // Log-magnitude STFT visualization of normalized residual
};
```

**Banded Δmag Gates (evaluated after gain normalization):**
- ≤ 0.25 dB up to 0.45·Fs
- ≤ 0.75 dB near Nyquist
- Average Δmag ≤ 0.1 dB

### Hard-Sync Semantics Test
```cpp
class SyncTest {
    void testPhaseResetTiming() {
        // Test whether phase resets before or after increment
        // Document exact behavior for parity
    }
    
    void testEnvelopeRetrigger() {
        // Verify envelope behavior on sync events
    }
};

// Targeted sync scene
Scene hardSyncScene = {
    .id = "hard_sync_semantics",
    .algorithm = "CSAW",
    .sync = {
        .generateHardSync = true,
        .periodSamples = 480,
        .phaseResetMode = "pre",  // Document actual behavior
        .envelopeRetrig = true
    }
};
```

### Stability & Drift Detection
```cpp
class StabilityAnalysis {
    int detectFirstNonFinite(float* signal, Scene& context);
    float computeDrift(float* ref, float* cand); // Sliding correlation
    float computeAliasingIndex(float* signal);   // HF energy ratio
    
    NoiseMetrics measureSilence(float* silenceOutput);
    // Baseline DC offset and noise floor
};
```

### State Persistence Test
```cpp
void testStatePersistence() {
    // Render 1000 samples
    processor.getStateInformation(state);
    // Reset processor
    processor.setStateInformation(state);
    // Continue render - must be bit-identical
}
```

### Algorithm-Specific Analysis

**Wavetable Algorithms with Hysteresis:**
- Verify wavetable data byte-for-byte
- Test parameter values near hysteresis boundaries
- Log selected table indices in report
- Check interpolation (linear vs cubic)

**FM Algorithms:**
- Compare modulation indices precisely
- Verify feedback scaling
- Check carrier/modulator ratios

**Physical Models:**
- Match exciter characteristics
- Verify filter coefficients
- Check delay line lengths

### Acceptance
- Spectral gates pass for test algorithms (post-normalization)
- No drift detected over 30s sustain
- Silence baseline < -90dB
- Hard-sync semantics documented and matched
- State persistence verified
- No NaN/Inf in any test

## Phase 3 — Sample Rate & Resampler Coverage (1 day)

### 3A: Native Rate Testing

Test at actual host rates:
- 44.1kHz (with srScale)
- 48kHz (native)
- 96kHz (upsampled, but expect LUT differences)

### 3B: Fixed-48k Core + Resampler

```cpp
class ResamplerTest {
    void testWithResampler(Scene& scene) {
        // Record resampler latency
        int latency = resampler.getLatency();
        
        // Set latency for DAW compensation (even if offline doesn't use it)
        processor.setLatencySamples(latency);
        
        // Use as initial alignment guess
        alignmentGuess = latency;
        
        // Test quality modes
        for (auto quality : {LINEAR, WINDOWED_SINC_HIGH}) {
            // Compare outputs
        }
        
        // Record in metadata
        metadata["resampler"]["latencySamples"] = latency;
        metadata["resampler"]["reportedToDAW"] = true;
    }
};
```

**Post-Resampler Gates (Relaxed, post-normalization):**
- Banded Δmag ≤ 0.25 dB (low/mid)
- Banded Δmag ≤ 0.75 dB (high)
- SNR ≥ 50 dB
- Transient peak error ≤ 5 LSB

### Block Size Variation
Test with blocks: 24, 64, 128, 256, 512
- Verify no block-boundary artifacts
- Check parameter smoothing consistency

### Acceptance
- 44.1/48/96kHz all pass respective gates
- Resampler latency documented and compensated
- setLatencySamples() called correctly
- Block size variation shows no regression

## Phase 4 — Comprehensive Algorithm Matrix (2 days)

### Full Coverage Testing

```python
class AlgorithmMatrix:
    # Exact names from MacroOscillatorShape enum
    ALGORITHMS = [
        "CSAW", "MORPH", "SAW_SQUARE", "SINE_TRIANGLE",
        "BUZZ", "SQUARE_SUB", "SAW_SUB", "SQUARE_SYNC",
        "SAW_SYNC", "TRIPLE_SAW", "TRIPLE_SQUARE", "TRIPLE_TRIANGLE",
        "TRIPLE_SINE", "TRIPLE_RING_MOD", "SAW_SWARM", "SAW_COMB",
        "TOY", "DIGITAL_FILTER_LP", "DIGITAL_FILTER_PK", "DIGITAL_FILTER_BP",
        "DIGITAL_FILTER_HP", "VOSIM", "VOWEL", "VOWEL_FOF",
        "HARMONICS", "FM", "FEEDBACK_FM", "CHAOTIC_FEEDBACK_FM",
        "PLUCKED", "BOWED", "BLOWN", "FLUTED",
        "STRUCK_BELL", "STRUCK_DRUM", "KICK", "CYMBAL",
        "SNARE", "WAVETABLES", "WAVE_MAP", "WAVE_LINE",
        "WAVE_PARAPHONIC", "FILTERED_NOISE", "TWIN_PEAKS_NOISE", "CLOCKED_NOISE",
        "GRANULAR_CLOUD", "PARTICLE_NOISE", "DIGITAL_MODULATION", "QUESTION_MARK"
    ]
    
    TIMBRE_POINTS = [0, 8192, 16384, 24576, 32767]
    COLOR_POINTS = [0, 8192, 16384, 24576, 32767]
    PITCHES = [36, 48, 60, 72, 84]  # C2-C6
    
    def run_matrix(self, filter_pattern=None):
        for _ in range(3):  # N=3 repetitions
            for algo in self.filter_algorithms(filter_pattern):
                for timbre in TIMBRE_POINTS:
                    for color in COLOR_POINTS:
                        for pitch in PITCHES:
                            result = self.run_test(algo, timbre, color, pitch)
                            self.results.append(result)
        
        self.compute_statistics()  # mean, std-dev per test
```

### Parameter Hysteresis Testing
```python
def test_hysteresis_boundaries():
    """Test parameter values that cross wavetable selection boundaries"""
    for wavetable_algo in ["WAVETABLES", "WAVE_MAP", "WAVE_LINE"]:
        for boundary in get_hysteresis_points(wavetable_algo):
            # Test just below, at, and just above boundary
            for offset in [-1, 0, 1]:
                timbre = boundary + offset
                result = run_test(wavetable_algo, timbre, 16384, 60)
                result["wavetableIndex"] = get_selected_table()
                log_hysteresis_behavior(result)
```

### Meta Modes & Special Cases

**META Mode:**
- Test smooth algorithm morphing
- Verify crossfade curves

**DIGITAL Modes:**
- Bit depth reduction accuracy
- Sample rate decimation

**Easter Eggs (QPSK, etc.):**
- Document expected behavior
- Verify bit-accuracy where applicable

### Report Generation

```markdown
## Algorithm Parity Report

| Algorithm | Timbre | Color | Pitch | SNR(dB) | Δmag-Low | Δmag-Mid | Δmag-High | Gain g | Offset | Normalized | PreResampler | Status |
|-----------|--------|-------|-------|---------|----------|----------|-----------|--------|--------|------------|--------------|--------|
| CSAW      | 50%    | 50%   | C4    | 85.2    | 0.02     | 0.04     | 0.12      | 0.998  | 0.23   | ✓          | ✓            | ✅ PASS |
| FM        | 75%    | 25%   | C3    | 62.1    | 0.18     | 0.22     | 0.68      | 1.003  | -0.15  | ✓          | ✓            | ✅ PASS |

### Hysteresis Behavior
| Algorithm  | Timbre | Expected Index | Actual Index | Match |
|------------|--------|----------------|--------------|-------|
| WAVETABLES | 8191   | 3              | 3            | ✅     |
| WAVETABLES | 8192   | 4              | 4            | ✅     |
| WAVETABLES | 8193   | 4              | 4            | ✅     |

[Spectrogram: csaw_50_50_c4_residual.png]
[Aligned WAV: csaw_50_50_c4_aligned.wav]
```

### Acceptance
- 95% of tests within tolerance
- Hysteresis boundaries behave correctly
- Remaining 5% have documented fix paths
- No systematic bias patterns

## Phase 5 — CI Integration & Ergonomics (0.5 day)

### GitHub Actions Workflow

```yaml
name: Audio Parity Tests
on: [push, pull_request]
jobs:
  smoke-tests:
    steps:
      - name: Setup FTZ/DAZ
        run: echo "Setting FTZ/DAZ flags..."
      
      - name: Build Braids Reference
        run: cd eurorack/braids && make -f test/makefile
      
      - name: Build Braidy
        run: ./scripts/build.sh standalone
      
      - name: Run Smoke Suite
        run: |
          python3 harness/run_matrix.py --filter "CSAW|VOSIM|FM|PLUCKED|KICK"
          python3 harness/run_matrix.py --scene silence_baseline
          python3 harness/run_matrix.py --scene hard_sync_semantics
      
      - name: Check Regression
        run: python3 harness/check_regression.py --threshold 0.1
      
      - name: Upload Reports
        uses: actions/upload-artifact@v2
        with:
          path: reports/
```

### CLI Ergonomics

```bash
# Run specific algorithms
./run_matrix --filter "CSAW|FM"

# Run with repetitions for statistics
./run_matrix --repeat 3

# Test hysteresis boundaries
./run_matrix --hysteresis-test

# Generate comparison report
./analyze --format markdown --output report.md

# Quick single test
./run_single CSAW --timbre 16384 --color 16384 --pitch 60
```

### Platform Metadata

Every report includes:
```json
{
  "platform": {
    "os": "macOS",
    "cpu": "Apple M1",
    "ftz_daz": true,
    "fma": false,
    "compiler": "clang-15",
    "buildFlags": "-O2 -fno-fast-math"
  },
  "rng": {
    "seed": 42,
    "sequence": "braids_compatible",
    "verified": true
  },
  "resampler": {
    "latencySamples": 64,
    "reportedToDAW": true,
    "quality": "windowed_sinc"
  },
  "audioHashes": {
    "reference": "sha256:abc123...",
    "candidate": "sha256:def456...",
    "aligned": "sha256:789012..."
  }
}
```

## Success Metrics

### Bit-Accurate Goals (Post-Normalization)
- Wavetable algorithms: ≤1 LSB @ 48kHz
- Digital oscillators: ≤2 LSB @ 48kHz
- FM algorithms: SNR ≥60dB (perceptually transparent)
- Physical models: SNR ≥50dB
- Silence baseline: < -90dB noise floor

### Diagnostic Value Goals
- Every failure links to specific code location
- Deviation type (gain/phase/spectrum) clearly identified
- Normalized vs absolute metrics reported
- Fix recommendation provided

## Debug Recipe Matrix

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| High DC offset | Missing/wrong DC blocker | Check 1-pole HP filter coeff |
| Good SNR, high HF Δmag | Aliasing/BLEP mismatch | Verify MinBLEP tables |
| Growing drift | Sample rate scaling | Check srScale calculation |
| Spikes at param changes | Smoothing mismatch | Verify 24-sample cadence |
| Gain g ≠ 1.0 | Output scaling | Check int16→float conversion |
| Phase offset | Initialization order | Verify reset() sequence |
| Wrong wavetable | Hysteresis logic | Check boundary conditions |
| RNG mismatch | Different sequence | Match Random::GetSample() |
| Sync glitches | Phase reset timing | Document pre/post increment |

## Output Structure

```
braidy-parity/
├── eurorack/           # Local Braids copy (version controlled)
├── references/         # Braids outputs
├── candidates/         
│   ├── direct/        # Direct DSP path
│   └── plugin/        # Full PluginProcessor path
│       ├── pre_resampler/  # 48kHz core output
│       └── post_resampler/ # Final output
├── aligned/           # Sub-sample aligned versions
├── reports/
│   ├── summary.md
│   ├── algorithms/
│   │   └── csaw_analysis.json
│   ├── hysteresis/
│   │   └── wavetable_boundaries.json
│   └── spectrograms/
│       └── csaw_residual.png
└── harness/
    ├── run_matrix.py
    ├── analyze.py
    └── scenes/
        ├── test_scenes.json
        ├── silence_baseline.json
        └── hard_sync_semantics.json
```

## "Done" Definition

✅ Phase 0: Braids reference builds with matching RNG sequence  
✅ Phase 1: CSAW matches at 48kHz within 2 LSB (both paths, post-normalization)  
✅ Phase 2: Diagnostics identify specific divergence points, sync semantics documented  
✅ Phase 3: Sample rates handled correctly with documented tolerances, latency reported  
✅ Phase 4: All 47 algorithms tested with 95% pass rate, hysteresis verified  
✅ Phase 5: CI prevents regressions, one-command testing works

## Braidy-Specific Implementation Notes

1. **Pre-Resampler Access**: Modify PluginProcessor to expose 48kHz core output in harness mode
2. **RNG Matching**: Ensure JUCE uses identical Random::GetSample() distribution with same seed
3. **DSPDispatcher**: Test both with and without dispatcher overhead
4. **BraidyVoice**: Verify voice allocation doesn't affect determinism
5. **Parameter Mapping**: Your ParameterCurves.h curves must match Braids exactly
6. **Settings Resolution**: Document where 7-bit vs 16-bit matters
7. **Hysteresis**: Test wavetable selection boundaries explicitly
8. **Known Issues**: Prioritize KICK, wavetable loading as mentioned in code

This plan provides:
- **Unambiguous implementation** with all clarifications included
- **Immediate validation** of core DSP accuracy with normalized metrics
- **Precise diagnostics** with sub-sample alignment and spectral analysis
- **Complete test coverage** including silence, sustain, sync, and hysteresis
- **Actionable fixes** with clear metrics before/after normalization
- **Regression safety** through automated CI