# Braidy Audio Fix – Technical Approach v2

This v2 refines the plan with a few important corrections and choices to maximize correctness and maintainability while keeping JUCE integration clean.

## Key Corrections vs v1

- Reference sample rate: Braids’ resources and timing assume 48 kHz, not 96 kHz.
  - `lut_oscillator_increments` is generated for 48 kHz (see `resources/lookup_tables.py`).
  - Use a base of 48000 for scaling; remove the 96 kHz decimation assumption.
- Internal sample/blocking: Braids renders in blocks of 24 samples at 48 kHz.
  - Keep 24‑sample micro‑blocks for parameter smoothing and sync accuracy.
- Integer vs float: Avoid blanket “no floats”.
  - Use fixed‑point where the original does (phase, tables, DSP internals).
  - Safely keep float at JUCE I/O edges and for convenience where it does not affect results.
- Parameter ranges: Treat TIMBRE/COLOR as 0..32767 (int16). Pitch uses 7‑bit fractional semitone units (octave is `12 << 7`).
- Global 96 kHz internal processing is not required.
  - Either scale DSP for the host rate or run a fixed 48 kHz core with resampling. See “Sample‑Rate Strategy”.

## Design Goals

- DAW compatibility at 44.1/48/88.2/96/192 kHz.
- Correct Braids timbre/pitch behavior and modulation feel.
- Stable sync and parameter interpolation (24‑sample cadence).
- Efficient CPU usage with predictable performance.

## Sample‑Rate Strategy

Two viable options; default to A for fidelity, keep B as a follow‑up optimization.

- A) Fixed 48 kHz core + resampling (most faithful)
  - Run the DSP core internally at 48 kHz in 24‑sample blocks.
  - Use high‑quality resampling at plugin boundaries when the host is not 48 kHz.
  - Pros: Closest match to hardware behavior (tables, delays, filter curves).
  - Cons: Resampler cost; slightly more plumbing.

- B) Native‑rate core with scaling (more integrated)
  - Keep 24‑sample micro‑blocks but operate at host rate.
  - Scale oscillator increments, filter params, delay lengths by `48000 / hostRate`.
  - Pros: No resampler; straightforward JUCE integration.
  - Cons: Requires careful review per algorithm; some curves are LUT‑baked for 48 kHz.

Recommendation: Start with A to unbreak audio and match reference sound. Add B behind a compile‑time flag once parity tests pass.

## Core Implementation Notes

- Lookup tables
  - Import LUTs from `eurorack/braids/resources.cc` into `BraidyCore/BraidsLookupTables.h`.
  - Keep original integer types and indexing; do not reinterpret into floats.

- Phase increment
  - Base increments are for 48 kHz. For native‑rate mode, scale as:
    - `increment = base_increment * (48000.0 / currentSampleRate)`
  - Use 32‑bit unsigned accumulator with wraparound; preserve original interpolation behavior.

- 24‑sample cadence
  - Maintain parameter interpolation and sync evaluation on the 24‑sample cadence.
  - In JUCE `processBlock`, iterate samples and internally process in 24‑sample chunks (micro‑buffers) for efficiency.

- Sync handling
  - Per‑sample sync detection, propagated into the oscillator core exactly once per sample.

- Integer/float boundaries
  - Internal DSP: fixed‑point as in Braids (phase math, waves, SVF, quantizers, etc.).
  - Plugin edge: convert to/from float for host buffers; keep any float utilities that do not alter DSP numerics.

- Filters and delays
  - Use original SVF and coefficient tables. In native‑rate mode, scale time constants; in fixed‑48k mode, leave as‑is.

## Revised Phases

1) Import + Wire Core DSP (Priority 1)
- Add `BraidyCore/BraidsLookupTables.h` and `BraidyCore/BraidsConstants.h`.
- Embed/port `MacroOscillator`, `DigitalOscillator`, `AnalogOscillator`, SVF, quantizer, and utilities as close to upstream as possible.
- Decide SR strategy (start with fixed 48 kHz core + resampling).

2) Processing Integration (Priority 1)
- JUCE `prepareToPlay`: set `currentSampleRate`, configure resampler for fixed‑48k, or compute `srScale = 48000.0 / currentSampleRate` for native‑rate mode.
- `processBlock`: iterate samples; aggregate into 24‑sample micro‑blocks; update parameters every 24 samples; per‑sample sync.

