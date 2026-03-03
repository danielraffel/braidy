# DSP Audio Fix Plan
## Fixing Faint/Broken Audio Output in Braidy Synthesizer

### Executive Summary
The Braidy synthesizer produces extremely faint audio due to severe gain reduction issues in the audio pipeline. The primary cause is an overly aggressive gain smoothing factor (0.001f) that takes ~96ms to reach target amplitude, making all sounds appear faint or inaudible.

### Root Causes Identified

#### 1. **Critical Issue: Gain Smoothing Too Slow**
- **Location**: `Source/BraidyVoice/BraidyVoice.cpp:493`
- **Current**: `smooth_factor = 0.001f` (0.1% per sample)
- **Impact**: Takes 4,600 samples (96ms) to reach 99% of target gain
- **Fix**: Increase to `0.1f` for 10% per sample (46 samples/1ms to 99%)

#### 2. **Potential Issue: Envelope Attack Time**
- **Location**: `Source/BraidyCore/BraidySettings.cpp`
- **Current**: Default ADSR may have slow attack
- **Impact**: Compounds with slow gain smoothing
- **Fix**: Ensure reasonable default attack time (< 10ms)

#### 3. **Amplitude Scaling Chain**
- **Location**: Multiple stages in voice rendering
- **Current**: Multiple gain stages multiply together
- **Impact**: Each stage < 1.0 reduces final output
- **Fix**: Verify each stage outputs appropriate levels

### Implementation Plan

#### Phase 1: Immediate Fixes (Critical)
1. **Fix gain smoothing factor**
   - Edit `BraidyVoice.cpp:493`
   - Change `smooth_factor` from `0.001f` to `0.1f`
   - This alone should restore audible output

2. **Verify envelope defaults**
   - Check `BraidySettings.cpp` for ADSR defaults
   - Ensure attack time is reasonable (5-10ms default)
   - Adjust if necessary

#### Phase 2: Amplitude Verification
1. **Add debug output for gain stages**
   - Log `target_gain` value
   - Log `current_gain_` value
   - Log envelope level
   - Verify values are reaching expected levels

2. **Test each algorithm**
   - Verify CSAW produces proper output
   - Test analog oscillators (SAW, SQR, TRI)
   - Test digital oscillators
   - Ensure all reach reasonable amplitude

#### Phase 3: Fine-tuning
1. **Optimize gain compensation**
   - Review 13/8 gain compensation in analog oscillators
   - Ensure digital algorithms output full range
   - Balance levels between different algorithms

2. **Add output limiter/compressor**
   - Prevent clipping
   - Normalize output levels
   - Ensure consistent amplitude across algorithms

### Testing Protocol

1. **Basic Audio Test**
   ```
   - Launch standalone app
   - Select CSAW algorithm
   - Press middle C
   - Should hear clear, loud sawtooth wave
   ```

2. **Envelope Test**
   ```
   - Trigger note and release quickly
   - Should hear immediate attack
   - Verify proper envelope shape
   ```

3. **Algorithm Comparison**
   ```
   - Test each algorithm at same settings
   - Verify similar output levels
   - Document any outliers
   ```

### Success Criteria
- [ ] Audio output is clearly audible at default settings
- [ ] All algorithms produce proper synth sounds
- [ ] No excessive clicking or popping
- [ ] Envelope response is immediate and natural
- [ ] Output levels are consistent across algorithms

### Files to Modify

1. **Primary Fix**:
   - `Source/BraidyVoice/BraidyVoice.cpp` - Fix gain smoothing

2. **Secondary Checks**:
   - `Source/BraidyCore/BraidySettings.cpp` - Verify envelope defaults
   - `Source/PluginProcessor.cpp` - Verify output scaling

3. **Testing**:
   - Build and test standalone app
   - Test with keyboard input
   - Verify all algorithms

### Timeline
- **Immediate** (5 min): Fix gain smoothing factor
- **Quick** (10 min): Verify and adjust envelope defaults
- **Thorough** (30 min): Test all algorithms and fine-tune

### Notes
The 440Hz test tone confirmed the audio pipeline works. The issue is specifically in the synthesizer's gain staging. The primary fix (gain smoothing) should immediately restore proper audio output.