#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "adapters/BraidsEngine.h"
#include <cmath>

BraidyAudioProcessor::BraidyAudioProcessor()
     : AudioProcessor(BusesProperties()
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true))
     , apvts_(*this, nullptr, "Parameters", createParameterLayout())
{
    // Create synthesiser with 8 voices
    synthesiser_ = std::make_unique<BraidyAdapter::BraidsSynthesiser>(8);

    // Initialize parameters to their default values
    // Using Griddy's approach - no separate base value tracking needed

    // Debug: Test debug logging system at startup
    DBG("[STARTUP DEBUG] BraidyAudioProcessor constructed, testing debug output...");

    // Set up parameter listener for algorithm changes
    algorithmListener_ = std::make_unique<AlgorithmParameterListener>(*this);
    if (auto* algorithmParam = apvts_.getParameter("algorithm")) {
        algorithmParam->addListener(algorithmListener_.get());
        DBG("[STARTUP] Added listener to algorithm parameter");
    }

    // Start timer for thread-safe parameter updates (50Hz)
    startTimer(20);
}

BraidyAudioProcessor::~BraidyAudioProcessor()
{
    // Stop timer first to prevent any more callbacks
    stopTimer();

    // Remove parameter listener
    if (algorithmListener_ && apvts_.getParameter("algorithm")) {
        apvts_.getParameter("algorithm")->removeListener(algorithmListener_.get());
    }

    // Add small delay to ensure timer callback completes
    juce::Thread::sleep(10);

    // Stop all active notes before clearing
    if (synthesiser_) {
        synthesiser_->allNotesOff(0, true);  // Force immediate note off

        // Wait briefly for notes to stop
        juce::Thread::sleep(5);

        // Clear all voices to prevent any audio processing during shutdown
        synthesiser_->clearSounds();
        synthesiser_->clearVoices();
    }

    // Clear modulation matrix
    modulationMatrix_.clearAllRoutings();

    // Ensure debug output is flushed
    DBG("[SHUTDOWN] BraidyAudioProcessor destroyed");
}

juce::AudioProcessorValueTreeState::ParameterLayout BraidyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Get algorithm names
    auto algorithmNames = BraidyAdapter::BraidsEngine::getAllAlgorithmNames();
    juce::StringArray algorithmChoices;
    for (const auto& name : algorithmNames) {
        algorithmChoices.add(name);
    }
    
    // Algorithm selector
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ALGORITHM_ID, 1},
        "Algorithm",
        algorithmChoices,
        0
    ));
    
    // Parameter 1 (Timbre/Color/etc depending on algorithm)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{PARAM1_ID, 1},
        "Parameter 1",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    // Parameter 2
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{PARAM2_ID, 1},
        "Parameter 2",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    // Volume
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VOLUME_ID, 1},
        "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.7f
    ));
    
    // Fine Tune - Braids doesn't have a dedicated fine tune, but we can simulate it
    // Using small pitch offset (-2 to +2 semitones for fine control)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fineTune", 1},
        "Fine Tune",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f  // Center position = no detuning
    ));
    
    // Coarse Tune - Braids uses octave switching: -2, -1, 0, +1, +2
    // Matching hardware: 5 discrete positions
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"coarseTune", 1},
        "Coarse Tune",
        juce::StringArray{"-2 Oct", "-1 Oct", "0", "+1 Oct", "+2 Oct"},
        2  // Default to center (0 octaves)
    ));
    
    // FM - In Braids, this is the internal envelope modulation of pitch
    // Range matches AD_FM setting (0-127 in hardware)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fmAmount", 1},
        "FM",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f  // Default to no FM
    ));
    
    // Meta Modulation - In Braids, enables algorithm modulation via FM input
    // When enabled, FM CV controls algorithm selection
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"metaMode", 1},
        "Meta Mode",
        true  // Default to ON to match Braids hardware behavior
    ));
    
    // Envelope to Timbre amount - In Braids, this is |\TIM setting
    // Controls how much the envelope affects the timbre parameter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"envTimbreAmount", 1},
        "Env→Timbre Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f  // Default to no envelope modulation
    ));
    
    // Item #7: Add modulation parameters to APVTS
    // LFO 1 Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo1Enable", 1},
        "LFO 1 Enable",
        true  // Default to ON for immediate modulation
    ));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo1Shape", 1},
        "LFO 1 Shape",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw", "Random", "Sample & Hold"},
        0  // Default to Sine
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Rate", 1},
        "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.5f),  // Logarithmic skew
        1.0f  // 1 Hz default
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Depth", 1},
        "LFO 1 Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo1TempoSync", 1},
        "LFO 1 Tempo Sync",
        true  // Default to tempo sync ON
    ));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo1Dest", 1},
        "LFO 1 Destination",
        juce::StringArray{"None", "META", "Timbre", "Color", "FM", "Modulation",
                         "Coarse (Pitch)", "Fine (Detune)", "Coarse (Octave)", "Env Attack", "Env Decay", "Env Sustain", "Env Release",
                         "Env FM", "Env Timbre", "Env Color", "Bit Depth", 
                         "Sample Rate", "Volume", "Pan", "Quantize Scale", "Quantize Root", 
                         "Vibrato Amount", "Vibrato Rate"},
        0  // Default to None
    ));
    
    // LFO 2 Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo2Enable", 1},
        "LFO 2 Enable",
        false
    ));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo2Shape", 1},
        "LFO 2 Shape",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw", "Random", "Sample & Hold"},
        0  // Default to Sine
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Rate", 1},
        "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.5f),  // Logarithmic skew
        1.0f  // 1 Hz default
    ));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Depth", 1},
        "LFO 2 Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"lfo2TempoSync", 1},
        "LFO 2 Tempo Sync",
        false
    ));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lfo2Dest", 1},
        "LFO 2 Destination",
        juce::StringArray{"None", "META", "Timbre", "Color", "FM", "Modulation",
                         "Coarse (Pitch)", "Fine (Detune)", "Coarse (Octave)", "Env Attack", "Env Decay", "Env Sustain", "Env Release",
                         "Env FM", "Env Timbre", "Env Color", "Bit Depth", 
                         "Sample Rate", "Volume", "Pan", "Quantize Scale", "Quantize Root", 
                         "Vibrato Amount", "Vibrato Rate"},
        0  // Default to None
    ));
    
    return layout;
}

void BraidyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    DBG("[DEBUG] prepareToPlay: sampleRate=" + juce::String(sampleRate) +
        " samplesPerBlock=" + juce::String(samplesPerBlock));
    synthesiser_->setCurrentPlaybackSampleRate(sampleRate);
    
    // Connect modulation matrix to synthesizer for real-time modulation
    synthesiser_->setModulationMatrix(&modulationMatrix_);
    
    midiCollector_.reset(sampleRate);
    updateSynthesiserFromParameters();
}

void BraidyAudioProcessor::releaseResources()
{
    // Nothing to do here
}

bool BraidyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only supports stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
        
    return true;
}

void BraidyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Debug: Log algorithm parameter value periodically to check if encoder is working
    static int algorithmDebugCounter = 0;
    if (++algorithmDebugCounter % 100 == 0) {  // Every ~2 seconds at 48kHz/512 buffer
        if (auto* algorithmParam = apvts_.getRawParameterValue("algorithm")) {
            float rawValue = algorithmParam->load();
            int algorithmIndex = static_cast<int>(std::round(rawValue));
            DBG("[PROCESSBLOCK DEBUG] Algorithm parameter: raw=" + juce::String(rawValue) +
                " index=" + juce::String(algorithmIndex) + " current=" + juce::String(currentAlgorithm_));
        }
    }

    // No manual adjustment flags needed with Griddy's simpler approach

    // Collect MIDI from the UI keyboard
    juce::MidiBuffer collectedMidi;
    midiCollector_.removeNextBlockOfMessages(collectedMidi, buffer.getNumSamples());
    
    // Merge collected MIDI with incoming MIDI
    for (const auto metadata : collectedMidi) {
        midiMessages.addEvent(metadata.getMessage(), metadata.samplePosition);
    }
    
    // Debug MIDI messages
    if (!midiMessages.isEmpty()) {
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn()) {
                DBG("[DEBUG] ProcessBlock MIDI NoteOn: note=" + juce::String(msg.getNoteNumber()) +
                    " vel=" + juce::String(msg.getVelocity()));
            } else if (msg.isNoteOff()) {
                DBG("[DEBUG] ProcessBlock MIDI NoteOff: note=" + juce::String(msg.getNoteNumber()));
            }
        }
    }
    
    // Clear any input audio
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Update parameters if changed
    updateSynthesiserFromParameters();
    
    // Update modulation matrix from APVTS parameters
    updateModulationFromParameters();
    
    // Process modulation LFOs (Item #2: Connect LFO processing to audio thread)
    double bpm = 120.0; // Default BPM
    if (auto* playHead = getPlayHead()) {
        if (auto positionInfo = playHead->getPosition()) {
            if (positionInfo->getBpm().hasValue()) {
                double hostBpm = *positionInfo->getBpm();
                // Validate BPM is in reasonable range to avoid division by zero
                if (hostBpm > 0.0 && hostBpm < 999.0) {
                    bpm = hostBpm;
                }
            }
        }
    }
    modulationMatrix_.processBlock(getSampleRate(), buffer.getNumSamples(), bpm);
    
    // Process MIDI and generate audio using JUCE's built-in synthesiser renderNextBlock
    synthesiser_->renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Apply gain compensation for polyphony to prevent clipping
    // With 8 voices, we need to scale down to prevent summing above 0dB
    // Using a conservative scaling factor that maintains headroom
    const float polyphonyCompensation = 0.35f; // Scale down to prevent clipping with multiple voices
    buffer.applyGain(polyphonyCompensation);
    
    // Apply soft clipping as safety to prevent any remaining peaks
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            // Soft clipping using tanh for musical saturation
            channelData[sample] = std::tanh(channelData[sample]);
        }
    }
    
    // Check if any audio was generated
    float maxSample = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            maxSample = std::max(maxSample, std::abs(channelData[i]));
        }
    }
    
    static int debugCounter = 0;
    if (++debugCounter % 100 == 0) {  // Print every 100 blocks
        DBG("[DEBUG] Audio max sample: " + juce::String(maxSample) +
            " Active voices: " + juce::String(synthesiser_->getActiveVoiceCount()));
    }
    
    // Apply volume
    float volume = currentVolume_.load();
    buffer.applyGain(volume);
}

