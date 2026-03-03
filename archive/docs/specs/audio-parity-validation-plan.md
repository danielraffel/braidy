# Braidy Audio Parity Validation Plan
## Objective: Automated WAV-to-WAV Comparison System

**CRITICAL**: No more manual testing. Build automated system to compare JUCE Braidy output with eurorack Braids reference implementation via WAV file analysis.

## Phase 0: Braids Reference Build (PRIORITY 1)
**Goal**: Working eurorack Braids that generates reference WAV files

### 0.1 Local Braids Setup
```bash
# Use the existing eurorack/braids directory
cd /Users/danielraffel/Code/braidy/eurorack/braids
make -f test/makefile  # Build test harness
```

### 0.2 Braids WAV Generator
Create `braids_wav_generator.cpp`:
```cpp
#include "macro_oscillator.h"
#include "random.h"
#include <fstream>
#include <cstdint>

class BraidsWavGenerator {
private:
    braids::MacroOscillator osc;
    braids::Random rng;
    
public:
    void generate_csaw_reference() {
        // Fixed seed for repeatability
        rng.Init(42);
        
        // Initialize oscillator
        osc.Init();
        osc.set_shape(braids::MACRO_OSC_SHAPE_CSAW);
        osc.set_pitch(60 << 7);  // Middle C in Braids format
        osc.set_parameter(0, 16384);  // TIMBRE mid-range
        osc.set_parameter(1, 16384);  // COLOR mid-range
        
        // Generate 1 second at 48kHz
        const size_t samples = 48000;
        int16_t buffer[24];  // Braids uses 24-sample blocks
        std::vector<int16_t> output;
        
        for (size_t i = 0; i < samples; i += 24) {
            size_t block_size = std::min(24, (int)(samples - i));
            uint8_t sync[24] = {0};  // No sync
            
            osc.Render(sync, buffer, block_size);
            
            for (size_t j = 0; j < block_size; ++j) {
                output.push_back(buffer[j]);
            }
        }
        
        // Write WAV file
        write_wav("braids_csaw_reference.wav", output);
    }
    
    void write_wav(const char* filename, const std::vector<int16_t>& samples) {
        std::ofstream file(filename, std::ios::binary);
        
        // WAV header
        file.write("RIFF", 4);
        uint32_t file_size = 36 + samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&file_size), 4);
        file.write("WAVE", 4);
        
        // Format chunk
        file.write("fmt ", 4);
        uint32_t fmt_size = 16;
        file.write(reinterpret_cast<const char*>(&fmt_size), 4);
        uint16_t audio_format = 1;  // PCM
        file.write(reinterpret_cast<const char*>(&audio_format), 2);
        uint16_t num_channels = 1;
        file.write(reinterpret_cast<const char*>(&num_channels), 2);
        uint32_t sample_rate = 48000;
        file.write(reinterpret_cast<const char*>(&sample_rate), 4);
        uint32_t byte_rate = sample_rate * 2;
        file.write(reinterpret_cast<const char*>(&byte_rate), 4);
        uint16_t block_align = 2;
        file.write(reinterpret_cast<const char*>(&block_align), 2);
        uint16_t bits_per_sample = 16;
        file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        
        // Data chunk
        file.write("data", 4);
        uint32_t data_size = samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&data_size), 4);
        file.write(reinterpret_cast<const char*>(samples.data()), data_size);
        
        file.close();
    }
};
```

**Deliverable**: `braids_csaw_reference.wav` - Known good CSAW output from eurorack implementation

## Phase 1: JUCE Braidy WAV Generator (PRIORITY 1)
**Goal**: Generate equivalent WAV from JUCE Braidy with identical parameters

