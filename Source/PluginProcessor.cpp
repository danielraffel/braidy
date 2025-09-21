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
}

BraidyAudioProcessor::~BraidyAudioProcessor() = default;

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
        false  // Default to off
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
        false
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
        false
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
                // Track active held notes for stuck-note protection
                heldNotes_++;
                samplesSinceLastNoteEvent_ = 0;
            } else if (msg.isNoteOff()) {
                DBG("[DEBUG] ProcessBlock MIDI NoteOff: note=" + juce::String(msg.getNoteNumber()));
                // Track releases
                heldNotes_ = std::max(0, heldNotes_.load() - 1);
                samplesSinceLastNoteEvent_ = 0;
            }
        }
    }
    
    // Clear any input audio
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // CRITICAL FIX: Process modulation LFOs BEFORE applying parameters
    // This ensures META mode uses fresh LFO values, not stale ones from previous block
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

    // Only update modulation configuration if LFO parameters changed
    // This is lightweight and needed for real-time modulation routing
    static int modUpdateCounter = 0;
    if (++modUpdateCounter >= 32) {  // Update every 32 blocks to reduce CPU load
        modUpdateCounter = 0;
        updateModulationFromParameters();
    }

    // Update synthesizer parameters - this includes real-time modulation
    // The recursion guard prevents issues if this is called from parameter listeners
    // NOTE: This must happen AFTER LFOs are processed to use current values
    updateSynthesiserFromParameters();
    
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

    // Stuck-note watchdog: If META is enabled and FM is being modulated,
    // and there are no held notes but voices are still active for a while,
    // force an all-notes-off to clear any lingering voices.
    // This specifically addresses rare edge-cases during rapid META switching.
    samplesSinceLastNoteEvent_ += buffer.getNumSamples();
    const double sr = getSampleRate();
    const int thresholdSamples = static_cast<int>(0.25 * sr); // 250ms without note activity
    const bool metaOn = isMetaModeEnabled();
    const bool fmModulated = modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT) ||
                             modulationMatrix_.isModulated(braidy::ModulationMatrix::ALGORITHM_SELECTION);
    if (metaOn && fmModulated && heldNotes_.load() == 0 && samplesSinceLastNoteEvent_.load() > thresholdSamples) {
        const int active = synthesiser_->getActiveVoiceCount();
        if (active > 0) {
            DBG("[WATCHDOG] No held notes, but " + juce::String(active) +
                " voices active in META+FM. Forcing allNotesOff.");
            // Immediate cutoff to ensure no hangs
            synthesiser_->allNotesOff(0, false /* allowTailOff */);
        }
    }
}

