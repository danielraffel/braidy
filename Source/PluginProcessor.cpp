#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BraidyCore/BraidyResources.h"

//==============================================================================
BraidyAudioProcessor::BraidyAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts_(*this, nullptr, "BraidyParameters", createParameterLayout())
{
    // Initialize Braidy components
    braidy_settings_ = std::make_unique<braidy::BraidySettings>();
    voice_manager_ = std::make_unique<braidy::VoiceManager>();
    
    braidy_settings_->Init();
    voice_manager_->Init();
    
    // Set global settings pointer
    braidy::global_settings = braidy_settings_.get();
    
    // Initialize resources
    braidy::InitializeResources();
}

BraidyAudioProcessor::~BraidyAudioProcessor() {
    braidy::global_settings = nullptr;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout BraidyAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Create parameters for each Braidy parameter
    for (int i = 0; i < static_cast<int>(braidy::BraidyParameter::PARAMETER_COUNT); ++i) {
        auto param = static_cast<braidy::BraidyParameter>(i);
        const auto& info = braidy::BraidySettings::GetParameterInfo(param);
        
        // Create parameter ID
        juce::String paramId = info.short_name;
        paramId = paramId.toLowerCase().replaceCharacter(' ', '_');
        
        if (info.is_discrete) {
            // Discrete parameter (integer choices)
            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID{paramId, 1},
                info.name,
                static_cast<int>(info.min_value),
                static_cast<int>(info.max_value),
                static_cast<int>(info.default_value)
            ));
        } else {
            // Continuous parameter (float)
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{paramId, 1},
                info.name,
                juce::NormalisableRange<float>(info.min_value, info.max_value),
                info.default_value
            ));
        }
    }
    
    return layout;
}

//==============================================================================
const juce::String BraidyAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool BraidyAudioProcessor::acceptsMidi() const {
    return true;  // Braidy is a synthesizer, needs MIDI input
}

bool BraidyAudioProcessor::producesMidi() const {
    return false;
}

bool BraidyAudioProcessor::isMidiEffect() const {
    return false;  // It's a synthesizer, not a MIDI effect
}

double BraidyAudioProcessor::getTailLengthSeconds() const {
    return 2.0;  // Allow for envelope decay
}

int BraidyAudioProcessor::getNumPrograms() {
    return 1;
}

int BraidyAudioProcessor::getCurrentProgram() {
    return 0;
}

void BraidyAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String BraidyAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return "Default";
}

void BraidyAudioProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void BraidyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);
    
    voice_manager_->SetSampleRate(static_cast<float>(sampleRate));
}

void BraidyAudioProcessor::releaseResources() {
    voice_manager_->AllNotesOff();
}

bool BraidyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Only support stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    
    // Don't need input
    return true;
}

void BraidyAudioProcessor::updateBraidyFromAPVTS() {
    // Update Braidy settings from JUCE parameters
    for (int i = 0; i < static_cast<int>(braidy::BraidyParameter::PARAMETER_COUNT); ++i) {
        auto param = static_cast<braidy::BraidyParameter>(i);
        const auto& info = braidy::BraidySettings::GetParameterInfo(param);
        
        juce::String paramId = info.short_name;
        paramId = paramId.toLowerCase().replaceCharacter(' ', '_');
        
        if (auto* apvtsParam = apvts_.getParameter(paramId)) {
            float normalizedValue = apvtsParam->getValue();
            float actualValue = info.min_value + normalizedValue * (info.max_value - info.min_value);
            braidy_settings_->SetParameter(param, actualValue);
        }
    }
    
    // Update parameter smoothing
    braidy_settings_->UpdateSmoothers();
}

void BraidyAudioProcessor::processMidiMessage(const juce::MidiMessage& message) {
    int channel = message.getChannel() - 1;  // Convert to 0-based
    
    if (message.isNoteOn()) {
        int midiNote = message.getNoteNumber();
        float velocity = message.getFloatVelocity();
        voice_manager_->NoteOn(midiNote, velocity, channel);
    }
    else if (message.isNoteOff()) {
        int midiNote = message.getNoteNumber();
        voice_manager_->NoteOff(midiNote, channel);
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff()) {
        voice_manager_->AllNotesOff();
    }
    else if (message.isPitchWheel()) {
        float pitchBend = (message.getPitchWheelValue() - 8192.0f) / 8192.0f;  // -1 to 1
        voice_manager_->SetPitchBend(pitchBend, channel);
    }
    else if (message.isAftertouch()) {
        float aftertouch = message.getAfterTouchValue() / 127.0f;  // 0 to 1
        voice_manager_->SetAftertouch(aftertouch, -1, channel);  // Channel aftertouch
    }
    else if (message.isChannelPressure()) {
        float pressure = message.getChannelPressureValue() / 127.0f;  // 0 to 1
        voice_manager_->SetAftertouch(pressure, -1, channel);  // Channel pressure
    }
    else if (message.isController()) {
        int ccNumber = message.getControllerNumber();
        float ccValue = message.getControllerValue() / 127.0f;  // 0 to 1
        
        // Handle special controllers
        if (ccNumber == 1) {  // Mod wheel
            voice_manager_->SetModWheel(ccValue, channel);
        } else {
            voice_manager_->SetCC(ccNumber, ccValue, channel);
        }
    }
}

void BraidyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // Clear any input (we're a synthesizer)
    buffer.clear();
    
    // Update parameters
    updateBraidyFromAPVTS();
    voice_manager_->UpdateFromSettings(*braidy_settings_);
    
    // Process MIDI messages
    for (const auto metadata : midiMessages) {
        processMidiMessage(metadata.getMessage());
    }
    
    // Generate audio
    voice_manager_->Process(buffer, buffer.getNumSamples());
}

//==============================================================================
bool BraidyAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* BraidyAudioProcessor::createEditor() {
    return new BraidyAudioProcessorEditor(*this);
}

//==============================================================================
void BraidyAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // Save APVTS state
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BraidyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // Restore APVTS state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts_.state.getType())) {
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BraidyAudioProcessor();
}