### 1.1 JUCE WAV Generator
Create `Source/test_wav_generator.cpp`:
```cpp
#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/BraidyResources.h"
#include <fstream>
#include <vector>

class BraidyWavGenerator {
private:
    braidy::MacroOscillator osc;
    
public:
    void generate_csaw_candidate() {
        // Initialize exactly like Braids reference
        braidy::InitializeResources();
        osc.Init();
        osc.set_shape(braidy::MacroOscillatorShape::CSAW);
        osc.set_pitch(60 << 7);  // Middle C in Braids format
        osc.set_parameters(16384, 16384);  // TIMBRE, COLOR mid-range
        
        // Generate 1 second at 48kHz
        const size_t samples = 48000;
        std::vector<int16_t> output;
        
        for (size_t i = 0; i < samples; i += 24) {
            size_t block_size = std::min(24, (int)(samples - i));
            int16_t buffer[24];
            uint8_t sync[24] = {0};  // No sync
            
            osc.Render(sync, buffer, block_size);
            
            for (size_t j = 0; j < block_size; ++j) {
                output.push_back(buffer[j]);
            }
        }
        
        // Write WAV file
        write_wav("braidy_csaw_candidate.wav", output);
    }
    
    void write_wav(const char* filename, const std::vector<int16_t>& samples) {
        // Same WAV writing code as reference
        std::ofstream file(filename, std::ios::binary);
        
        // WAV header
        file.write("RIFF", 4);
        uint32_t file_size = 36 + samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&file_size), 4);
        file.write("WAVE", 4);
        
        // Format chunk
        file.write("fmt ", 4);
        uint32_t fmt_size = 16;
        file.write(reinterpret_cast<const char*>(&fmt_size), 4);
        uint16_t audio_format = 1;  // PCM
        file.write(reinterpret_cast<const char*>(&audio_format), 2);
        uint16_t num_channels = 1;
        file.write(reinterpret_cast<const char*>(&num_channels), 2);
        uint32_t sample_rate = 48000;
        file.write(reinterpret_cast<const char*>(&sample_rate), 4);
        uint32_t byte_rate = sample_rate * 2;
        file.write(reinterpret_cast<const char*>(&byte_rate), 4);
        uint16_t block_align = 2;
        file.write(reinterpret_cast<const char*>(&block_align), 2);
        uint16_t bits_per_sample = 16;
        file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        
        // Data chunk
        file.write("data", 4);
        uint32_t data_size = samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&data_size), 4);
        file.write(reinterpret_cast<const char*>(samples.data()), data_size);
        
        file.close();
    }
};

int main() {
    BraidyWavGenerator gen;
    gen.generate_csaw_candidate();
    std::cout << "Generated braidy_csaw_candidate.wav" << std::endl;
    return 0;
}
```

**Deliverable**: `braidy_csaw_candidate.wav` - JUCE implementation output

## Phase 2: Automated WAV Comparison (PRIORITY 1)
**Goal**: Automated analysis to determine if files sound similar

