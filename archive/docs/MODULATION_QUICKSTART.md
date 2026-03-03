# Braidy Modulation Quick Start Guide

## 5-Minute Setup for Amazing Sounds

### 1. Basic Vibrato (30 seconds)
```
1. Click MOD button
2. Select LFO 1
3. Set Destination: PITCH
4. Set Rate: 5Hz
5. Set Depth: 10%
Done! Natural vibrato on all notes.
```

### 2. Rhythmic Filter Sweep (1 minute)
```
1. Select CSAW algorithm
2. Click MOD button
3. LFO 1 Settings:
   - Destination: COLOR
   - Shape: SAW DOWN
   - Tempo Sync: ON
   - Rate: 1/4 note
   - Depth: 80%
Perfect for techno basslines!
```

### 3. Evolving Pad Texture (1 minute)
```
1. Select CLOUD algorithm
2. Click MOD button
3. LFO 1:
   - Destination: TIMBRE
   - Shape: SINE
   - Rate: 0.1Hz
   - Depth: 30%
4. LFO 2:
   - Destination: COLOR
   - Shape: TRIANGLE
   - Rate: 0.25Hz
   - Depth: 40%
Instant ambient atmospheres!
```

### 4. Glitch & Bit Crush (30 seconds)
```
1. Any algorithm
2. Click MOD button
3. LFO 1:
   - Destination: BIT_DEPTH
   - Shape: SAMPLE_HOLD
   - Rate: 8Hz
   - Depth: 100%
Lo-fi digital mayhem!
```

### 5. META Mode Magic (2 minutes)
```
1. Long-press encoder → META → ON
2. Click MOD button
3. LFO 1:
   - Destination: ALGORITHM
   - Shape: TRIANGLE
   - Rate: 0.05Hz
   - Depth: 50%
Watch algorithms morph seamlessly!
```

## Pro Tips

### Tempo Sync for Live Performance
- Enable Tempo Sync to lock LFOs to your DAW
- Common musical divisions:
  - 1/16 = Hi-hat patterns
  - 1/8 = Typical trance gate
  - 1/4 = Four-on-floor pump
  - 1/1 = Bar-length sweeps

### LFO Shape Character
- **SINE**: Smooth, natural, organic
- **TRIANGLE**: Linear, predictable sweeps
- **SQUARE**: On/off switching, gates
- **SAW UP/DOWN**: Building/falling tension
- **SAMPLE_HOLD**: Random stepping
- **NOISE**: Chaotic, unpredictable

### Sweet Spot Settings

**Subtle Movement** (barely noticeable but adds life)
- Depth: 5-15%
- Rate: 0.1-1Hz
- Best for: TIMBRE, COLOR

**Obvious Effect** (clearly audible modulation)
- Depth: 30-60%
- Rate: 2-8Hz
- Best for: PITCH, FM_AMOUNT

**Extreme Modulation** (sound design territory)
- Depth: 80-100%
- Rate: 10Hz+
- Best for: BIT_DEPTH, SAMPLE_RATE

## Common Modulation Recipes

### Classic Sounds

**70s Sci-Fi Lead**
```
Algorithm: CSAW
LFO1 → PITCH: Sine, 7Hz, 8%
LFO2 → COLOR: Triangle, 0.5Hz, 40%
```

**Dubstep Wobble Bass**
```
Algorithm: FOLD
LFO1 → COLOR: Sine, Tempo 1/8T, 90%
```

**Ambient Drone**
```
Algorithm: HARM
LFO1 → TIMBRE: Sine, 0.03Hz, 20%
LFO2 → COLOR: Triangle, 0.07Hz, 15%
```

### Modern Techniques

**Sidechain Simulation**
```
LFO1 → VCA: Square, Tempo 1/4, 60%
(Negative modulation for ducking)
```

**FM Growl**
```
Algorithm: FM
LFO1 → FM_AMOUNT: Saw Up, 0.25Hz, 70%
```

**Random Melody Generator**
```
Algorithm: Any melodic
LFO1 → QUANTIZE_ROOT: Sample Hold, 2Hz, 100%
(With scale quantization enabled)
```

## Modulation Matrix Overview

### Quick Access
- **MOD Button**: Instant overlay access
- **No menu diving**: All settings visible
- **Real-time preview**: Hear changes immediately

### Signal Flow
```
LFO → Depth Control → Destination Parameter
     ↓
  Tempo Sync (optional)
```

### Multiple Routings
You can route:
- Same LFO to multiple destinations
- Different LFOs to same destination
- Create complex interactions

## Troubleshooting

**No Modulation Heard?**
- Check LFO is enabled
- Verify depth > 0%
- Confirm destination is audible

**Modulation Too Fast/Slow?**
- Toggle Tempo Sync
- Adjust Rate or Time Division
- Check DAW tempo

**Clicking or Stepping?**
- Use SINE or TRIANGLE for smooth modulation
- Reduce modulation depth
- Slower LFO rates for pitched destinations

## Advanced Tricks

### Pseudo-Polyphonic Effects
Route different LFOs to create the illusion of multiple independent voices:
```
LFO1 → TIMBRE (slow)
LFO2 → COLOR (medium)
Result: Complex, evolving single voice
```

### Self-Patching Simulation
Use fast LFOs as audio-rate modulators:
```
LFO1 → FM_AMOUNT
Rate: 30-50Hz
Depth: 20%
Creates FM-like timbres
```

### Generative Patches
Combine multiple S&H LFOs:
```
LFO1 → ALGORITHM (S&H, slow)
LFO2 → TIMBRE (S&H, medium)
Self-playing, ever-changing patches
```

## Quick Reference Card

### Destinations by Category

**Tone/Timbre**
- TIMBRE, COLOR, ALGORITHM
- FM_AMOUNT, BIT_DEPTH, SAMPLE_RATE

**Pitch/Tuning**
- PITCH, VIBRATO_RATE, VIBRATO_AMOUNT
- QUANTIZE_ROOT, QUANTIZE_SCALE

**Dynamics**
- ATTACK, DECAY, ENVELOPE_AMOUNT
- VCA (overall amplitude)

**Special**
- DRIFT (analog character)
- SIGNATURE (rhythmic patterns)

### Instant Patches

**Need vibrato?** → LFO→PITCH, Sine, 5Hz, 10%
**Need movement?** → LFO→COLOR, Triangle, 1Hz, 40%
**Need rhythm?** → LFO→Any, Square, Tempo Sync, 50%+
**Need chaos?** → LFO→Any, S&H or Noise, Fast, 100%

---

Remember: There are no wrong settings! Experiment freely - you can always reset by turning MOD off and on again.

Happy Modulating!