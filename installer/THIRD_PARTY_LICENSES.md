# Third-Party Software Licenses

This document summarizes third-party licensing for Braidy distribution artifacts.

## Core Third-Party Code

### Mutable Instruments Braids
- **Original Source:** https://github.com/pichenettes/eurorack/tree/master/braids
- **License:** GNU General Public License v3.0 (GPLv3)
- **Copyright:** Emilie Gillet / Mutable Instruments
- **Used for:** Macro oscillator DSP implementation and resources
- **Important:** Braidy does **not** include Mutable Instruments Grids code

### JUCE Framework
- **Website:** https://juce.com/
- **Source:** https://github.com/juce-framework/JUCE
- **License Model:** JUCE dual-license model (GPL/AGPL variants and commercial licensing)
- **Copyright:** Raw Material Software Limited
- **Used for:** Plugin/runtime framework (AU, VST3, Standalone), GUI, audio/MIDI plumbing

## Build and Distribution Tooling

### CMake
- **Website:** https://cmake.org/
- **License:** BSD 3-Clause

### GitHub CLI (`gh`)
- **Website:** https://cli.github.com/
- **License:** MIT

### Apple Tooling
- **Tools:** Xcode, `codesign`, `notarytool`, `stapler`
- **License:** Apple tooling licenses and developer agreements apply

## Compliance Notes

1. Braidy distributions include `LICENSE`, `LICENSES.md`, and `braidy-licenses.html`.
2. Because Braids code is GPLv3, distributed derivatives must satisfy GPLv3 obligations.
3. This project intentionally excludes Visage and Mutable Instruments Grids code.

Last updated: March 3, 2026
