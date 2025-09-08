# Braids Test Harness

A comprehensive test harness for comparing the original Mutable Instruments Braids MacroOscillator implementation with the JUCE wrapper implementation.

## Overview

This test harness provides:

- **BraidsReference**: Direct wrapper for the original Braids MacroOscillator
- **AudioComparator**: Comprehensive audio analysis and comparison metrics
- **ModeRegistry**: Mapping between algorithm names and Braids shape enums
- **TestRunner**: Main orchestrator for running tests and generating reports

## Key Features

- **Deterministic Testing**: Uses fixed random seeds for reproducible results
- **Comprehensive Metrics**: RMS error, SNR, correlation, spectral analysis
- **Multiple Test Modes**: Single tests, batch tests, reference generation
- **Standard Parameters**: 48kHz sample rate, 24-sample blocks (Braids standard)
- **Report Generation**: Text, CSV, and JSON output formats

## Quick Start

### Build the Example

```bash
cd testing/harness
make all
```

### Run the Example

```bash
./test_harness_example
```

This will run several example tests demonstrating different usage patterns.

## Usage Examples

### Single Test

```cpp
#include "TestRunner.h"

TestRunner testRunner(48000.0f, 24);
testRunner.initialize(42);

TestConfig config;
config.algorithmName = "CSAW";
config.frequency = 440.0f;
config.timbre = 0.5f;
config.color = 0.3f;
config.duration = 1.0f;

TestResult result = testRunner.runSingleTest(config);
std::cout << AudioComparator::generateReport(result.metrics) << std::endl;
```

### Batch Testing

```cpp
BatchTestConfig batchConfig;
batchConfig.algorithms = {"CSAW", "MORPH", "BUZZ"};
batchConfig.frequencies = {220.0f, 440.0f, 880.0f};
batchConfig.timbreValues = {0.0f, 0.5f, 1.0f};
batchConfig.duration = 0.5f;

std::vector<TestResult> results = testRunner.runBatchTests(batchConfig);
testRunner.saveResults(results, "batch_results.csv", "csv");
```

### Reference Generation

```cpp
TestConfig config;
config.algorithmName = "MORPH";
config.frequency = 440.0f;
config.duration = 2.0f;

std::vector<float> referenceAudio(96000); // 2 seconds at 48kHz
int generated = testRunner.generateReference(config, referenceAudio.data(), 96000);
```

## Components

### ModeRegistry

Maps algorithm names to Braids MacroOscillatorShape enum values.

- Supports all 47 Braids algorithms
- Case-insensitive name lookup
- Bidirectional mapping (name ↔ shape)

### BraidsReference

Direct wrapper for the original Braids MacroOscillator.

- Uses deterministic RNG for reproducible output
- Matches Braids parameter ranges and behavior
- Supports all oscillator shapes and parameters

### AudioComparator

Comprehensive audio analysis with metrics including:

- **Error Metrics**: RMS error, peak error, mean error
- **Quality Metrics**: SNR, THD, SINAD
- **Level Analysis**: RMS levels, DC offset, level differences
- **Correlation**: Cross-correlation coefficient
- **Spectral Analysis**: Centroid, rolloff (requires FFT implementation)

### TestRunner

Main orchestrator providing:

- **Single Tests**: Compare reference vs. JUCE wrapper
- **Batch Tests**: Systematic parameter sweeps
- **Report Generation**: Multiple output formats
- **Statistics**: Pass rates, performance analysis

## Quality Thresholds

Default quality thresholds for pass/fail determination:

```cpp
QualityThresholds thresholds;
thresholds.maxRmsError = 0.01;      // 1% RMS error
thresholds.maxPeakError = 0.1;      // 10% peak error
thresholds.minSnr = 60.0;           // 60 dB SNR
thresholds.maxThd = 1.0;            // 1% THD
thresholds.maxDcDifference = 0.001; // 0.1% DC difference
thresholds.maxLevelDiff = 0.5;      // 0.5 dB level difference
thresholds.minCorrelation = 0.95;   // 95% correlation
```

## Algorithm Coverage

The test harness supports all 47 Braids algorithms:

**Additive Synthesis:**
- CSAW, MORPH, SAW_SQUARE, SINE_TRIANGLE, BUZZ
- SQUARE_SUB, SAW_SUB, SQUARE_SYNC, SAW_SYNC
- TRIPLE_SAW, TRIPLE_SQUARE, TRIPLE_TRIANGLE, TRIPLE_SINE
- TRIPLE_RING_MOD, SAW_SWARM, SAW_COMB, TOY

**Digital Modulation:**
- DIGITAL_FILTER_LP, DIGITAL_FILTER_PK, DIGITAL_FILTER_BP, DIGITAL_FILTER_HP
- VOSIM, VOWEL, VOWEL_FOF

**FM Synthesis:**
- HARMONICS, FM, FEEDBACK_FM, CHAOTIC_FEEDBACK_FM
- PLUCKED, BOWED, BLOWN, FLUTED
- STRUCK_BELL, STRUCK_DRUM, KICK, CYMBAL, SNARE

**Wavetable:**
- WAVETABLES, WAVE_MAP, WAVE_LINE, WAVE_PARAPHONIC

**Speech/Noise:**
- FILTERED_NOISE, TWIN_PEAKS_NOISE, CLOCKED_NOISE
- GRANULAR_CLOUD, PARTICLE_NOISE

**Additional:**
- DIGITAL_MODULATION, QUESTION_MARK

## Output Formats

### Text Report
Detailed human-readable analysis with metrics and quality assessment.

### CSV Report
Machine-readable format suitable for analysis in spreadsheet applications or Python/R.

### JSON Report
Structured format for programmatic analysis and integration with other tools.

## Integration

To integrate with your JUCE project:

1. Include the harness headers in your test project
2. Link against your BraidyCore and BraidyVoice implementations
3. Ensure your BraidyVoice class provides the expected interface:
   - `setAlgorithm(const std::string& name)`
   - `setFrequency(float freq)`
   - `setTimbre(float timbre)`
   - `setColor(float color)`
   - `setAux(float aux)`
   - `renderNextBlock(float* buffer, int numSamples)`

## Building with CMake

To integrate into a CMake project:

```cmake
# Add test harness as a library
add_library(BraidsTestHarness
    testing/harness/ModeRegistry.cpp
    testing/harness/BraidsReference.cpp
    testing/harness/AudioComparator.cpp
    testing/harness/TestRunner.cpp
)

target_include_directories(BraidsTestHarness PUBLIC testing/harness)
target_link_libraries(BraidsTestHarness BraidyCore BraidyVoice)

# Create test executable
add_executable(BraidsTests testing/harness/example_usage.cpp)
target_link_libraries(BraidsTests BraidsTestHarness)
```

## Notes

- The test harness expects a 48kHz sample rate for optimal Braids compatibility
- Block size of 24 samples matches the original Braids processing
- Deterministic RNG ensures reproducible test results
- Some spectral analysis features require FFT implementation (placeholders provided)
- Tests are designed to be fast enough for continuous integration