void BraidyAudioProcessor::updateSynthesiserFromParameters()
{
    // No recursion guard needed with Griddy's simpler architecture
    // Parameters are read directly, modulation is applied separately
    
    // Get parameter values
    auto* algorithmParam = apvts_.getRawParameterValue(ALGORITHM_ID);
    auto* param1 = apvts_.getRawParameterValue(PARAM1_ID);
    auto* param2 = apvts_.getRawParameterValue(PARAM2_ID);
    auto* volumeParam = apvts_.getRawParameterValue(VOLUME_ID);
    auto* fineTuneParam = apvts_.getRawParameterValue("fineTune");
    auto* coarseTuneParam = apvts_.getRawParameterValue("coarseTune");
    auto* fmAmountParam = apvts_.getRawParameterValue("fmAmount");
    
    if (algorithmParam && param1 && param2 && volumeParam) {
        // For AudioParameterChoice, the raw value is already the index (0-46)
        // No conversion needed
        float algorithmIndex = algorithmParam->load();
        int newAlgorithm = static_cast<int>(std::round(algorithmIndex));
        
        // Debug output - increased frequency for debugging
        static int debugCount = 0;
        if (++debugCount % 10 == 0) {  // Log more frequently to catch algorithm changes
            DBG("[PROCESSBLOCK] Algorithm index: raw=" + juce::String(algorithmIndex) +
                " -> rounded=" + juce::String(newAlgorithm) +
                " (current=" + juce::String(currentAlgorithm_) + ")");
        }
        
        float newParam1 = param1->load();
        float newParam2 = param2->load();
        float newVolume = volumeParam->load();
        
        // Apply modulation to parameters (Item #3: Apply modulation to synthesis parameters)
        // Timbre modulation
        if (modulationMatrix_.isModulated(braidy::ModulationMatrix::TIMBRE)) {
            newParam1 = modulationMatrix_.applyModulation(
                braidy::ModulationMatrix::TIMBRE, newParam1, 0.0f, 1.0f);
        }
        
        // ENV_TIMBRE_AMOUNT modulation - this controls the envTimbreAmount parameter
        // For now, this just provides visual feedback in the UI
        // TODO: Implement envelope-to-timbre modulation in BraidsEngine
        
        // Color modulation
        if (modulationMatrix_.isModulated(braidy::ModulationMatrix::COLOR)) {
            newParam2 = modulationMatrix_.applyModulation(
                braidy::ModulationMatrix::COLOR, newParam2, 0.0f, 1.0f);
        }
        
        // META mode algorithm modulation will be handled in FM section to avoid duplication
        
        if (newParam1 != currentParam1_ || newParam2 != currentParam2_) {
            DBG("[DEBUG] PluginProcessor: Parameters changed - param1: " +
                juce::String(currentParam1_) + " -> " + juce::String(newParam1) +
                ", param2: " + juce::String(currentParam2_) + " -> " + juce::String(newParam2));
            currentParam1_ = newParam1;
            currentParam2_ = newParam2;
            synthesiser_->setParameters(newParam1, newParam2);
        }
        
        currentVolume_ = newVolume;
        
        // Apply tuning parameters to all voices
        if (fineTuneParam && coarseTuneParam) {
            // Fine tune: -2 to +2 semitones for fine control
            float fineTuneBase = fineTuneParam->load();
            // Apply DETUNE modulation
            if (modulationMatrix_.isModulated(braidy::ModulationMatrix::DETUNE)) {
                fineTuneBase = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::DETUNE, fineTuneBase, 0.0f, 1.0f);
            }
            float fineTune = (fineTuneBase - 0.5f) * 4.0f;
            
            // Coarse tune: Braids hardware uses octave switching
            // AudioParameterChoice returns the index directly (0-4), convert to octave offset
            float octaveIndexFloat = coarseTuneParam->load();
            // Apply OCTAVE modulation
            if (modulationMatrix_.isModulated(braidy::ModulationMatrix::OCTAVE)) {
                octaveIndexFloat = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::OCTAVE, octaveIndexFloat, 0.0f, 4.0f);
            }
            int octaveIndex = static_cast<int>(std::round(octaveIndexFloat));
            float coarseTune = (octaveIndex - 2) * 12.0f;  // -2 to +2 octaves in semitones
            
            float totalPitchOffset = fineTune + coarseTune;
            
            // Apply to all voices
            for (int i = 0; i < synthesiser_->getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidyAdapter::BraidsVoice*>(synthesiser_->getVoice(i))) {
                    voice->setPitchOffset(totalPitchOffset);
                }
            }
        }
        
        // Apply FM amount and META mode algorithm modulation
        if (fmAmountParam) {
            // Use Griddy's approach: always get base value from APVTS
            float fmAmount = fmAmountParam->load();
            
            // CRITICAL: Check if FM is an LFO destination BEFORE applying modulation
            // This prevents race conditions in META mode
            bool fmIsLfoDestination = false;
            if (auto* lfo1Dest = apvts_.getRawParameterValue("lfo1Dest")) {
                fmIsLfoDestination = (static_cast<int>(lfo1Dest->load()) == 4); // 4 = FM
            }
            if (!fmIsLfoDestination) {
                if (auto* lfo2Dest = apvts_.getRawParameterValue("lfo2Dest")) {
                    fmIsLfoDestination = (static_cast<int>(lfo2Dest->load()) == 4); // 4 = FM
                }
            }
            
            // Check META mode first
            auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
            bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;
            
            // SIMPLIFIED META MODE IMPLEMENTATION (matching original Braids):
            // - META is just an on/off setting (not an LFO destination)
            // - When META=ON: FM input controls algorithm selection
            // - When META=OFF: FM input works as normal frequency modulation
            // - FM knob position determines algorithm range in META mode
            
            if (metaMode) {
                // META MODE ACTIVE - FM controls algorithm selection instead of frequency modulation
                int baseAlgorithm = newAlgorithm;
                int targetAlgorithm = baseAlgorithm;
                
                // Check if FM is being modulated by LFO (this becomes algorithm modulation in META mode)
                bool fmModulated = fmIsLfoDestination || modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT);
                
                if (fmModulated) {
                    // Get the current FM value (base or modulated)
                    float currentFm = fmAmount;
                    
                    // Apply FM modulation from LFO if routed
                    if (fmIsLfoDestination || modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                        currentFm = modulationMatrix_.applyModulation(
                            braidy::ModulationMatrix::FM_AMOUNT, fmAmount, 0.0f, 1.0f);
                    }
                    
                    // FM knob sets the algorithm range
                    // When FM knob is 0, use a default range to allow some cycling
                    // As FM knob increases, the range increases up to all 46 algorithms
                    int algorithmRange;
                    if (fmAmount < 0.01f) {
                        // FM knob at minimum - use a moderate default range
                        algorithmRange = 8;  // Cycle through 8 algorithms by default
                    } else {
                        // FM knob turned up - use it to control range (1-46)
                        algorithmRange = std::max(1, static_cast<int>(fmAmount * 46.0f));
                    }
                    
                    // The LFO modulation (difference from base) drives algorithm selection
                    float modulationAmount = currentFm - fmAmount; // How much LFO is adding/subtracting
                    int algorithmOffset = static_cast<int>(modulationAmount * algorithmRange);
                    targetAlgorithm = baseAlgorithm + algorithmOffset;
                    
                    // Debug logging - increased frequency for better visibility
                    static int metaLogCounter = 0;
                    if (++metaLogCounter % 10 == 0) {  // Log more frequently
                        DBG("[META MODE FM] Active! Base: " + juce::String(baseAlgorithm) +
                            ", FM Knob: " + juce::String(fmAmount) +
                            ", Current FM: " + juce::String(currentFm) +
                            ", FmIsLfoDest: " + juce::String(fmIsLfoDestination ? "true" : "false") +
                            ", Range: " + juce::String(algorithmRange) +
                            ", Modulation: " + juce::String(modulationAmount) +
                            ", Offset: " + juce::String(algorithmOffset) +
                            ", Target: " + juce::String(targetAlgorithm));
                    }
                } else {
                    // No FM modulation - use FM knob directly for algorithm selection
                    // Map FM knob (0.0-1.0) to algorithm range centered around baseAlgorithm
                    int algorithmRange = static_cast<int>(fmAmount * 23.0f) + 1; // 1-24 algorithms
                    int halfRange = algorithmRange / 2;
                    int algorithmOffset = static_cast<int>((fmAmount - 0.5f) * algorithmRange);
                    targetAlgorithm = baseAlgorithm + algorithmOffset;
                    
                    // Ensure we stay within bounds by wrapping around if needed
                    if (targetAlgorithm < 0) {
                        targetAlgorithm = (targetAlgorithm % 46 + 46) % 46;
                    } else if (targetAlgorithm >= 46) {
                        targetAlgorithm = targetAlgorithm % 46;
                    }
                }
                
                // Clamp algorithm to valid range (0-45, which is 46 total algorithms)
                targetAlgorithm = std::max(0, std::min(45, targetAlgorithm));
                
                // Always update metaModeAlgorithm_ for display purposes
                metaModeAlgorithm_.store(targetAlgorithm);
                
                // Apply algorithm change
                if (targetAlgorithm != currentAlgorithm_) {
                    int activeVoices = synthesiser_->getActiveVoiceCount();
                    
                    // IMPROVED STRATEGY: Allow algorithm changes with safety measures
                    if (activeVoices == 0) {
                        // Safe to change immediately when no voices active
                        currentAlgorithm_ = targetAlgorithm;
                        synthesiser_->setAlgorithm(targetAlgorithm);
                        pendingAlgorithmUpdate_.store(targetAlgorithm);
                        
                        DBG("[META SAFE CHANGE] Algorithm " + juce::String(currentAlgorithm_) +
                            " -> " + juce::String(targetAlgorithm) + " (no active voices)");
                    } else {
                        // When voices are active, use graceful transition:
                        // 1. Force stop all voices immediately (no tail-off)
                        // 2. Apply algorithm change  
                        // 3. Allow new notes to start with new algorithm
                        DBG("[META FORCED CHANGE] Force stopping " + juce::String(activeVoices) +
                            " voices for algorithm " + juce::String(currentAlgorithm_) + " -> " + juce::String(targetAlgorithm));
                        
                        // Use forceStopAllVoices instead of allNotesOff for immediate clearing
                        synthesiser_->forceStopAllVoices();
                        
                        // Verify voices are actually cleared
                        int remainingVoices = synthesiser_->getActiveVoiceCount();
                        if (remainingVoices > 0) {
                            DBG("[META WARNING] " + juce::String(remainingVoices) + " voices still active after force stop");
                        }
                        
                        currentAlgorithm_ = targetAlgorithm;
                        synthesiser_->setAlgorithm(targetAlgorithm);
                        pendingAlgorithmUpdate_.store(targetAlgorithm);
                    }
                } else {
                    // Always update the meta mode display algorithm even if audio algorithm doesn't change
                    metaModeAlgorithm_.store(targetAlgorithm);
                }
            }
            else {
                // NORMAL MODE - Standard FM modulation
                // Apply FM modulation from LFO if routed
                if (fmIsLfoDestination && modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                    float originalFm = fmAmount;
                    fmAmount = modulationMatrix_.applyModulation(
                        braidy::ModulationMatrix::FM_AMOUNT, fmAmount, 0.0f, 1.0f);
                    
                    // Debug logging for normal FM modulation
                    static int fmModLogCounter = 0;
                    if (++fmModLogCounter % 50 == 0) {
                        DBG("[FM MODULATION] Original: " + juce::String(originalFm) +
                            ", Modulated: " + juce::String(fmAmount));
                    }
                }
                
                // Algorithm from parameter only in normal mode
                if (newAlgorithm != currentAlgorithm_) {
                    DBG("[ALGORITHM CHANGE] Switching from " + juce::String(currentAlgorithm_) +
                        " to " + juce::String(newAlgorithm));

                    synthesiser_->allNotesOff(0, true);
                    currentAlgorithm_ = newAlgorithm;
                    synthesiser_->setAlgorithm(newAlgorithm);

                    // Ensure parameters are set to reasonable defaults for the new algorithm
                    // Most algorithms work well with center position (0.5, 0.5)
                    synthesiser_->setParameters(0.5f, 0.5f);
                    DBG("[ALGORITHM CHANGE] Set default parameters (0.5, 0.5) for algorithm " +
                        juce::String(currentAlgorithm_));

                    DBG("[NORMAL] Algorithm changed to " + juce::String(currentAlgorithm_));
                }
            }
            
            // Apply modulation to timbre and color parameters
            float timbre = param1->load();
            float color = param2->load();
            
            // Apply LFO modulation if active
            if (modulationMatrix_.isModulated(braidy::ModulationMatrix::TIMBRE)) {
                float originalTimbre = timbre;
                timbre = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::TIMBRE, timbre, 0.0f, 1.0f);
                
                // Debug logging for timbre modulation
                static int timbreModLogCounter = 0;
                if (++timbreModLogCounter % 50 == 0 && std::abs(originalTimbre - timbre) > 0.001f) {
                    DBG("[TIMBRE MODULATION] Original: " + juce::String(originalTimbre) +
                        ", Modulated: " + juce::String(timbre));
                }
            }
            
            if (modulationMatrix_.isModulated(braidy::ModulationMatrix::COLOR)) {
                float originalColor = color;
                color = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::COLOR, color, 0.0f, 1.0f);
                
                // Debug logging for color modulation
                static int colorModLogCounter = 0;
                if (++colorModLogCounter % 50 == 0 && std::abs(originalColor - color) > 0.001f) {
                    DBG("[COLOR MODULATION] Original: " + juce::String(originalColor) +
                        ", Modulated: " + juce::String(color));
                }
            }
            
            // Pass values to voices
            // Allow META mode when FM is LFO destination - this enables algorithm cycling via FM modulation
            bool effectiveMetaMode = metaMode;
            
            for (int i = 0; i < synthesiser_->getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidyAdapter::BraidsVoice*>(synthesiser_->getVoice(i))) {
                    voice->setMetaMode(effectiveMetaMode);
                    voice->setFMAmount(fmAmount);
                    voice->setTimbre(timbre);
                    voice->setColor(color);
                }
            }
        }
    }
}

