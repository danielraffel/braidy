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
    
    if (algorithmParam && param1 && param2 && volumeParam) {
        int newAlgorithm = static_cast<int>(algorithmParam->load());
        float newParam1 = param1->load();
        float newParam2 = param2->load();
        float newVolume = volumeParam->load();
        
        // Update if changed
        if (newAlgorithm != currentAlgorithm_) {
            currentAlgorithm_ = newAlgorithm;
            synthesiser_->setAlgorithm(newAlgorithm);
        }
        
        if (newParam1 != currentParam1_ || newParam2 != currentParam2_) {
            currentParam1_ = newParam1;
            currentParam2_ = newParam2;
            synthesiser_->setParameters(newParam1, newParam2);
        }
        
        currentVolume_ = newVolume;
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