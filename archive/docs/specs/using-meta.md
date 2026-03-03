# Using Meta Mode in Braidy

Meta mode is a special feature that allows real-time algorithm selection using the FM parameter, exactly matching the behavior of the original Mutable Instruments Braids hardware module.

## What is Meta Mode?

When Meta mode is enabled, the FM knob/parameter (normally used for frequency modulation) switches its function to control algorithm selection instead. This provides dynamic, real-time morphing between the 47 different synthesis algorithms available in Braids.

This matches the exact hardware behavior where Meta modulation uses ADC channel 3 (the FM CV input) to control algorithm selection.

## How to Enable Meta Mode

### In the Plugin UI:
1. Click the EDIT encoder to enter menu mode
2. Use the EDIT encoder to navigate to "META"  
3. Click the EDIT encoder to toggle Meta mode ON/OFF
4. The display will show "On" when Meta mode is enabled

### Via Host Automation:
- Use the "Meta Mode" parameter in your DAW
- Enable/disable via automation or MIDI CC control

## How Meta Mode Works

### When Meta Mode is OFF (Normal Operation):
- **TIMBRE knob**: Controls parameter 1 (varies by algorithm)
- **COLOR knob**: Controls parameter 2 (varies by algorithm)  
- **FM knob**: Controls FM depth/amount for frequency modulation

### When Meta Mode is ON:
- **TIMBRE knob**: Still controls parameter 1 (varies by algorithm)
- **COLOR knob**: Still controls parameter 2 (varies by algorithm)
- **FM knob**: Controls algorithm selection (0-46, covering all 47 algorithms)

## Using Meta Mode

### Manual Control:
1. Enable Meta mode as described above
2. Turn the **FM knob** to select different algorithms in real-time
3. Watch the OLED display change to show the current algorithm name as you turn the FM knob
4. The algorithm changes smoothly as you adjust the FM parameter

### Automated Control:
1. Enable Meta mode
2. Automate the "FM Amount" parameter in your DAW
3. Use LFOs, envelopes, or manual automation to create dynamic algorithm changes
4. Each position of the FM parameter corresponds to a different algorithm:
   - FM = 0% → CSAW (Algorithm 0)
   - FM = 50% → Algorithm 23
   - FM = 100% → DIGI (Algorithm 46)

### With Modulation:
1. Enable Meta mode
2. In the modulation settings, route an LFO to the FM parameter
3. The algorithm will cycle through different synthesis models based on the LFO wave
4. Use different LFO shapes for different algorithm selection patterns:
   - **Sine wave**: Smooth morphing between algorithms
   - **Triangle wave**: Linear sweep through algorithms
   - **Square wave**: Hard switching between two sets of algorithms
   - **Random wave**: Unpredictable algorithm jumps

## Visual Feedback

### OLED Display:
- Shows the currently selected algorithm name (4-character abbreviation)
- Updates in real-time as you adjust the FM parameter in Meta mode
- Examples: "CSAW", "MORPH", "BUZZ", "VOWL", "DIGI"

### Future Enhancement (LED Feedback):
The hardware Braids module shows algorithm changes via LED brightness. This visual feedback for Meta mode algorithm switching will be added in a future update to match the exact hardware behavior.

## Algorithm Range

Meta mode provides access to all algorithms available for Meta modulation (0-46):
- Algorithms 0-46: All synthesis models accessible via Meta mode
- This matches the hardware's `MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META` limit

## Tips and Creative Uses

### Morphing Pads:
1. Enable Meta mode
2. Use a slow sine wave LFO on the FM parameter
3. Set LFO depth to taste for smooth algorithm morphing
4. Perfect for evolving ambient textures

### Rhythmic Algorithm Changes:
1. Enable Meta mode  
2. Use a square wave LFO synced to tempo on FM parameter
3. Creates rhythmic changes between synthesis models
4. Great for drum programming and rhythmic synthesis

### Performance Control:
1. Enable Meta mode
2. Map the FM parameter to a MIDI controller knob or expression pedal
3. Perform live algorithm changes during playback
4. Each algorithm has its own sonic character and parameter behavior

### Sequence-Driven Morphing:
1. Enable Meta mode
2. Automate the FM parameter with step sequences or complex envelopes
3. Create evolving sequences where each note uses a different synthesis algorithm
4. Combine with other parameter automation for complex evolving sounds

## Troubleshooting

**Q: Meta mode isn't working - FM knob still affects pitch**
A: Make sure Meta mode is properly enabled in the menu settings. The display should show "META On".

**Q: Algorithm changes cause audio glitches**
A: This is normal behavior - algorithm changes reinitialize the oscillator. The hardware behaves the same way.

**Q: Some algorithms sound different when accessed via Meta mode**
A: All algorithms should sound identical whether accessed via the main algorithm selector or Meta mode. If you notice differences, this may indicate a bug.

**Q: LED feedback isn't showing algorithm changes**
A: LED feedback for Meta mode algorithm switching is planned for a future update to match hardware behavior.

## Hardware Correspondence

This implementation exactly matches the original Mutable Instruments Braids hardware behavior:

- **Hardware ADC Channel 0**: TIMBRE parameter → Plugin param1
- **Hardware ADC Channel 1**: COLOR parameter → Plugin param2  
- **Hardware ADC Channel 2**: V/OCT pitch → Plugin pitch tracking
- **Hardware ADC Channel 3**: FM/Meta parameter → Plugin FM Amount

When `settings.meta_modulation()` is true in the hardware, ADC channel 3 controls algorithm selection instead of FM depth, exactly as implemented in this plugin.

## Technical Implementation Notes

- Meta mode uses the exact same algorithm selection logic as the hardware
- Algorithm changes are safe and handle voice management properly
- Parameter mapping is identical to hardware: Timbre=ADC0, Color=ADC1, FM/Meta=ADC3
- All 47 algorithms are accessible, matching hardware limitations for Meta mode (0-46)