3) Algorithm Parity Pass (Priority 1)
- Analog: bandlimited shapes, BLEP and parameter scaling parity.
- Digital: integer math paths, DC blocking, sync behavior.
- Wavetables: correct table selection and interpolation logic; hysteresis where present.
- Physical models: trigger path, modal frequencies, envelope behaviors.

4) Parameter System + Mappings
- Internal params as int16 (0..32767) for TIMBRE/COLOR; map JUCE floats accordingly.
- Pitch as Braids pitch units (MIDI<<7, etc.); verify bend and detune mappings.
- Keep original interpolation macros/logic for smoothness.

5) Validation
- Golden checks vs upstream test harness/recordings at 48 kHz.
- Sanity checks across 44.1/96 kHz using fixed‑48k + resampler path.
- Pitch tracking (≥ 8 octaves), parameter response curves, sync edge cases.

## Concrete File Changes (adjusted)

- Create
  - `Source/BraidyCore/BraidsLookupTables.h` (copied/translated from `resources.cc`).
  - `Source/BraidyCore/BraidsConstants.h` (block size, ranges, utility macros).

- Update
  - `Source/BraidyCore/MacroOscillator.cpp` and `DigitalOscillator.cpp`: integrate LUTs, phase math, 24‑sample cadence, sync.
  - `Source/BraidyVoice/VoiceManager.cpp`: render in 24‑sample micro‑blocks.
  - `Source/PluginProcessor.cpp`: configure fixed‑48k core + resampler, or native‑rate scaling.

- Remove/Replace (targeted, not blanket)
  - Runtime `pow/exp` where LUTs exist or fixed‑point alternatives are provided.
  - Redundant float frequency state in the core; keep at the plugin edge.

## Scaling/Conversion Snippets

```cpp
// Phase increment scaling for native‑rate mode
uint32_t ComputePhaseIncrement(int16_t pitch_units, double sampleRate) {
  // pitch_units follows Braids convention; index as in upstream code
  const uint32_t base = lut_oscillator_increments[pitch_index]; // 48 kHz base
  const double scale = 48000.0 / sampleRate;
  return static_cast<uint32_t>(base * scale);
}

// 24‑sample micro‑block processing skeleton
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) {
  const int numSamples = buffer.getNumSamples();
  int processed = 0;
  while (processed < numSamples) {
    const int n = std::min(24, numSamples - processed);
    // 1) Pull per‑sample sync/gates for n samples
    // 2) Update interpolated params at the start of each 24‑sample block
    // 3) Render n samples via core
    processed += n;
  }
}
```

## Success Criteria (unchanged but clarified)

- All algorithms render with characteristic spectra and behavior.
- Pitch tracking accurate across wide range; stable phase behavior.
- Parameter curves and interpolation match Braids feel.
- No clicks/pops; DC offset controlled; CPU cost reasonable for 1–4 voices.
- Fixed‑48k mode null‑tests or matches reference recordings within tolerance.

## Timeline (realistic ranges)

- Import + wiring: 2–4 hours
- Processing integration (SR + 24‑sample loop): 3–5 hours
- Algorithm parity pass: 6–10 hours
- Validation and polish: 2–4 hours

Total: ~13–23 hours depending on SR strategy and parity depth.

## Rationale

Anchoring on the original 48 kHz/24‑sample design eliminates ambiguity about table meanings and timing, making initial audio correctness straightforward. Once parity is achieved, a native‑rate path can be layered on without compromising fidelity.

## Implementation Approach - CLEAN PORT

### Core Strategy
**USE THE ACTUAL BRAIDS CODE** - No reimplementations, no guessing. Import the real algorithms.

### File Organization
```
Source/
├── adapters/          # JUCE-to-Braids bridge (ONLY place we write code)
│   ├── BraidsEngine.h/cpp    # Wraps braids::MacroOscillator
│   ├── BraidsVoice.h/cpp     # JUCE voice using BraidsEngine  
│   └── ModeRegistry.h/cpp    # Maps modes to UI
├── core/              # Existing JUCE plugin code
└── BraidyCore/        # DELETE THIS - it's all wrong reimplementations

eurorack/
├── braids/           # Original code - DO NOT MODIFY
└── stmlib/           # Original code - DO NOT MODIFY
```