void BraidyAudioProcessor::updateModulationFromParameters()
{
    // No guards needed - use Griddy's simpler approach
    // This function just updates LFO settings and routings from APVTS parameters

    // Debug: Log when this function is called and check META mode state
    auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
    bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;

    // Debug output will be handled by DBG() macro

    static int updateCounter = 0;
    ++updateCounter;

    // Log more frequently during startup and testing
    if (updateCounter <= 5 || updateCounter % 50 == 0) {  // First 5 calls, then every 50th
        DBG("[UPDATE MODULATION] Called (count: " + juce::String(updateCounter) +
            "), META mode: " + juce::String(metaMode ? "ON" : "OFF"));
    }

    // Track previous destinations to avoid unnecessary clearing of routings
    // This prevents race conditions where manual adjustments can cause routing to be cleared
    static int previousLfo1Dest = -999;  // Initialize to impossible value
    static float previousLfo1Depth = -1.0f;
    static bool previousLfo1Enable = false;

    static int previousLfo2Dest = -999;  // Initialize to impossible value
    static float previousLfo2Depth = -1.0f;
    static bool previousLfo2Enable = false;

    // Update LFO 1 settings from APVTS
    if (auto* lfo1Enable = apvts_.getRawParameterValue("lfo1Enable")) {
        bool enabled = lfo1Enable->load() > 0.5f;
        
        // Get LFO 1 reference
        auto& lfo1 = modulationMatrix_.getLFO(0);
        
        if (enabled) {
            // Get LFO 1 parameters
            auto* lfo1Shape = apvts_.getRawParameterValue("lfo1Shape");
            auto* lfo1Rate = apvts_.getRawParameterValue("lfo1Rate");
            auto* lfo1Depth = apvts_.getRawParameterValue("lfo1Depth");
            auto* lfo1TempoSync = apvts_.getRawParameterValue("lfo1TempoSync");
            auto* lfo1Dest = apvts_.getRawParameterValue("lfo1Dest");
            
            if (lfo1Shape && lfo1Rate && lfo1Depth && lfo1TempoSync && lfo1Dest) {
                // Configure LFO 1
                int shapeIndex = static_cast<int>(lfo1Shape->load());
                braidy::LFO::Shape shape = static_cast<braidy::LFO::Shape>(shapeIndex);
                
                float rate = lfo1Rate->load();
                float depth = lfo1Depth->load();
                bool tempoSync = lfo1TempoSync->load() > 0.5f;
                
                // Set LFO parameters
                lfo1.setShape(shape);
                lfo1.setRate(rate);
                lfo1.setDepth(depth);
                lfo1.setTempoSync(tempoSync);
                lfo1.setEnabled(true);
                
                // Set routing
                int destIndex = static_cast<int>(lfo1Dest->load());

                // Check if destination or depth actually changed
                bool destinationChanged = (destIndex != previousLfo1Dest);
                bool depthChanged = std::abs(depth - previousLfo1Depth) > 0.001f;
                bool enableChanged = (enabled != previousLfo1Enable);

                // Only update routings if something actually changed
                if (destinationChanged || depthChanged || enableChanged) {
                    previousLfo1Dest = destIndex;
                    previousLfo1Depth = depth;
                    previousLfo1Enable = enabled;

                    DBG("[MODULATION] LFO1 config changed - Dest: " + juce::String(destIndex) +
                        ", Depth: " + juce::String(depth) + ", Enabled: " + juce::String(enabled ? "true" : "false"));
                }

                // Map APVTS dropdown index to ModulationMatrix::Destination enum
                // This must match the mapping in ModulationSettingsOverlay.h
                if (destIndex > 0 && destinationChanged) {
                    braidy::ModulationMatrix::Destination dest;
                    bool validDest = true;
                    
                    // Debug log the destination
                    juce::String destStr = "[MODULATION UPDATE] LFO1 dest index: " + juce::String(destIndex);
                    
                    switch (destIndex) {
                        case 1:  // "META" -> ALGORITHM_SELECTION (enum 0)
                            dest = braidy::ModulationMatrix::ALGORITHM_SELECTION;
                            DBG(destStr + " -> ALGORITHM_SELECTION");
                            break;
                        case 2:  // "Timbre" -> TIMBRE (enum 1)
                            dest = braidy::ModulationMatrix::TIMBRE;
                            DBG(destStr + " -> TIMBRE");
                            break;
                        case 3:  // "Color" -> COLOR (enum 2)
                            dest = braidy::ModulationMatrix::COLOR;
                            DBG(destStr + " -> COLOR");
                            break;
                        case 4:  // "FM" -> FM_AMOUNT (enum 3)
                            dest = braidy::ModulationMatrix::FM_AMOUNT;
                            DBG(destStr + " -> FM_AMOUNT (META mode will use this for algorithm cycling)");
                            break;
                        case 5:  // "Modulation" -> ENV_TIMBRE_AMOUNT (enum 12)
                            dest = braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT;
                            break;
                        case 6:  // "Coarse (Pitch)" -> PITCH (enum 4)
                            dest = braidy::ModulationMatrix::PITCH;
                            break;
                        case 7:  // "Fine (Detune)" -> DETUNE (enum 5)
                            dest = braidy::ModulationMatrix::DETUNE;
                            break;
                        case 8:  // "Coarse (Octave)" -> OCTAVE (enum 6)
                            dest = braidy::ModulationMatrix::OCTAVE;
                            break;
                        case 9:  // "Env Attack" -> ENV_ATTACK (enum 7)
                            dest = braidy::ModulationMatrix::ENV_ATTACK;
                            break;
                        case 10: // "Env Decay" -> ENV_DECAY (enum 8)
                            dest = braidy::ModulationMatrix::ENV_DECAY;
                            break;
                        case 11: // "Env Sustain" -> ENV_SUSTAIN (enum 9)
                            dest = braidy::ModulationMatrix::ENV_SUSTAIN;
                            break;
                        case 12: // "Env Release" -> ENV_RELEASE (enum 10)
                            dest = braidy::ModulationMatrix::ENV_RELEASE;
                            break;
                        default:
                            validDest = false;
                            DBG("[MODULATION] ERROR: No mapping defined for LFO1 APVTS index " + juce::String(destIndex));
                            break;
                    }

                    if (validDest) {
                        // Clear all routings for LFO 1 first
                        for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                            auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                            if (modulationMatrix_.getRouting(currentDest).sourceId == 0) {
                                modulationMatrix_.clearRouting(currentDest);
                            }
                        }
                        
                        // Set new routing
                        modulationMatrix_.setRouting(0, dest, depth);

                        // Initialize base value tracking for the newly modulated parameter
                        // Log routing changes
                        if (dest == braidy::ModulationMatrix::TIMBRE) {
                            DBG("[MODULATION] LFO1 routed to TIMBRE");
                        }
                        else if (dest == braidy::ModulationMatrix::COLOR) {
                            DBG("[MODULATION] LFO1 routed to COLOR");
                        }
                        else if (dest == braidy::ModulationMatrix::FM_AMOUNT) {
                            DBG("[MODULATION] LFO1 routed to FM_AMOUNT");
                        }
                        else if (dest == braidy::ModulationMatrix::DETUNE) {
                            DBG("[MODULATION] LFO1 routed to DETUNE");
                        }
                        else if (dest == braidy::ModulationMatrix::OCTAVE) {
                            DBG("[MODULATION] LFO1 routed to OCTAVE");
                        }
                    }
                } else if (destIndex == 0 && destinationChanged) {
                    // destIndex == 0 means "None" selected - only clear if this is a change
                    DBG("[MODULATION UPDATE] LFO1 dest index: " + juce::String(destIndex) + " -> NONE (clearing all routings)");

                    // Clear base value tracking flags for any parameters that were modulated by LFO1
                    // No need to reset manual adjustment flags with Griddy's approach

                    // If META mode is enabled, automatically route to META for algorithm cycling
                    auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
                    bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;

                    // Debug: Always log when we hit the "None" destination case
                    DBG("[LFO1 NONE DEST] LFO1 destination set to None. META mode: " +
                        juce::String(metaMode ? "ON" : "OFF") + ", depth: " + juce::String(depth));

                    // Clear all routings for LFO 1 first
                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 0) {
                            modulationMatrix_.clearRouting(currentDest);
                        }
                    }
                    
                    if (metaMode) {
                        // Route LFO to META for automatic algorithm cycling
                        modulationMatrix_.setRouting(0, braidy::ModulationMatrix::ALGORITHM_SELECTION, depth);
                        DBG("[LFO1 AUTO-ROUTING] None destination + META mode -> routing to ALGORITHM_SELECTION (depth=" +
                            juce::String(depth) + ")");
                        
                        // Debug: Verify that routing was applied correctly
                        bool metaModulated = modulationMatrix_.isModulated(braidy::ModulationMatrix::ALGORITHM_SELECTION);
                        float metaModValue = modulationMatrix_.getModulation(braidy::ModulationMatrix::ALGORITHM_SELECTION);
                        DBG("[ROUTING DEBUG] META modulated: " + juce::String(metaModulated ? "true" : "false") +
                            ", Current META modulation: " + juce::String(metaModValue));
                    }
                } else if (!destinationChanged && depthChanged && destIndex > 0) {
                    // Just the depth changed - update amount without clearing routing
                    // Find which destination LFO1 is currently routed to
                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 0 &&
                            modulationMatrix_.getRouting(currentDest).enabled) {
                            // Update the amount for this existing routing
                            modulationMatrix_.setRouting(0, currentDest, depth);
                            DBG("[MODULATION] Updated LFO1 depth for existing routing to " + juce::String(depth));
                            break;
                        }
                    }
                }
            }
        } else {
            // Disable LFO 1
            lfo1.setEnabled(false);

            // Only clear routings if LFO1 was previously enabled
            if (previousLfo1Enable) {
                previousLfo1Enable = false;
                previousLfo1Dest = -999;  // Reset to impossible value

                // Clear all routings for LFO 1
                for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                    auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                    if (modulationMatrix_.getRouting(currentDest).sourceId == 0) {
                        modulationMatrix_.clearRouting(currentDest);
                    }
                }
                DBG("[MODULATION] LFO1 disabled - cleared all routings");
            }
        }
    }
    
    // Update LFO 2 settings from APVTS
    if (auto* lfo2Enable = apvts_.getRawParameterValue("lfo2Enable")) {
        bool enabled = lfo2Enable->load() > 0.5f;
        
        // Get LFO 2 reference
        auto& lfo2 = modulationMatrix_.getLFO(1);
        
        if (enabled) {
            // Get LFO 2 parameters
            auto* lfo2Shape = apvts_.getRawParameterValue("lfo2Shape");
            auto* lfo2Rate = apvts_.getRawParameterValue("lfo2Rate");
            auto* lfo2Depth = apvts_.getRawParameterValue("lfo2Depth");
            auto* lfo2TempoSync = apvts_.getRawParameterValue("lfo2TempoSync");
            auto* lfo2Dest = apvts_.getRawParameterValue("lfo2Dest");
            
            if (lfo2Shape && lfo2Rate && lfo2Depth && lfo2TempoSync && lfo2Dest) {
                // Configure LFO 2
                int shapeIndex = static_cast<int>(lfo2Shape->load());
                braidy::LFO::Shape shape = static_cast<braidy::LFO::Shape>(shapeIndex);
                
                float rate = lfo2Rate->load();
                float depth = lfo2Depth->load();
                bool tempoSync = lfo2TempoSync->load() > 0.5f;
                
                // Set LFO parameters
                lfo2.setShape(shape);
                lfo2.setRate(rate);
                lfo2.setDepth(depth);
                lfo2.setTempoSync(tempoSync);
                lfo2.setEnabled(true);
                
                // Set routing
                int destIndex = static_cast<int>(lfo2Dest->load());

                // Check if destination or depth actually changed
                bool destinationChanged = (destIndex != previousLfo2Dest);
                bool depthChanged = std::abs(depth - previousLfo2Depth) > 0.001f;
                bool enableChanged = (enabled != previousLfo2Enable);

                // Only update routings if something actually changed
                if (destinationChanged || depthChanged || enableChanged) {
                    previousLfo2Dest = destIndex;
                    previousLfo2Depth = depth;
                    previousLfo2Enable = enabled;

                    DBG("[MODULATION] LFO2 config changed - Dest: " + juce::String(destIndex) +
                        ", Depth: " + juce::String(depth) + ", Enabled: " + juce::String(enabled ? "true" : "false"));
                }

                // Map APVTS dropdown index to ModulationMatrix::Destination enum
                // This must match the mapping in ModulationSettingsOverlay.h
                if (destIndex > 0 && destinationChanged) {
                    braidy::ModulationMatrix::Destination dest;
                    bool validDest = true;
                    
                    switch (destIndex) {
                        case 1:  // "META" -> ALGORITHM_SELECTION (enum 0)
                            dest = braidy::ModulationMatrix::ALGORITHM_SELECTION;
                            break;
                        case 2:  // "Timbre" -> TIMBRE (enum 1)
                            dest = braidy::ModulationMatrix::TIMBRE;
                            break;
                        case 3:  // "Color" -> COLOR (enum 2)
                            dest = braidy::ModulationMatrix::COLOR;
                            break;
                        case 4:  // "FM" -> FM_AMOUNT (enum 3)
                            dest = braidy::ModulationMatrix::FM_AMOUNT;
                            break;
                        case 5:  // "Modulation" -> ENV_TIMBRE_AMOUNT (enum 12)
                            dest = braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT;
                            break;
                        case 6:  // "Coarse (Pitch)" -> PITCH (enum 4)
                            dest = braidy::ModulationMatrix::PITCH;
                            break;
                        case 7:  // "Fine (Detune)" -> DETUNE (enum 5)
                            dest = braidy::ModulationMatrix::DETUNE;
                            break;
                        case 8:  // "Coarse (Octave)" -> OCTAVE (enum 6)
                            dest = braidy::ModulationMatrix::OCTAVE;
                            break;
                        case 9:  // "Env Attack" -> ENV_ATTACK (enum 7)
                            dest = braidy::ModulationMatrix::ENV_ATTACK;
                            break;
                        case 10: // "Env Decay" -> ENV_DECAY (enum 8)
                            dest = braidy::ModulationMatrix::ENV_DECAY;
                            break;
                        case 11: // "Env Sustain" -> ENV_SUSTAIN (enum 9)
                            dest = braidy::ModulationMatrix::ENV_SUSTAIN;
                            break;
                        case 12: // "Env Release" -> ENV_RELEASE (enum 10)
                            dest = braidy::ModulationMatrix::ENV_RELEASE;
                            break;
                        default:
                            validDest = false;
                            DBG("[MODULATION] ERROR: No mapping defined for LFO2 APVTS index " + juce::String(destIndex));
                            break;
                    }

                    if (validDest) {
                        // Clear all routings for LFO 2 first
                        for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                            auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                            if (modulationMatrix_.getRouting(currentDest).sourceId == 1) {
                                modulationMatrix_.clearRouting(currentDest);
                            }
                        }
                        
                        // Set new routing
                        modulationMatrix_.setRouting(1, dest, depth);

                        // Log routing changes
                        if (dest == braidy::ModulationMatrix::TIMBRE) {
                            DBG("[MODULATION] LFO2 routed to TIMBRE");
                        }
                        else if (dest == braidy::ModulationMatrix::COLOR) {
                            DBG("[MODULATION] LFO2 routed to COLOR");
                        }
                        else if (dest == braidy::ModulationMatrix::FM_AMOUNT) {
                            DBG("[MODULATION] LFO2 routed to FM_AMOUNT");
                        }
                        else if (dest == braidy::ModulationMatrix::DETUNE) {
                            DBG("[MODULATION] LFO2 routed to DETUNE");
                        }
                        else if (dest == braidy::ModulationMatrix::OCTAVE) {
                            DBG("[MODULATION] LFO2 routed to OCTAVE");
                        }
                    }
                } else if (destIndex == 0 && destinationChanged) {
                    // destIndex == 0 means "None" selected - only clear if this is a change

                    // Clear base value tracking flags for any parameters that were modulated by LFO2
                    // No need to reset manual adjustment flags with Griddy's approach

                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 1) {
                            modulationMatrix_.clearRouting(currentDest);
                        }
                    }
                    DBG("[MODULATION] LFO2 destination set to None - cleared routings");
                } else if (!destinationChanged && depthChanged && destIndex > 0) {
                    // Just the depth changed - update amount without clearing routing
                    // Find which destination LFO2 is currently routed to
                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 1 &&
                            modulationMatrix_.getRouting(currentDest).enabled) {
                            // Update the amount for this existing routing
                            modulationMatrix_.setRouting(1, currentDest, depth);
                            DBG("[MODULATION] Updated LFO2 depth for existing routing to " + juce::String(depth));
                            break;
                        }
                    }
                }
            }
        } else {
            // Disable LFO 2
            lfo2.setEnabled(false);

            // Only clear routings if LFO2 was previously enabled
            if (previousLfo2Enable) {
                previousLfo2Enable = false;
                previousLfo2Dest = -999;  // Reset to impossible value

                // Clear all routings for LFO 2
                for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                    auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                    if (modulationMatrix_.getRouting(currentDest).sourceId == 1) {
                        modulationMatrix_.clearRouting(currentDest);
                    }
                }
                DBG("[MODULATION] LFO2 disabled - cleared all routings");
            }
        }
    }
}