void BraidyAudioProcessor::updateSynthesiserFromParameters()
{
    // Prevent recursive calls - this is critical to avoid freezes
    bool expected = false;
    if (!isUpdatingParameters_.compare_exchange_strong(expected, true)) {
        return; // Already updating, skip to prevent recursion
    }
    
    // RAII guard to ensure flag is reset even if exception occurs
    struct UpdateGuard {
        std::atomic<bool>& flag;
        ~UpdateGuard() { flag = false; }
    } guard{isUpdatingParameters_};
    
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
        
        // Debug output
        static int debugCount = 0;
        if (++debugCount % 50 == 0) {  // Log every 50th call to avoid flooding
            DBG("[DEBUG] Algorithm index: raw=" + juce::String(algorithmIndex) +
                " -> rounded=" + juce::String(newAlgorithm));
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
            float fmAmount = fmAmountParam->load();
            
            // Check META mode first
            auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
            bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;

            // Store the original FM value before any modulation
            float originalFmAmount = fmAmount;

            // CRITICAL FIX: Only apply FM modulation when META mode is OFF
            // When META mode is ON, FM controls algorithm selection and shouldn't be modulated
            if (!metaMode && modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                fmAmount = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::FM_AMOUNT, fmAmount, 0.0f, 1.0f);

                DBG("[FM MODULATION] Applied LFO modulation - FM: " + juce::String(fmAmount));
            } else if (metaMode && modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                DBG("[META MODE] FM modulation blocked - META mode controls FM for algorithm selection");
            }
            
            // Debug META mode state
            static int metaDebugCounter = 0;
            if (++metaDebugCounter % 100 == 0) {  // Log every 100 blocks
                if (metaMode) {
                    DBG("[META MODE] Enabled - FM: " + juce::String(fmAmount) +
                        ", FM modulated: " + juce::String(modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT) ? "true" : "false"));
                }
            }
            
            // META mode algorithm selection - RE-ENABLED with crash-safe approach
            if (metaMode && metaModeParam && metaModeParam->load() > 0.5f) {
                int targetAlgorithm = newAlgorithm; // Start with current algorithm selection
                bool shouldModulateAlgorithm = false;
                
                // Priority 1: Check for LFO modulation to Algorithm destination
                if (modulationMatrix_.isModulated(braidy::ModulationMatrix::ALGORITHM_SELECTION)) {
                    // LFO directly modulates algorithm selection
                    targetAlgorithm = modulationMatrix_.applyModulationInt(
                        braidy::ModulationMatrix::ALGORITHM_SELECTION, newAlgorithm, 0, 46);
                    shouldModulateAlgorithm = true;
                    
                    DBG("[META MODULATION] LFO modulating algorithm to index " + juce::String(targetAlgorithm));
                }
                // Priority 2: Use FM to control algorithm when META is enabled
                // In META mode, FM parameter ALWAYS controls algorithm selection (even at 0)
                else {
                    // For META mode algorithm selection, calculate what the modulated FM would be
                    // This ensures the LED display cycles even when FM modulation is blocked
                    float metaFmValue = originalFmAmount;
                    if (modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                        // Calculate modulated FM value for display purposes
                        metaFmValue = modulationMatrix_.applyModulation(
                            braidy::ModulationMatrix::FM_AMOUNT, originalFmAmount, 0.0f, 1.0f);
                    }

                    // Traditional META mode: FM parameter controls algorithm
                    // FM value of 0.0 = algorithm 0, FM value of 1.0 = algorithm 46
                    // ENHANCED BOUNDS CHECKING: Validate FM value before conversion
                    if (!std::isfinite(metaFmValue)) {
                        metaFmValue = 0.0f;  // Default to CSAW on invalid values
                    }
                    metaFmValue = juce::jlimit(0.0f, 1.0f, metaFmValue);  // Pre-clamp for safety
                    targetAlgorithm = static_cast<int>(metaFmValue * 46.0f);
                    targetAlgorithm = juce::jlimit(0, 46, targetAlgorithm);  // Double-check bounds
                    shouldModulateAlgorithm = true;
                    
                    // Always log when META mode is active to help debug
                    static int metaLogCounter = 0;
                    if (++metaLogCounter % 50 == 0) {  // Log every 50 blocks
                        DBG("[META MODULATION] FM controlling algorithm to index " + juce::String(targetAlgorithm) +
                            " (original FM=" + juce::String(originalFmAmount) +
                            ", meta FM=" + juce::String(metaFmValue) +
                            ", modulated=" + juce::String(modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT) ? "true" : "false") + ")");
                    }
                }
                // Priority 3: No modulation active - stay on current algorithm, don't cycle
                
                // Apply algorithm change only if we should modulate and target changed
                if (shouldModulateAlgorithm) {
                    // Store the modulated algorithm for display purposes
                    metaModeAlgorithm_.store(targetAlgorithm);
                    
                    // Only actually change the synthesis algorithm if it's significantly different
                    // This prevents rapid switching that causes crashes
                    if (targetAlgorithm != currentAlgorithm_) {
                        // Add hysteresis: only switch if we've moved by more than 1 algorithm
                        // or if enough time has passed since last switch
                        static int lastSwitchCounter = 0;
                        static int lastAlgorithm = -1;
                        
                        // Increment counter every process block
                        lastSwitchCounter++;
                        
                        // MUSICAL META MODE: Enhanced rate limiting for stable, responsive algorithm switching
                        // Balance between "hyper" character and crash prevention
                        bool isTempoSynced = false;
                        bool isModulated = modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT);
                        if (auto* lfo1TempoSync = apvts_.getRawParameterValue("lfo1TempoSync")) {
                            isTempoSynced = lfo1TempoSync->load() > 0.5f;
                        }
                        // ENHANCED RATE LIMITING: Increase limits when modulated to prevent crashes
                        const int MIN_BLOCKS_BETWEEN_SWITCHES = isModulated ?
                            (isTempoSynced ? 4 : 3) :  // More conservative when modulated
                            (isTempoSynced ? 2 : 1);    // Original values for manual control

                        bool shouldSwitch = false;

                        // Allow immediate switch for any algorithm change (more musical)
                        if (targetAlgorithm != currentAlgorithm_) {
                            shouldSwitch = true;
                        }
                        // Still respect minimal rate limiting to prevent audio thread issues
                        if (lastSwitchCounter < MIN_BLOCKS_BETWEEN_SWITCHES) {
                            shouldSwitch = false;
                        }
                        
                        if (shouldSwitch) {
                            // META MODE: Allow algorithm switching even with active voices
                            // This ensures real-time response and smooth transitions
                            if (synthesiser_ && targetAlgorithm >= 0 && targetAlgorithm <= 46) {
                                // Update algorithm immediately for real-time response
                                currentAlgorithm_ = targetAlgorithm;
                                newAlgorithm = targetAlgorithm;

                                try {
                                    // Extra safety: Check if we're switching too rapidly
                                    static auto lastSafeSwitch = std::chrono::steady_clock::now();
                                    auto now = std::chrono::steady_clock::now();
                                    auto timeSinceLastSwitch = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSafeSwitch);

                                    // ENHANCED PROTECTION: Increase time threshold for modulated scenarios
                                    // This prevents crashes while maintaining musicality
                                    int minSwitchDelayMs = 5;  // Default for manual changes
                                    if (modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                                        // Increase protection for modulated scenarios
                                        minSwitchDelayMs = isTempoSynced ? 20 : 15;  // Much safer threshold
                                    }

                                    if (timeSinceLastSwitch.count() < minSwitchDelayMs) {
                                        // Skip this switch to prevent crash - but don't exit entire function
                                        // Just skip the algorithm change and continue processing
                                        lastSwitchCounter = 0;  // Reset counter to try again soon
                                        lastAlgorithm = currentAlgorithm_;  // Keep current algorithm
                                    } else {

                                        synthesiser_->setAlgorithm(targetAlgorithm);

                                        // CRITICAL: Set parameters for the new algorithm
                                        synthesiser_->setParameters(newParam1, newParam2);

                                        lastSafeSwitch = now;
                                    }

                                    // Log the change with voice count for debugging
                                    int activeVoices = synthesiser_->getActiveVoiceCount();
                                    DBG("[META MODE] Switched to algorithm " + juce::String(targetAlgorithm) +
                                        " (voices: " + juce::String(activeVoices) +
                                        ") with params (" + juce::String(newParam1) +
                                        ", " + juce::String(newParam2) + ")");
                                } catch (const std::exception& e) {
                                    DBG("[ERROR] Failed to set algorithm " + juce::String(targetAlgorithm) +
                                        " - Exception: " + juce::String(e.what()));
                                    // Revert to safe state
                                    currentAlgorithm_ = 0;  // CSAW
                                    try {
                                        synthesiser_->setAlgorithm(0);
                                    } catch (...) {}
                                } catch (...) {
                                    DBG("[ERROR] Unknown exception setting algorithm " + juce::String(targetAlgorithm));
                                    // Revert to safe state
                                    currentAlgorithm_ = 0;  // CSAW
                                    try {
                                        synthesiser_->setAlgorithm(0);
                                    } catch (...) {}
                                }

                                lastAlgorithm = targetAlgorithm;
                                lastSwitchCounter = 0;
                            }
                        }
                    }
                }
            }
            else {
                // Not in META mode, ensure we're using the selected algorithm
                if (newAlgorithm != currentAlgorithm_) {
                    DBG("[ALGORITHM CHANGE] Switching from " + juce::String(currentAlgorithm_) +
                        " to " + juce::String(newAlgorithm) + " (normal mode)");

                    // REMOVED allNotesOff() - allow smooth transition with active voices
                    // This ensures continuous sound when changing algorithms
                    currentAlgorithm_ = newAlgorithm;
                    synthesiser_->setAlgorithm(newAlgorithm);

                    // CRITICAL FIX: Use NEW parameters (newParam1, newParam2) not old ones!
                    // This ensures the new algorithm gets the correct parameters immediately
                    synthesiser_->setParameters(newParam1, newParam2);

                    // DON'T update currentParam1_ and currentParam2_ here!
                    // Let the normal parameter update logic handle it below
                    // This prevents the parameter check from being skipped

                    DBG("[ALGORITHM CHANGE] Parameters set to (" +
                        juce::String(newParam1) + ", " + juce::String(newParam2) + ")");
                }
            }
            
            // Pass values to voices
            for (int i = 0; i < synthesiser_->getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidyAdapter::BraidsVoice*>(synthesiser_->getVoice(i))) {
                    voice->setMetaMode(metaMode);
                    voice->setFMAmount(fmAmount);
                }
            }
        }
    }
}

