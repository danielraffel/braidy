# Braidy: Software Synthesizer Product Specification
## Based on Mutable Instruments Braids Eurorack Module

### Executive Summary
Braidy is a highly faithful software recreation of the Mutable Instruments Braids macro-oscillator module, implemented as a JUCE audio plugin. The software preserves the original's 45+ synthesis models, distinctive character, and unique interface paradigm while adapting to the advantages of a software environment through MIDI control and polyphonic capability.

---

## 1. Core Product Vision

### 1.1 Primary Goals
- **Faithful Recreation**: Preserve the exact sonic character and behavior of the original Braids hardware
- **Software Adaptation**: Thoughtfully translate CV/hardware paradigms to MIDI/software equivalents
- **Enhanced Capabilities**: Add polyphony and other software-specific enhancements while maintaining the original's essence
- **Authentic Interface**: Recreate the encoder + LED display interaction paradigm faithfully

### 1.2 Target Audience
- Electronic music producers seeking the distinctive Braids sound
- Former Braids hardware users wanting a software version
- Sound designers interested in experimental synthesis techniques
- Musicians exploring diverse synthesis methods in one instrument

---

## 2. Technical Architecture

### 2.1 Core Synthesis Engine
The synthesizer will implement all 45 synthesis models from the original Braids firmware:

#### Classic Analog Waveforms (5 models)
- **CSAW**: CS-80 inspired sawtooth with notch
- **/\/|-_-_**: Variable morphing between triangle, saw, square, pulse
- **/|/|-_-_**: Sawtooth/square with PWM
- **FOLD**: Sine/triangle wavefolder
- **TOY***: Circuit-bent lo-fi sounds

#### Digital Synthesis (11 models)
- **_|_|_|_**: Dual detuned harmonic combs
- **SYN-_/SYN/|**: Dual oscillator hard sync
- **/|/| x3, -_ x3, /\ x3, SI x3**: Triple oscillator configurations
- **RING**: Triple ring-modulated sine waves
- **/|/|/|/|**: 7-oscillator sawtooth swarm
- **/|/|_|_|**: Comb-filtered sawtooth

#### Filtered Synthesis (4 models)
- **ZLPF, ZPKF, ZBPF, ZHPF**: Direct synthesis of filtered waveforms

#### Formant Synthesis (3 models)
- **VOSM**: VOSIM formant synthesis
- **VOWL**: Low-fi vowel synthesis
- **VFOF**: FOF vowel synthesis

#### Additive & FM (4 models)
- **HARM**: 14-harmonic additive synthesis
- **FM**: Classic 2-operator FM
- **FBFM**: Feedback FM
- **WTFM**: Chaotic FM with dual feedback

#### Physical Models (5 models)
- **PLUK**: Karplus-Strong plucked string
- **BOWD**: Bowed string
- **BLOW**: Reed instrument
- **FLUT**: Flute model
- **Physical excitation models require trigger input**

#### Percussion Models (5 models)
- **BELL**: Additive bell synthesis
- **DRUM**: Metallic drum
- **KICK**: TR-808 bass drum emulation
- **CYMB**: Cymbal noise synthesis
- **SNAR**: TR-808 snare drum emulation

#### Wavetable Synthesis (4 models)
- **WTBL**: 21 wavetables with morphing
- **WMAP**: 16x16 wavetable matrix
- **WLIN**: Linear wavetable scanning
- **WTx4**: Polyphonic wavetable with chord structures

#### Noise & Texture (6 models)
- **NOIS**: Filtered noise with tuned resonator
- **TWNQ**: Twin peaks dual resonator noise
- **CLKN**: Clocked digital noise
- **CLOU**: Sinusoidal granular synthesis
- **PRTC**: Particle/droplet synthesis
- **QPSK**: Modem/data transmission sounds

### 2.2 Control Parameters