juce::AudioProcessorEditor* BraidyAudioProcessor::createEditor()
{
    return new BraidyAudioProcessorEditor(*this);
}

void BraidyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state
    auto state = apvts_.copyState();
    
    // Add modulation matrix state
    auto modulationTree = state.getOrCreateChildWithName("ModulationMatrix", nullptr);
    modulationMatrix_.saveToValueTree(modulationTree);
    
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

// Get modulated parameter values for UI display
float BraidyAudioProcessor::getModulatedTimbre() const {
    // Griddy's approach: always get base value from APVTS
    float baseValue = apvts_.getRawParameterValue("param1")->load();

    // Apply modulation if active
    if (modulationMatrix_.isModulated(braidy::ModulationMatrix::TIMBRE)) {
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::TIMBRE, baseValue, 0.0f, 1.0f);
    }

    return baseValue;
}

float BraidyAudioProcessor::getModulatedColor() const {
    // Griddy's approach: always get base value from APVTS
    float baseValue = apvts_.getRawParameterValue("param2")->load();
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::COLOR, baseValue, 0.0f, 1.0f);
}

float BraidyAudioProcessor::getModulatedFM() const {
    // Griddy's approach: always get base value from APVTS
    float baseValue = apvts_.getRawParameterValue("fmAmount")->load();
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::FM_AMOUNT, baseValue, 0.0f, 1.0f);
}

