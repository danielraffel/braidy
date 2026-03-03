# Braidy Implementation TODO List
## Missing Features & Fixes Required

Generated: 2025-09-09

---

## 🚨 HIGH PRIORITY - Critical Missing Features

### 1. **INTEGRATE MODULATION SYSTEM INTO AUDIO PROCESSING** ⭐ [RECOMMENDED]
**Status**: Designed but completely disconnected  
**Difficulty**: Medium  
**Time Estimate**: 2-3 hours  

**Current State**:
- ModulationMatrix class fully implemented (`Source/Modulation/ModulationMatrix.h`)
- LFO class fully implemented (`Source/Modulation/LFO.h`)  
- UI overlay designed (`Source/UI/ModulationSettingsOverlay.h`)
- NO connection to audio thread

**Required Changes**:
- Add ModulationMatrix member to PluginProcessor
- Call `modulationMatrix_.processBlock()` in processBlock()
- Apply modulation values to synthesis parameters
- Connect to APVTS for parameter persistence

**Files to Modify**:
- `Source/PluginProcessor.cpp` - Add modulation processing
- `Source/PluginProcessor.h` - Add ModulationMatrix member

---

### 2. **CONNECT LFO PROCESSING TO AUDIO THREAD** ⭐ [RECOMMENDED]
**Status**: LFOs exist but never advance  
**Difficulty**: Easy  
**Time Estimate**: 1 hour  

**Required Implementation**:
```cpp
// In PluginProcessor::processBlock():
double bpm = playHead ? playHead->bpm : 120.0;
modulationMatrix_.processBlock(getSampleRate(), buffer.getNumSamples(), bpm);
```

**Files to Modify**:
- `Source/PluginProcessor.cpp` - Add LFO advancement

---

### 3. **APPLY MODULATION TO SYNTHESIS PARAMETERS**
**Status**: No modulation applied to any parameters  
**Difficulty**: Medium  
**Time Estimate**: 2 hours  

**Required Implementation**:
- Apply modulation to algorithm selection when META mode enabled
- Apply modulation to timbre/color parameters
- Apply modulation to pitch/detune
- Handle bipolar vs unipolar modulation correctly

**Key Code Needed**:
```cpp
// Example for META mode algorithm modulation:
if (metaMode && modulationMatrix_.isModulated(ModulationMatrix::ALGORITHM_SELECTION)) {
    float modValue = modulationMatrix_.getModulation(ModulationMatrix::ALGORITHM_SELECTION);
    int modulatedAlgorithm = modulationMatrix_.applyModulationInt(
        ModulationMatrix::ALGORITHM_SELECTION, 
        currentAlgorithm, 0, 46
    );
    voice->setAlgorithm(modulatedAlgorithm);
}
```

---

### 4. **MAKE MODULATION OVERLAY ACCESSIBLE FROM UI** ⭐ [RECOMMENDED]
**Status**: Settings button exists but doesn't open overlay  
**Difficulty**: Easy  
**Time Estimate**: 30 minutes  

**Required Implementation**:
- Connect Settings button click to show modulation overlay
- Handle overlay visibility toggling
- Ensure proper z-order for overlay

**Files to Modify**:
- `Source/PluginEditor.cpp` - Connect button to overlay

---

## 📦 MEDIUM PRIORITY - Feature Completions

### 5. **IMPLEMENT SETTINGS PERSISTENCE (WAVE MODE)**
**Status**: TODO comment at line 1136  
**Difficulty**: Medium  
**Time Estimate**: 1-2 hours  

**Required Implementation**:
- Save current patch settings to ValueTree
- Implement preset save/load system
- Connect to WAVE menu option

---

### 6. **COMPLETE META MODE ALGORITHM MORPHING**
**Status**: Partially implemented  
**Difficulty**: Medium  
**Time Estimate**: 1 hour  

**Required Implementation**:
- Smooth interpolation between algorithms
- Parameter morphing between algorithm states
- Real-time updates for held notes

---

### 7. **ADD MODULATION PARAMETERS TO APVTS**
**Status**: Missing from parameter layout  
**Difficulty**: Easy  
**Time Estimate**: 1 hour  

**Required Implementation**:
- Add LFO1/2 enable, rate, depth, shape parameters
- Add modulation routing parameters
- Ensure proper save/recall in DAW sessions

---

## 📝 DOCUMENTATION UPDATES

### 8. **UPDATE USER_MANUAL.md WITH MODULATION SECTION**
**Status**: No modulation documentation exists  
**Difficulty**: Easy  
**Time Estimate**: 1 hour  

**Required Sections**:
- LFO Overview
- Modulation Routing
- META Mode Modulation Tutorial
- Example Patches

---

### 9. **CREATE MODULATION QUICK START GUIDE**
**Status**: Not created  
**Difficulty**: Easy  
**Time Estimate**: 30 minutes  

**Content Needed**:
- Step-by-step LFO setup
- Common modulation routings
- META mode algorithm morphing guide

---

## 🔧 LOW PRIORITY - Polish & Enhancements

### 10. **REPLACE STUB IMPLEMENTATIONS**
**Status**: Some methods have placeholder code  
**Difficulty**: Low  
**Time Estimate**: 1 hour  

**Files with Stubs**:
- `Source/PluginEditor_Complete.cpp` - handleEncoderClick/LongPress stubs

---

### 11. **IMPLEMENT ADVANCED MIDI FEATURES**
**Status**: TODO comments at lines 1661, 1667  
**Difficulty**: Low  
**Time Estimate**: 1 hour  

**Features**:
- Enhanced MIDI CC mapping
- MIDI learn functionality
- Velocity curves

---

### 12. **ADD VISUAL FEEDBACK FOR MODULATION**
**Status**: No visual indication of modulation  
**Difficulty**: Medium  
**Time Estimate**: 2 hours  

**Features Needed**:
- Animated parameter indicators
- LFO waveform display
- Modulation amount visualization

---

## 🎯 Implementation Recommendations

### **My Suggested Priority Order**:

1. **#4 - Make modulation overlay accessible** (Quick win - 30 min)
2. **#1 - Integrate modulation system** (Core functionality - 2-3 hrs)
3. **#2 - Connect LFO processing** (Essential for modulation - 1 hr)
4. **#3 - Apply modulation to parameters** (Complete the system - 2 hrs)
5. **#8 - Update documentation** (Help users understand - 1 hr)

**Total Time for Core Modulation**: ~6-7 hours

### **Quick Wins (Under 1 hour each)**:
- #4 - UI overlay connection
- #2 - LFO processing 
- #7 - APVTS parameters
- #9 - Quick start guide

### **Why Prioritize Modulation?**
The modulation system is the biggest missing feature. It's fully designed with sophisticated routing capabilities but completely disconnected. Once connected, Braidy will have:
- 2 independent LFOs
- 22+ modulation destinations
- Full META mode algorithm morphing
- Professional-grade modulation capabilities

---

## 📋 Testing Checklist After Implementation

- [ ] LFO1 can modulate algorithm selection in META mode
- [ ] LFO2 can modulate timbre/color independently  
- [ ] Modulation settings persist in DAW sessions
- [ ] Settings overlay opens and closes properly
- [ ] Visual feedback shows active modulation
- [ ] Documentation covers all modulation features
- [ ] No performance impact from modulation processing

---

## Notes

- The modulation system architecture is excellent and well-designed
- Previous implementation exists in `.old` files for reference
- All infrastructure is in place, just needs reconnection
- This will significantly enhance Braidy's capabilities

---

*Request specific items by number for implementation*