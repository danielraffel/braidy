# Braidy Algorithm Test Report

## Test Methodology
Test each algorithm by:
1. Selecting the algorithm
2. Playing middle C (MIDI note 60)
3. Adjusting both parameters through their range
4. Verifying audio output and characteristic sound

## Algorithm Test Results

### Analog Oscillators (0-7)
- [ ] 0. CSAW - CS-80 Saw
- [ ] 1. MORPH - Morph
- [ ] 2. SAW_SQUARE - Saw Square
- [ ] 3. FOLD - Fold
- [ ] 4. BUZZ - Buzz
- [ ] 5. SQUARE_SUB - Square Sub
- [ ] 6. SAW_SUB - Saw Sub
- [ ] 7. SQUARE_SYNC - Square Sync

### Digital Synthesis (8-11)
- [ ] 8. SAW_SYNC - Saw Sync  
- [ ] 9. TRIPLE_RING_MOD - Triple Ring Mod
- [ ] 10. SAW_SWARM - Saw Swarm
- [ ] 11. COMB - Comb Filter
- [ ] 12. TOY - Toy/Lo-fi

### Digital Filters (13-16)
- [ ] 13. DIGITAL_FILTER_LP - Digital Filter LP
- [ ] 14. DIGITAL_FILTER_PK - Digital Filter Peak
- [ ] 15. DIGITAL_FILTER_BP - Digital Filter BP
- [ ] 16. DIGITAL_FILTER_HP - Digital Filter HP

### Formant/Vowel (17-20)
- [ ] 17. VOSIM - VOSIM
- [ ] 18. VOWEL - Vowel
- [ ] 19. VOWEL_FOF - Vowel FOF
- [ ] 20. HARMONICS - Harmonics

### FM Synthesis (21-23)
- [ ] 21. FM - FM
- [ ] 22. FEEDBACK_FM - Feedback FM
- [ ] 23. CHAOTIC_FEEDBACK_FM - Chaotic FM

### Physical Models - Percussion (24-28)
- [ ] 24. STRUCK_BELL - Struck Bell
- [ ] 25. STRUCK_DRUM - Struck Drum
- [ ] 26. KICK - Kick Drum
- [ ] 27. CYMBAL - Cymbal
- [ ] 28. SNARE - Snare

### Physical Models - Strings/Winds (29-32)
- [ ] 29. PLUCKED - Plucked String
- [ ] 30. BOWED - Bowed String
- [ ] 31. BLOWN - Blown Pipe
- [ ] 32. FLUTED - Fluted

### Wavetables (33-36)
- [ ] 33. WAVETABLES - Wavetables
- [ ] 34. WAVE_MAP - Wave Map
- [ ] 35. WAVE_LINE - Wave Line
- [ ] 36. WAVE_PARAPHONIC - Wave Paraphonic

### Noise/Granular (37-42)
- [ ] 37. FILTERED_NOISE - Filtered Noise
- [ ] 38. TWIN_PEAKS_NOISE - Twin Peaks Noise
- [ ] 39. CLOCKED_NOISE - Clocked Noise
- [ ] 40. GRANULAR_CLOUD - Granular Cloud
- [ ] 41. PARTICLE_NOISE - Particle Noise
- [ ] 42. DIGITAL_MODULATION - Digital Modulation/QPSK

### Additional Algorithms (43-47)
- [ ] 43. STRINGS - String Machine
- [ ] 44. MODAL_RESONATOR - Modal Resonator
- [ ] 45. BASS_DRUM - Analog Bass Drum
- [ ] 46. SNARE_DRUM - Analog Snare
- [ ] 47. HI_HAT - Analog Hi-Hat

## Known Issues from Previous Test
The following algorithms were reported as not producing sound:
- TWLN (Twin Peaks Noise - #38)
- CLDS (Granular Cloud - #40)  
- PART (Particle Noise - #41) - produces just noise
- QPSK (Digital Modulation - #42) - produces garbled audio
- CSAW (CS-80 Saw - #0)
- CYMB (Cymbal - #27)
- KICK (Kick - #26)
- SNAR (Snare - #28)

## Test Focus
Priority testing on the previously broken algorithms to verify fixes.