float BraidyAudioProcessor::getModulatedFine() const {
    // Griddy's approach: always get base value from APVTS
    float baseValue = apvts_.getRawParameterValue("fineTune")->load();
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::DETUNE, baseValue, 0.0f, 1.0f);
}

float BraidyAudioProcessor::getModulatedCoarse() const {
    // Griddy's approach: always get base value from APVTS
    float baseValue = apvts_.getRawParameterValue("coarseTune")->load();
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::OCTAVE, baseValue, 0.0f, 1.0f);
}

float BraidyAudioProcessor::getModulatedTimbreMod() const {
    // Get the base parameter value
    if (auto* param = apvts_.getParameter("envTimbreAmount")) {
        float baseValue = param->getValue();
        
        // Apply modulation if active
        if (modulationMatrix_.isModulated(braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT)) {
            return modulationMatrix_.applyModulation(braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT, baseValue, 0.0f, 1.0f);
        }
        
        return baseValue;
    }
    
    return 0.0f;  // Default value when parameter not found
}

void BraidyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState && xmlState->hasTagName(apvts_.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xmlState);
        apvts_.replaceState(state);
        
        // Restore modulation matrix state
        auto modulationTree = state.getChildWithName("ModulationMatrix");
        if (modulationTree.isValid()) {
            modulationMatrix_.loadFromValueTree(modulationTree);
        }
        
        updateSynthesiserFromParameters();
        updateModulationFromParameters();
    }
}