#### Primary Controls (per voice)
- **Coarse Frequency**: ±4 octaves from base pitch
- **Fine Frequency**: ±100 cents detuning
- **FM Amount**: -100% to +100% (bipolar)
- **Timbre**: Primary timbral control (0-100%)
- **Timbre Modulation**: -100% to +100% (bipolar)
- **Color**: Secondary timbral control (0-100%)

#### MIDI Mappings
- **Note On/Off**: Pitch and gate control
- **Velocity**: Maps to amplitude and optionally to parameters
- **Pitch Bend**: ±2 semitones default (configurable)
- **Mod Wheel**: Assignable to any parameter
- **Aftertouch**: Assignable parameter modulation
- **CC Messages**: Full CC mapping for all parameters

### 2.3 Interface Design

#### Visual Elements
1. **LED Display Simulation**
   - 4-character alphanumeric display showing current model
   - Animated transitions between models
   - Parameter value display mode
   - Menu navigation display

2. **Encoder Control**
   - Virtual rotary encoder with click functionality
   - Rotation: Model selection or value adjustment
   - Click: Enter/exit menu, confirm selection
   - Long press: Access settings menu

3. **Knob Controls**
   - Fine frequency
   - Coarse frequency  
   - FM amount (bipolar)
   - Timbre
   - Timbre modulation (bipolar)
   - Color

4. **Visual Feedback**
   - Real-time waveform display
   - Parameter automation indicators
   - Polyphonic voice activity display

### 2.4 Settings Menu System

The settings menu (accessed via encoder click) includes:

#### Synthesis Settings
- **META**: Enable model selection via modulation
- **BITS**: Bit depth (4-16 bits)
- **RATE**: Sample rate (4kHz-96kHz)

#### Envelope Settings
- **|\ATT**: Attack time (0-10s)
- **|\DEC**: Decay time (0-10s)
- **|\FM**: Envelope to FM amount
- **|\TIM**: Envelope to Timbre amount
- **|\COL**: Envelope to Color amount
- **|\VCA**: Envelope to amplitude amount

#### Pitch Settings
- **RANG**: Coarse knob range (EXT/FREE/XTND/440)
- **OCTV**: Octave transposition (-4 to +4)
- **QNTZ**: Scale quantization (50+ scales)
- **ROOT**: Quantizer root note

#### Character Settings
- **FLAT**: Analog-style frequency drift
- **DRFT**: VCO-style pitch instability
- **SIGN**: Unique module character/quirks

### 2.5 Polyphonic Architecture

#### Voice Management
- **Polyphony**: 1-16 voices (configurable)
- **Voice Allocation**: Last-note, round-robin, or lowest-note priority
- **Unison Mode**: Stack voices with configurable detune
- **Voice Stealing**: Intelligent voice management

#### Per-Voice Independence
- Each voice maintains independent:
  - Synthesis model state
  - Envelope state
  - Parameter values
  - Modulation routing

---

## 3. Development Phases

### Phase 1: Foundation & Core Architecture (Weeks 1-3)

#### 1.1 Project Setup
- Initialize JUCE project structure
- Configure build system for AU/VST3/Standalone
- Set up basic plugin processor and editor classes
- Implement preset management system

#### 1.2 DSP Framework
- Port core DSP utilities from Braids firmware
- Implement sample rate handling and oversampling
- Create parameter management system
- Build modulation routing architecture

#### 1.3 Basic Synthesis Models
- Port and test CSAW model
- Port basic waveform models (/\/|-_-_, /|/|-_-_, FOLD)
- Implement parameter interpolation
- Verify bit-accurate output against original

**Deliverable**: Working plugin with 4 basic synthesis models

---

### Phase 2: Classic Synthesis Models (Weeks 4-6)

#### 2.1 Digital Synthesis Models
- Port harmonic comb models (_|_|_|_)
- Implement hard sync models (SYN-_, SYN/|)
- Add triple oscillator variants
- Implement ring modulation model

