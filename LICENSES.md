# Third-Party Credits & Licensing Disclosures

Braidy includes third-party code and frameworks. This document records required attributions and licensing notes for distribution.

## Mutable Instruments Braids

- Original project: https://github.com/pichenettes/eurorack/tree/master/braids
- License: GNU General Public License v3.0 (GPLv3)
- Copyright: Emilie Gillet / Mutable Instruments
- Used for:
  - Macro oscillator DSP algorithms
  - Related oscillator resources/data tables

Important compliance note:
- Because Braidy includes GPLv3-licensed Braids code, Braidy distributions that include this derived code must comply with GPLv3 terms.

Clarification:
- Braidy does not include Mutable Instruments Grids code.

## JUCE Framework

- Website: https://juce.com/
- Source: https://github.com/juce-framework/JUCE
- License model: Dual licensing (GPL/AGPL variants and commercial licensing as provided by JUCE)
- Copyright: Raw Material Software Limited
- Used for:
  - Audio plugin runtime (AU/VST3/Standalone)
  - Audio/MIDI infrastructure
  - GUI framework

## Build/Distribution Tooling

- CMake: https://cmake.org/ (BSD-3-Clause)
- GitHub CLI (`gh`): https://cli.github.com/ (MIT)
- Apple Xcode and related notarization/signing tools: Apple developer tooling licenses apply

## Distribution Notes

- Installer/release artifacts include:
  - `LICENSE`
  - `LICENSES.md`
  - `braidy-licenses.html`
- The settings overlay includes acknowledgements links to:
  - Mutable Instruments Braids source
  - JUCE licensing

Last updated: March 3, 2026
