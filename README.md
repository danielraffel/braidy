# Braidy

Braidy is a macOS software instrument based on Mutable Instruments' **Braids** macro oscillator engine.

It builds as:
- AU (Audio Unit)
- VST3
- Standalone app

This repository targets Braids code (not Grids code) and JUCE.

## Features

- Braids oscillator models adapted for desktop plugin use
- MIDI-input synth workflow for DAWs and standalone app use
- AU/VST3/Standalone outputs from one CMake project
- Built-in modulation UI and performance controls
- Automated version bumping + signing/notarization/release packaging pipeline

## Requirements

- macOS
- Xcode + Command Line Tools
- CMake
- Apple Developer certificates (for signing/notarization/publish)
- GitHub CLI (`gh`) authenticated (for release publishing)

## Build

From project root:

```bash
# AU only
./scripts/build.sh au local

# All formats (AU + VST3 + Standalone)
./scripts/build.sh all local
```

Artifacts/install locations:

- AU: `~/Library/Audio/Plug-Ins/Components/Braidy.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Braidy.vst3`
- Standalone: `build-release/Braidy_artefacts/Release/Standalone/Braidy.app`

## Publish Release

```bash
./scripts/build.sh all publish
```

The publish flow performs:

1. Version bump (`.env` and `CMakeLists.txt`)
2. Build AU, VST3, Standalone
3. Code signing and notarization
4. Signed installer package + DMG + ZIP creation
5. GitHub release creation with binaries + license docs attached

Release artifacts are created on `~/Desktop` before upload.

## Configuration

Project and release settings are loaded from `.env`.

Key variables include:
- `PROJECT_NAME`, `PROJECT_BUNDLE_ID`
- `VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`
- `APPLE_ID`, `TEAM_ID`, `APP_CERT`, `INSTALLER_CERT`
- `APP_SPECIFIC_PASSWORD`
- `GITHUB_USER`, `GITHUB_REPO`

## DAW Notes (Logic Pro)

After installing a new AU build/version, run a plugin rescan in Logic Plugin Manager if it does not appear immediately.

## Licensing and Acknowledgements

- Mutable Instruments Braids source reference: https://github.com/pichenettes/eurorack/tree/master/braids
- JUCE framework: https://github.com/juce-framework/JUCE

Project license/attribution files:
- `LICENSE`
- `LICENSES.md`
- `braidy-licenses.html`
- `installer/THIRD_PARTY_LICENSES.md`