void BraidyAudioProcessor::updateModulationFromParameters()
{
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
                // Map APVTS dropdown index to ModulationMatrix::Destination enum
                // This must match the mapping in ModulationSettingsOverlay.h
                if (destIndex > 0) {
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
                            DBG("[MODULATION] ERROR: No mapping defined for APVTS index " + juce::String(destIndex));
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
                    }
                } else {
                    // destIndex == 0 means "None" selected - clear all routings for LFO 1
                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 0) {
                            modulationMatrix_.clearRouting(currentDest);
                        }
                    }
                }
            }
        } else {
            // Disable LFO 1
            lfo1.setEnabled(false);
            
            // Clear all routings for LFO 1
            for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                if (modulationMatrix_.getRouting(currentDest).sourceId == 0) {
                    modulationMatrix_.clearRouting(currentDest);
                }
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
                // Map APVTS dropdown index to ModulationMatrix::Destination enum
                // This must match the mapping in ModulationSettingsOverlay.h
                if (destIndex > 0) {
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
                            DBG("[MODULATION] ERROR: No mapping defined for APVTS index " + juce::String(destIndex));
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
                    }
                } else {
                    // destIndex == 0 means "None" selected - clear all routings for LFO 2
                    for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                        auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                        if (modulationMatrix_.getRouting(currentDest).sourceId == 1) {
                            modulationMatrix_.clearRouting(currentDest);
                        }
                    }
                }
            }
        } else {
            // Disable LFO 2
            lfo2.setEnabled(false);
            
            // Clear all routings for LFO 2
            for (int i = 0; i < static_cast<int>(braidy::ModulationMatrix::NUM_DESTINATIONS); ++i) {
                auto currentDest = static_cast<braidy::ModulationMatrix::Destination>(i);
                if (modulationMatrix_.getRouting(currentDest).sourceId == 1) {
                    modulationMatrix_.clearRouting(currentDest);
                }
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
    float baseValue = apvts_.getRawParameterValue("param1")->load();
    float modulatedValue = modulationMatrix_.applyModulation(braidy::ModulationMatrix::TIMBRE, baseValue, 0.0f, 1.0f);
    
    if (std::abs(modulatedValue - baseValue) > 0.001f) {
        DBG("[MODULATION DEBUG] Timbre - Base: " + juce::String(baseValue) + 
            ", Modulated: " + juce::String(modulatedValue));
    }
    
    return modulatedValue;
}

float BraidyAudioProcessor::getModulatedColor() const {
    float baseValue = apvts_.getRawParameterValue("param2")->load();
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::COLOR, baseValue, 0.0f, 1.0f);
}

float BraidyAudioProcessor::getModulatedFM() const {
    if (auto* param = apvts_.getParameter("fmAmount")) {
        float baseValue = param->getValue();
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::FM_AMOUNT, baseValue, 0.0f, 1.0f);
    }
    return 0.0f;
}

float BraidyAudioProcessor::getModulatedFine() const {
    if (auto* param = apvts_.getParameter("fineTune")) {
        float baseValue = param->getValue();
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::DETUNE, baseValue, 0.0f, 1.0f);
    }
    return 0.5f;
}

float BraidyAudioProcessor::getModulatedCoarse() const {
    if (auto* param = apvts_.getParameter("coarseTune")) {
        float baseValue = param->getValue();
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::OCTAVE, baseValue, 0.0f, 1.0f);
    }
    return 0.5f;
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

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidyAudioProcessor();
}