#### 2.2 Filtered Synthesis
- Port CZ-style filter synthesis (ZLPF, ZPKF, ZBPF, ZHPF)
- Implement filter parameter mapping
- Add waveshape morphing

#### 2.3 Swarm & Effects
- Implement 7-voice sawtooth swarm
- Add comb filtering model
- Port TOY* circuit-bent model

**Deliverable**: 16 total synthesis models operational

---

### Phase 3: Advanced Synthesis (Weeks 7-9)

#### 3.1 Formant & Vowel Synthesis
- Port VOSIM implementation
- Implement VOWL speech synthesis
- Add FOF formant synthesis
- Create formant parameter mapping

#### 3.2 FM Synthesis
- Implement classic 2-operator FM
- Add feedback FM variant
- Implement chaotic FM with dual feedback
- Optimize FM calculations

#### 3.3 Additive Synthesis
- Port 14-harmonic additive engine
- Implement spectral control parameters
- Add harmonic distribution controls

**Deliverable**: Complete tonal synthesis collection (26 models)

---

### Phase 4: Physical Modeling (Weeks 10-12)

#### 4.1 String Models
- Port Karplus-Strong (PLUK)
- Implement bowed string (BOWD)
- Add damping and excitation controls
- Implement trigger detection system

#### 4.2 Wind Models
- Port reed instrument model (BLOW)
- Implement flute synthesis (FLUT)
- Add breath and geometry controls

#### 4.3 Percussion Models
- Port TR-808 kick drum
- Implement TR-808 snare
- Add bell synthesis
- Implement metallic drum
- Port cymbal noise generator

**Deliverable**: Complete physical modeling suite (35 models)

---

### Phase 5: Wavetables & Noise (Weeks 13-14)

#### 5.1 Wavetable Engine
- Port wavetable infrastructure
- Implement 21 wavetable banks
- Add 16x16 wavetable map
- Implement linear scanning mode
- Add 4-voice paraphonic mode

#### 5.2 Noise Generators
- Implement filtered noise
- Add twin peaks resonator
- Port clocked digital noise
- Implement granular cloud synthesis
- Add particle synthesis
- Port QPSK modem sounds

**Deliverable**: All 45 synthesis models complete

---

### Phase 6: User Interface (Weeks 15-17)

#### 6.1 Core UI Implementation
- Create main plugin window
- Implement virtual LED display
- Add encoder control with animations
- Create knob components with bipolar support

#### 6.2 Visual Feedback
- Add real-time waveform display
- Implement voice activity indicators
- Create parameter automation display
- Add preset browser

#### 6.3 Menu System
- Implement settings menu navigation
- Create all menu pages
- Add value editing interface
- Implement menu memory and persistence

**Deliverable**: Fully functional user interface

---

### Phase 7: Polyphony & MIDI (Weeks 18-19)

#### 7.1 Voice Architecture
- Implement voice allocation system
- Add polyphony controls (1-16 voices)
- Create unison mode with detune
- Implement voice stealing algorithms

#### 7.2 MIDI Implementation
- Complete MIDI note handling
- Add velocity mapping
- Implement pitch bend
- Add mod wheel routing
- Create CC mapping system
- Implement MPE support (optional)

**Deliverable**: Full polyphonic operation

---

### Phase 8: Advanced Features (Weeks 20-21)

#### 8.1 Modulation System
- Internal envelope generator
- LFO implementation (optional enhancement)
- Modulation matrix
- Parameter automation

#### 8.2 Effects & Processing
- High-quality oversampling
- Bit depth reduction
- Sample rate reduction
- Analog modeling (drift, detuning)

#### 8.3 Meta Mode
- Model selection via modulation
- Smooth model morphing (where possible)
- Model randomization

**Deliverable**: Enhanced feature set

---

### Phase 9: Optimization & Polish (Weeks 22-23)

#### 9.1 Performance Optimization
- CPU usage profiling
- SIMD optimizations
- Memory usage optimization
- Reduce latency