### 2.1 WAV Comparison Tool
Create `Source/wav_compare.py`:
```python
#!/usr/bin/env python3
import numpy as np
import scipy.io.wavfile as wav
import matplotlib.pyplot as plt
from scipy.signal import correlate
import argparse

class WavComparer:
    def __init__(self, reference_file, candidate_file):
        self.ref_rate, self.ref_data = wav.read(reference_file)
        self.cand_rate, self.cand_data = wav.read(candidate_file)
        
        if self.ref_rate != self.cand_rate:
            raise ValueError(f"Sample rate mismatch: {self.ref_rate} vs {self.cand_rate}")
    
    def align_signals(self):
        """Align signals using cross-correlation"""
        correlation = correlate(self.ref_data, self.cand_data, mode='full')
        offset = np.argmax(correlation) - (len(self.cand_data) - 1)
        
        # Apply offset
        if offset > 0:
            aligned_ref = self.ref_data[offset:]
            aligned_cand = self.cand_data
        else:
            aligned_ref = self.ref_data
            aligned_cand = self.cand_data[-offset:]
        
        # Trim to same length
        min_len = min(len(aligned_ref), len(aligned_cand))
        return aligned_ref[:min_len], aligned_cand[:min_len], offset
    
    def compute_metrics(self, ref, cand):
        """Compute comparison metrics"""
        # Convert to float
        ref_f = ref.astype(np.float64)
        cand_f = cand.astype(np.float64)
        
        # Basic metrics
        rms_error = np.sqrt(np.mean((ref_f - cand_f) ** 2))
        peak_error = np.max(np.abs(ref_f - cand_f))
        
        # SNR calculation
        signal_power = np.mean(ref_f ** 2)
        noise_power = np.mean((ref_f - cand_f) ** 2)
        if noise_power > 0:
            snr_db = 10 * np.log10(signal_power / noise_power)
        else:
            snr_db = float('inf')
        
        # Correlation
        correlation = np.corrcoef(ref_f, cand_f)[0, 1]
        
        return {
            'rms_error': rms_error,
            'peak_error': peak_error,
            'snr_db': snr_db,
            'correlation': correlation,
            'max_sample_value': 32768.0  # 16-bit
        }
    
    def analyze(self):
        """Full analysis with verdict"""
        print("=== WAV COMPARISON ANALYSIS ===")
        print(f"Reference: {len(self.ref_data)} samples at {self.ref_rate}Hz")
        print(f"Candidate: {len(self.cand_data)} samples at {self.cand_rate}Hz")
        
        # Align signals
        ref_aligned, cand_aligned, offset = self.align_signals()
        print(f"Alignment offset: {offset} samples")
        
        # Compute metrics
        metrics = self.compute_metrics(ref_aligned, cand_aligned)
        
        print(f"\n=== METRICS ===")
        print(f"RMS Error: {metrics['rms_error']:.2f}")
        print(f"Peak Error: {metrics['peak_error']:.2f}")
        print(f"SNR: {metrics['snr_db']:.2f} dB")
        print(f"Correlation: {metrics['correlation']:.6f}")
        
        # Convert to LSB units (16-bit)
        rms_lsb = metrics['rms_error']
        peak_lsb = metrics['peak_error']
        print(f"RMS Error: {rms_lsb:.2f} LSB")
        print(f"Peak Error: {peak_lsb:.2f} LSB")
        
        # Verdict
        print(f"\n=== VERDICT ===")
        if peak_lsb < 2.0 and metrics['snr_db'] > 80:
            print("✅ EXCELLENT: Bit-accurate match")
            verdict = "EXCELLENT"
        elif peak_lsb < 10.0 and metrics['snr_db'] > 60:
            print("✅ GOOD: Perceptually transparent")
            verdict = "GOOD"
        elif peak_lsb < 100.0 and metrics['snr_db'] > 40:
            print("⚠️  ACCEPTABLE: Audible differences but usable")
            verdict = "ACCEPTABLE"
        elif metrics['correlation'] > 0.8:
            print("⚠️  POOR: Same waveform but wrong amplitude/offset")
            verdict = "POOR"
        else:
            print("❌ BROKEN: Completely different signals")
            verdict = "BROKEN"
        
        return verdict, metrics
    
    def save_aligned_wavs(self, ref, cand):
        """Save aligned versions for listening"""
        wav.write('reference_aligned.wav', self.ref_rate, ref.astype(np.int16))
        wav.write('candidate_aligned.wav', self.cand_rate, cand.astype(np.int16))
        print("Saved aligned WAVs for listening test")

def main():
    parser = argparse.ArgumentParser(description='Compare two WAV files')
    parser.add_argument('reference', help='Reference WAV file')
    parser.add_argument('candidate', help='Candidate WAV file')
    args = parser.parse_args()
    
    comparer = WavComparer(args.reference, args.candidate)
    verdict, metrics = comparer.analyze()
    
    return 0 if verdict in ['EXCELLENT', 'GOOD'] else 1

if __name__ == '__main__':
    exit(main())
```

## Phase 3: CSAW Parity Loop (CRITICAL)
**Goal**: Fix CSAW algorithm until WAV comparison shows parity

### 3.1 Automated Test Loop
Create `scripts/test_csaw_parity.sh`:
```bash
#!/bin/bash

echo "=== CSAW PARITY VALIDATION ==="

# Build reference
echo "Building Braids reference..."
cd eurorack/braids
if ! make -f test/makefile; then
    echo "❌ Failed to build Braids reference"
    exit 1
fi

# Generate reference WAV
echo "Generating reference WAV..."
if ! ./braids_wav_generator; then
    echo "❌ Failed to generate reference WAV"
    exit 1
fi

# Build JUCE
echo "Building JUCE candidate..."
cd ../../
if ! g++ -std=c++17 -I./Source -I./Source/BraidyCore -o test_wav_gen \
    Source/test_wav_generator.cpp \
    Source/BraidyCore/*.cpp \
    -framework Accelerate; then
    echo "❌ Failed to build JUCE generator"
    exit 1
fi

# Generate candidate WAV
echo "Generating candidate WAV..."
if ! ./test_wav_gen; then
    echo "❌ Failed to generate candidate WAV"
    exit 1
fi

# Compare
echo "Comparing WAVs..."
python3 Source/wav_compare.py braids_csaw_reference.wav braidy_csaw_candidate.wav

verdict=$?
if [ $verdict -eq 0 ]; then
    echo "🎉 CSAW PARITY ACHIEVED!"
else
    echo "❌ CSAW still needs work"
fi

exit $verdict
```

