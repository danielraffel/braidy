#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "adapters/BraidsEngine.h"
#include <iostream>

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
    
    // Process modulation LFOs (Item #2: Connect LFO processing to audio thread)
    double bpm = 120.0; // Default BPM
    if (auto* playHead = getPlayHead()) {
        if (auto positionInfo = playHead->getPosition()) {
            if (positionInfo->getBpm().hasValue()) {
                bpm = *positionInfo->getBpm();
            }
        }
    }
    modulationMatrix_.processBlock(getSampleRate(), buffer.getNumSamples(), bpm);
    
    // Process MIDI and generate audio using JUCE's built-in synthesiser renderNextBlock
    synthesiser_->renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
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
        
        // Update if changed
        if (newAlgorithm != currentAlgorithm_) {
            std::cout << "[DEBUG] PluginProcessor: Algorithm changed from " << currentAlgorithm_ 
                      << " to " << newAlgorithm << std::endl;
            currentAlgorithm_ = newAlgorithm;
            synthesiser_->setAlgorithm(newAlgorithm);
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
            
            // Meta mode (algorithm modulation)
            auto* metaModeParam = apvts_.getRawParameterValue("metaMode");
            bool metaMode = metaModeParam ? (metaModeParam->load() > 0.5f) : false;
            
            for (int i = 0; i < synthesiser_->getNumVoices(); ++i) {
                if (auto* voice = dynamic_cast<BraidyAdapter::BraidsVoice*>(synthesiser_->getVoice(i))) {
                    voice->setFMAmount(fmAmount);
                    voice->setMetaMode(metaMode);
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
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BraidyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState && xmlState->hasTagName(apvts_.state.getType())) {
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
        updateSynthesiserFromParameters();
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