#### 9.2 Preset System
- Create factory preset bank
- Implement preset morphing
- Add preset randomization
- Create category system

#### 9.3 Documentation
- User manual creation
- Preset design guide
- MIDI implementation chart

**Deliverable**: Production-ready plugin

---

### Phase 10: Testing & Release (Week 24)

#### 10.1 Quality Assurance
- Comprehensive model testing
- A/B testing with original hardware
- Automation testing
- Performance benchmarking

#### 10.2 Beta Testing
- Internal testing phase
- Select user beta program
- Bug fixes and refinements
- Performance optimization

#### 10.3 Release Preparation
- Installer creation
- License system integration
- User Manual and updated README.md

**Deliverable**: Released product

---

## 4. Future Enhancement Opportunities

### Post-Launch Features (Not in initial release)
1. **Extended Polyphony**
   - MPE (MIDI Polyphonic Expression) support
   - Per-voice model selection
   - Poly-chain support

2. **Advanced Modulation**
   - Multiple LFOs
   - Extended envelope generators
   - Step sequencer
   - Euclidean rhythm generators

3. **Effects Section**
   - Reverb tailored for Braids
   - Delay with model-specific settings
   - Distortion and saturation
   - Modulated filters

4. **Creative Extensions**
   - Model morphing/crossfading
   - Custom wavetable import
   - Macro control system
   - Randomization with constraints

5. **Integration Features**
   - DAW parameter locks
   - Advanced MIDI learn
   - OSC control
   - Hardware controller templates

---

## 5. Technical Requirements

### Minimum System Requirements
- **OS**: macOS 10.13+ / Windows 10+ / Linux (Ubuntu 20.04+)
- **CPU**: Intel Core i5 or equivalent
- **RAM**: 4GB minimum, 8GB recommended
- **Disk**: 200MB free space
- **Formats**: AU, VST3, Standalone

### Development Tools
- **Framework**: JUCE 7.x
- **IDE**: Xcode (macOS), Visual Studio (Windows)
- **Version Control**: Git
- **Testing**: JUCE unit tests, PluginVal
- **DSP Reference**: Original Braids source code (MIT licensed)

---

## 6. Success Metrics

### Technical Goals
- CPU usage < 20% for 8-voice polyphony (modern i5)
- Latency < 64 samples
- Bit-accurate synthesis (where applicable)
- Sample-accurate automation

### User Experience Goals
- Intuitive interface matching hardware paradigm
- Smooth parameter changes without clicks/pops
- Preset recall < 100ms
- Stable operation in all major DAWs

### Market Goals
- Feature parity with original hardware
- Competitive with similar software instruments
- Positive user reviews (4.5+ stars average)
- Active user community

---

## 7. Risk Mitigation

### Technical Risks
- **DSP Complexity**: Mitigated by leveraging open-source Braids code
- **CPU Performance**: Address through optimization and optional quality settings
- **Compatibility**: Extensive testing across DAWs and systems

### Legal Considerations
- Original Braids firmware is MIT licensed (permissive)
- Ensure proper attribution to Émilie Gillet/Mutable Instruments
- Avoid trademark conflicts with naming and branding

### Development Risks
- **Scope Creep**: Strict adherence to phase-based development
- **Timeline**: Built-in buffer periods between phases
- **Quality**: Continuous testing throughout development

---

## 8. Conclusion

Braidy represents a comprehensive software adaptation of one of the most versatile and beloved eurorack modules. By maintaining strict fidelity to the original while thoughtfully adapting to the software environment, this instrument will serve both existing Braids users and new musicians discovering its unique sonic palette. The phased development approach ensures systematic progress while maintaining flexibility for refinements based on testing and feedback.

The key to success lies in preserving the module's character and immediacy while leveraging software's advantages in polyphony, preset management, and integration. With careful attention to both technical implementation and user experience, Braidy will establish itself as the definitive software version of this classic synthesis platform.