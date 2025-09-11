#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "adapters/BraidsEngine.h"
#include <iostream>
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
    
    // FM Amount - In Braids, this is the internal envelope modulation of pitch
    // Range matches AD_FM setting (0-127 in hardware)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fmAmount", 1},
        "FM Amount",
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
        juce::StringArray{"None", "Algorithm (META)", "Timbre", "Color", "Pitch", "Volume"},
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
        juce::StringArray{"None", "Algorithm (META)", "Timbre", "Color", "Pitch", "Volume"},
        0  // Default to None
    ));
    
    return layout;
}

void BraidyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    std::cout << "[DEBUG] prepareToPlay: sampleRate=" << sampleRate 
              << " samplesPerBlock=" << samplesPerBlock << std::endl;
    synthesiser_->setCurrentPlaybackSampleRate(sampleRate);
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
                std::cout << "[DEBUG] ProcessBlock MIDI NoteOn: note=" << msg.getNoteNumber() 
                          << " vel=" << msg.getVelocity() << std::endl;
            } else if (msg.isNoteOff()) {
                std::cout << "[DEBUG] ProcessBlock MIDI NoteOff: note=" << msg.getNoteNumber() << std::endl;
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
        std::cout << "[DEBUG] Audio max sample: " << maxSample 
                  << " Active voices: " << synthesiser_->getActiveVoiceCount() << std::endl;
    }
    
    // Apply volume
    float volume = currentVolume_.load();
    buffer.applyGain(volume);
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
    auto* modAmountParam = apvts_.getRawParameterValue("modAmount");
    
    if (algorithmParam && param1 && param2 && volumeParam) {
        // For AudioParameterChoice, the raw value is already the index (0-46)
        // No conversion needed
        float algorithmIndex = algorithmParam->load();
        int newAlgorithm = static_cast<int>(std::round(algorithmIndex));
        
        // Debug output
        static int debugCount = 0;
        if (++debugCount % 50 == 0) {  // Log every 50th call to avoid flooding
            std::cout << "[DEBUG] Algorithm index: raw=" << algorithmIndex 
                      << " -> rounded=" << newAlgorithm << std::endl;
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
        
        // Color modulation
        if (modulationMatrix_.isModulated(braidy::ModulationMatrix::COLOR)) {
            newParam2 = modulationMatrix_.applyModulation(
                braidy::ModulationMatrix::COLOR, newParam2, 0.0f, 1.0f);
        }
        
        // META mode algorithm modulation
        auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
        bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;
        
        if (metaMode && modulationMatrix_.isModulated(braidy::ModulationMatrix::ALGORITHM_SELECTION)) {
            // Apply algorithm modulation in META mode
            newAlgorithm = modulationMatrix_.applyModulationInt(
                braidy::ModulationMatrix::ALGORITHM_SELECTION, newAlgorithm, 0, 46);
        }
        
        // Update if changed with safer voice management
        if (newAlgorithm != currentAlgorithm_) {
            std::cout << "[DEBUG] PluginProcessor: Algorithm changed from " << currentAlgorithm_ 
                      << " to " << newAlgorithm << std::endl;
            
            // THREAD SAFETY FIX: Stop all voices before changing algorithm to prevent crashes
            // This is especially important for algorithms like PLUK/BELL that can crash during voice switches
            synthesiser_->allNotesOff(0, true); // Force stop all active voices
            
            currentAlgorithm_ = newAlgorithm;
            
            // Set algorithm on synthesiser (this will update all voices)
            synthesiser_->setAlgorithm(newAlgorithm);
            
            std::cout << "[DEBUG] PluginProcessor: Algorithm change completed safely" << std::endl;
        }
        
        if (newParam1 != currentParam1_ || newParam2 != currentParam2_) {
            std::cout << "[DEBUG] PluginProcessor: Parameters changed - param1: " 
                      << currentParam1_ << " -> " << newParam1 
                      << ", param2: " << currentParam2_ << " -> " << newParam2 << std::endl;
            currentParam1_ = newParam1;
            currentParam2_ = newParam2;
            synthesiser_->setParameters(newParam1, newParam2);
        }
        
        currentVolume_ = newVolume;
        
        // Apply tuning parameters to all voices
        if (fineTuneParam && coarseTuneParam) {
            // Fine tune: -2 to +2 semitones for fine control
            float fineTune = (fineTuneParam->load() - 0.5f) * 4.0f;
            
            // Coarse tune: Braids hardware uses octave switching
            // AudioParameterChoice returns the index directly (0-4), convert to octave offset
            float octaveIndexFloat = coarseTuneParam->load();
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
        
        // Apply FM amount and meta modulation mode
        if (fmAmountParam) {
            float fmAmount = fmAmountParam->load();
            
            // Apply FM modulation
            if (modulationMatrix_.isModulated(braidy::ModulationMatrix::FM_AMOUNT)) {
                fmAmount = modulationMatrix_.applyModulation(
                    braidy::ModulationMatrix::FM_AMOUNT, fmAmount, 0.0f, 1.0f);
            }
            
            // Meta mode (algorithm modulation)
            auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
            bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;
            
            // Pass FM value to voices - let BraidsEngine handle META mode algorithm switching internally
            for (int i = 0; i < synthesiser_->getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidyAdapter::BraidsVoice*>(synthesiser_->getVoice(i))) {
                    voice->setMetaMode(metaMode);  // Set meta mode first
                    voice->setFMAmount(fmAmount);  // Then set FM amount which triggers algorithm change in meta mode
                    
                    // Update current algorithm for display based on what the engine is actually using
                    if (metaMode) {
                        // In META mode, FM value (0-1) maps to algorithm (0-46)
                        // Calculate the algorithm directly from the FM value
                        int targetAlgorithm = static_cast<int>(fmAmount * 46.0f);
                        targetAlgorithm = juce::jlimit(0, 46, targetAlgorithm);
                        
                        // Update the algorithm if it changed
                        if (targetAlgorithm != currentAlgorithm_) {
                            currentAlgorithm_ = targetAlgorithm;
                            synthesiser_->setAlgorithm(targetAlgorithm);
                            
                            // Log the change for debugging
                            DBG("[META MODE] Algorithm changed to: " + 
                                juce::String(targetAlgorithm) + " (FM: " + juce::String(fmAmount) + ")");
                        }
                    }
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
            auto* lfo1Dest = apvts_.getRawParameterValue("lfo1Destination");
            
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
                braidy::ModulationMatrix::Destination dest = 
                    static_cast<braidy::ModulationMatrix::Destination>(destIndex);
                
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
            auto* lfo2Dest = apvts_.getRawParameterValue("lfo2Destination");
            
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
                braidy::ModulationMatrix::Destination dest = 
                    static_cast<braidy::ModulationMatrix::Destination>(destIndex);
                
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
    return modulationMatrix_.applyModulation(braidy::ModulationMatrix::TIMBRE, baseValue, 0.0f, 1.0f);
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
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::PITCH, baseValue, 0.0f, 1.0f);
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
    if (auto* param = apvts_.getParameter("timbreMod")) {
        float baseValue = param->getValue();
        return modulationMatrix_.applyModulation(braidy::ModulationMatrix::ENV_TIMBRE_AMOUNT, baseValue, 0.0f, 1.0f);
    }
    return 0.5f;
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