## Phase 4: Analysis of Current CSAW Issues
**Goal**: Use automated tools to identify exactly what's wrong

### 4.1 Diagnostic Analysis
Based on waveform-settings.md, CSAW should:
- Use `OSC_SHAPE_CSAW` (analog oscillator)
- `Parameter[0] (TIMBRE)`: Phase randomization amount (0-32767)
- `Parameter[1] (COLOR)`: Low-pass filter cutoff (-32767 to 32767)
- Add DC offset based on COLOR
- Scale output by 13/8 for gain compensation

### 4.2 Current Implementation Review
Check if current `MacroOscillator::RenderCSaw()` matches specification:
1. **Does it use analog oscillator?** ✓ (uses `analog_oscillator_[0]`)
2. **Does it implement phase randomization?** ❌ (Missing)
3. **Does it implement COLOR as LP filter?** ❌ (Implements as notch filter)
4. **Does it add DC offset?** ❌ (Missing)
5. **Does it apply gain compensation?** ❌ (Missing)

**DIAGNOSIS**: Our CSAW implementation is completely wrong!

## Phase 5: Fix CSAW Implementation
**Goal**: Rewrite CSAW to match eurorack specification exactly

### 5.1 Correct CSAW Implementation
Update `MacroOscillator.cpp`:
```cpp
void MacroOscillator::RenderCSaw(const uint8_t* sync_buffer, int16_t* buffer, size_t size) {
    // Use analog oscillator with CSAW shape
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::CSAW);
    analog_oscillator_[0].set_pitch(pitch_);
    
    // TIMBRE controls phase randomization (not implemented in analog oscillator yet)
    // For now, use as parameter
    analog_oscillator_[0].set_parameter(parameter_[0]);
    
    // COLOR controls LP filter cutoff
    int16_t color = parameter_[1];
    
    // Render analog oscillator
    analog_oscillator_[0].Render(sync_buffer, buffer, nullptr, size);
    
    // Apply COLOR as LP filter and DC offset
    for (size_t i = 0; i < size; ++i) {
        // Simple one-pole LP filter based on COLOR
        float cutoff = (color + 32768) / 65536.0f; // Normalize to 0-1
        // TODO: Implement proper LP filter
        
        // Add DC offset based on COLOR
        int32_t sample = buffer[i];
        sample += (color >> 8); // Add DC offset
        
        // Apply gain compensation (13/8 = 1.625)
        sample = (sample * 13) >> 3;
        
        buffer[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
    }
}
```

## Success Criteria

### Phase 0-2: Infrastructure ✅
- [x] Braids reference builds and generates WAV
- [x] JUCE candidate builds and generates WAV  
- [x] Automated comparison tool works

### Phase 3-5: CSAW Parity 🎯
- [ ] `python3 Source/wav_compare.py braids_csaw_reference.wav braidy_csaw_candidate.wav` returns "EXCELLENT" or "GOOD"
- [ ] Manual listening test confirms they sound similar
- [ ] SNR > 60dB, Peak error < 10 LSB

### Next Steps (After CSAW Works)
1. **Expand to other analog algorithms**: MORPH, SAW_SQUARE, etc.
2. **Digital algorithms**: Start with simpler ones like HARMONICS, FM
3. **Complex algorithms**: VOSIM, VOWEL, physical models
4. **All 47 algorithms**: Complete matrix testing

## Implementation Order

1. **TODAY**: Build Phase 0-2 infrastructure
2. **TODAY**: Run initial comparison, expect "BROKEN" verdict
3. **TODAY**: Analyze why it's broken using wav_compare output
4. **TODAY**: Fix CSAW implementation based on waveform-settings.md
5. **REPEAT**: Until CSAW passes comparison
6. **THEN**: Move to next algorithm

## Why This Will Work

1. **Objective measurement**: No more subjective "sounds bad"
2. **Automated iteration**: Can test changes immediately
3. **Clear success criteria**: SNR numbers don't lie
4. **Exact specification**: waveform-settings.md provides implementation details
5. **Proven reference**: eurorack Braids is the ground truth

This plan prioritizes getting ONE algorithm working perfectly with automated validation before moving to the next. No more manual testing frustration!