std::vector<std::string> BraidyAudioProcessor::getAlgorithmNames()
{
    return BraidyAdapter::BraidsEngine::getAllAlgorithmNames();
}

void BraidyAudioProcessor::timerCallback()
{
    // Always update modulation to ensure LFO destination changes are synced
    updateModulationFromParameters();
    
    // Check for pending algorithm updates from META mode
    int pendingAlgorithm = pendingAlgorithmUpdate_.exchange(-1);
    if (pendingAlgorithm >= 0 && pendingAlgorithm <= 46) {
        // SAFETY: Double-check that FM is not an LFO destination before updating
        bool fmIsLfoDestination = false;
        if (auto* lfo1Dest = apvts_.getRawParameterValue("lfo1Dest")) {
            fmIsLfoDestination = (static_cast<int>(lfo1Dest->load()) == 4);
        }
        if (!fmIsLfoDestination) {
            if (auto* lfo2Dest = apvts_.getRawParameterValue("lfo2Dest")) {
                fmIsLfoDestination = (static_cast<int>(lfo2Dest->load()) == 4);
            }
        }
        
        // Only update algorithm if FM is not modulated (safety check)
        if (!fmIsLfoDestination) {
            // Update the APVTS parameter on the message thread (safe)
            if (auto* algorithmParam = apvts_.getParameter("algorithm")) {
                // Only update if the value actually changed to avoid recursion
                float currentValue = algorithmParam->getValue();
                float newValue = algorithmParam->convertTo0to1(pendingAlgorithm);
                if (std::abs(currentValue - newValue) > 0.001f) {
                    algorithmParam->setValueNotifyingHost(newValue);
                    DBG("[TIMER] Updated algorithm parameter to " + juce::String(pendingAlgorithm));
                }
            }
        }
        else {
            DBG("[TIMER] Skipped algorithm update - FM is LFO destination");
        }
    }
}