### Build System (CMakeLists.txt)
```cmake
# Build Braids as static library
add_library(braids_dsp STATIC
    eurorack/braids/macro_oscillator.cc
    eurorack/braids/analog_oscillator.cc
    eurorack/braids/digital_oscillator.cc
    eurorack/braids/resources.cc
    # NO hardware files, NO UI files
)

target_compile_definitions(braids_dsp PUBLIC
    TEST  # Disables STM32 hardware dependencies
)

# Link to main plugin
target_link_libraries(${PROJECT_NAME} PRIVATE braids_dsp)
```

### Critical Implementation Rules
1. **NEVER modify files in eurorack/**
2. **ONLY write adapter code in Source/adapters/**
3. **Keep Braids' 24-sample processing blocks**
4. **Use Braids' fixed-point math as-is**
5. **Run core at 48kHz, resample at boundaries**

## Appendix: Fidelity Checklist and Rationale

These items ensure an implementation matches Braids’ behavior and help an AI understand what to do and why it matters.

**DSP Numerics**
- **Lookup tables preserved:** Use exact integer tables and widths from upstream; do not re‑quantize or reinterpret as floats. Why: Table values encode tuned response curves (pitch, filters) at 48 kHz; changing representation alters timbre and tuning.
- **Saturating arithmetic:** Replicate clamps/saturation semantics; avoid UB from signed overflows by explicit clamping. Why: Many Braids paths rely on predictable wrap/clip for stability and characteristic harmonics.
- **DC blockers maintained:** Keep per‑algorithm DC removal and its state. Why: Prevents bias/offset that causes headroom loss, clicks, and mismatched spectra.
- **Deterministic randomness:** Match `Random::GetSample` distribution and seeding. Why: Noise/physical models depend on specific statistics; different RNGs change tone and dynamics.

**Timing & Cadence**
- **24‑sample interpolation cadence:** Persist parameter/sync interpolation state across blocks and voices. Why: Smoothing and modulation depths are designed around this cadence.
- **Sample‑accurate MIDI:** Apply MIDI events at exact sample offsets; split micro‑blocks when events fall mid‑block. Why: Onsets, sync, and transient models need precise timing to avoid flamming and phase errors.
- **Per‑sample sync/gate:** Feed sync into the core each sample as upstream does. Why: Hard‑sync and FM algorithms depend on single‑sample edge detection.

**Sample Rate & I/O**
- **Fixed‑48k core with proper resampler (if chosen):** Use high‑quality linear‑phase resampling and call `setLatencySamples` with its exact latency. Why: Preserves original timing/curves and keeps DAW PDC correct.
- **Amplitude/polarity mapping:** Convert Braids int16 centered audio to host float −1..1 with matching gain and sign. Why: Prevents perceived loudness/tone differences and avoids unintended clipping.
- **Stereo output policy:** Duplicate mono core to both channels unless a deliberate stereo algorithm is added. Why: Avoids accidental inter‑channel differences from uninitialized or divergent states.

**Parameters & Mappings**
- **0..32767 param mapping:** Map JUCE 0..1 to int16 with correct rounding; retain hysteresis/quantization (e.g., wavetable selection). Why: Many modes rely on stepped/latched behavior for stable sound selection.
- **Pitch units and reference:** Convert MIDI to Braids pitch units (MIDI<<7), include bend/detune, and confirm A4 reference. Why: Ensures accurate pitch tracking and chord relationships.

**Build/Runtime**
- **Denormal protection:** Guard inner loops (e.g., `juce::ScopedNoDenormals`). Why: Prevents denormal slowdowns and unexpected noise behavior at very low levels.
- **Compiler flags sanity:** Avoid fast‑math that changes integer/fixed‑point semantics; keep consistent behavior across platforms. Why: Numeric assumptions in the DSP depend on strict evaluation and rounding.

**Validation**
- **Golden renders at 48 kHz:** Render known test scenes (e.g., upstream test harness) and null/AB against the plugin in fixed‑48k mode before resampling. Why: Detects subtle spectral and envelope differences early.
- **Edge‑case sweeps:** Verify extremes of pitch, TIMBRE/COLOR, and sync‑heavy modes. Why: Bugs hide at limits (overflow, aliasing, envelope corner cases).