// Helper method to check if a parameter is being modulated
bool BraidyAudioProcessor::isParameterModulated(const juce::String& parameterID) const {
    // Map parameter IDs to modulation destinations
    if (parameterID == "fmAmount") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT);
    } else if (parameterID == "param1") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::TIMBRE);
    } else if (parameterID == "param2") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::COLOR);
    } else if (parameterID == "fineTune") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::DETUNE);
    } else if (parameterID == "coarseTune") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::OCTAVE);
    } else if (parameterID == "envTimbreAmount") {
        return modulationMatrix_.isModulated(braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT);
    }
    return false;
}

// Helper method to set the base value for a modulated parameter
void BraidyAudioProcessor::setModulatedParameterBaseValue(const juce::String& parameterID, float newBaseValue) {
    // Using Griddy's approach: Just update the APVTS parameter directly
    // No need for separate base value tracking
    if (auto* param = apvts_.getParameter(parameterID)) {
        param->setValueNotifyingHost(newBaseValue);
    }
}

void BraidyAudioProcessor::algorithmParameterChanged()
{
    // Called when algorithm parameter changes from any source (automation, UI, etc.)
    DBG("[ALGORITHM LISTENER] Algorithm parameter changed, forcing update");

    // Force immediate update on message thread
    juce::MessageManager::callAsync([this]() {
        updateSynthesiserFromParameters();
    });
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidyAudioProcessor();
}