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
    preset_manager_ = std::make_unique<braidy::PresetManager>();
    
    // Initialize performance optimization variables
    parameter_update_pending_ = false;
    samples_since_parameter_update_ = 0;
    
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
    // Performance optimization: only update parameters if they've changed
    if (!parameter_update_pending_) {
        return;
    }
    
    DBG("=== PROCESSOR UPDATE BRAIDY FROM APVTS DEBUG ===");
    
    bool shapeParameterFound = false;
    float oldShapeValue = braidy_settings_->GetParameter(braidy::BraidyParameter::SHAPE);
    
    // Update Braidy settings from JUCE parameters
    for (int i = 0; i < static_cast<int>(braidy::BraidyParameter::PARAMETER_COUNT); ++i) {
        auto param = static_cast<braidy::BraidyParameter>(i);
        const auto& info = braidy::BraidySettings::GetParameterInfo(param);
        
        juce::String paramId = info.short_name;
        paramId = paramId.toLowerCase().replaceCharacter(' ', '_');
        
        if (auto* apvtsParam = apvts_.getParameter(paramId)) {
            float normalizedValue = apvtsParam->getValue();
            float actualValue = info.min_value + normalizedValue * (info.max_value - info.min_value);
            
            if (param == braidy::BraidyParameter::SHAPE) {
                shapeParameterFound = true;
                DBG("SHAPE parameter found!");
                DBG("  Parameter ID: " + paramId);
                DBG("  Normalized value: " + juce::String(normalizedValue, 4));
                DBG("  Min value: " + juce::String(info.min_value, 2));
                DBG("  Max value: " + juce::String(info.max_value, 2));
                DBG("  Actual value: " + juce::String(actualValue, 2));
                DBG("  Old Braidy shape value: " + juce::String(oldShapeValue, 2));
            }
            
            braidy_settings_->SetParameter(param, actualValue);
            
            if (param == braidy::BraidyParameter::SHAPE) {
                float newShapeValue = braidy_settings_->GetParameter(braidy::BraidyParameter::SHAPE);
                DBG("  New Braidy shape value: " + juce::String(newShapeValue, 2));
                DBG("  Shape as int: " + juce::String(static_cast<int>(newShapeValue)));
            }
        }
    }
    
    if (!shapeParameterFound) {
        DBG("WARNING: SHAPE parameter was not found!");
    }
    
    // Update parameter smoothing
    braidy_settings_->UpdateSmoothers();
    
    parameter_update_pending_ = false;
    DBG("=== END PROCESSOR UPDATE DEBUG ===");
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
    
    // Performance optimization: mark parameter update as pending
    parameter_update_pending_ = true;
    samples_since_parameter_update_ += buffer.getNumSamples();
    
    // Update parameters (optimized to skip if no changes)
    bool parameters_were_updated = parameter_update_pending_;
    updateBraidyFromAPVTS();
    
    // Only update voice settings if parameters changed or periodically
    if (parameters_were_updated || samples_since_parameter_update_ > 1024) {
        float currentShape = braidy_settings_->GetParameter(braidy::BraidyParameter::SHAPE);
        DBG("=== PROCESSOR PROCESS BLOCK DEBUG ===");
        DBG("Parameters were updated: " + juce::String(parameters_were_updated ? "true" : "false"));
        DBG("Current shape value being passed to voice manager: " + juce::String(currentShape, 2));
        DBG("Shape as int: " + juce::String(static_cast<int>(currentShape)));
        
        voice_manager_->UpdateFromSettings(*braidy_settings_);
        samples_since_parameter_update_ = 0;
        
        DBG("=== END PROCESSOR PROCESS BLOCK DEBUG ===");
    }
    
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

void BraidyAudioProcessor::loadPreset(size_t index) {
    const braidy::BraidyPreset* preset = preset_manager_->GetPreset(index);
    if (preset && preset->IsValid()) {
        preset->ApplyToSettings(*braidy_settings_);
        
        // Update APVTS to reflect new parameter values
        for (int i = 0; i < static_cast<int>(braidy::BraidyParameter::PARAMETER_COUNT); ++i) {
            auto param = static_cast<braidy::BraidyParameter>(i);
            const auto& info = braidy::BraidySettings::GetParameterInfo(param);
            
            juce::String paramId = info.short_name;
            paramId = paramId.toLowerCase().replaceCharacter(' ', '_');
            
            if (auto* apvtsParam = apvts_.getParameter(paramId)) {
                float value = braidy_settings_->GetParameter(param);
                float normalized = (value - info.min_value) / (info.max_value - info.min_value);
                apvtsParam->setValueNotifyingHost(normalized);
            }
        }
        
        preset_manager_->SetCurrentPreset(index);
        parameter_update_pending_ = true;
    }
}

void BraidyAudioProcessor::saveCurrentAsPreset(const juce::String& name) {
    preset_manager_->AddPreset(name.toStdString(), *braidy_settings_);
}

void BraidyAudioProcessor::optimizeForRealtime() {
    // Performance optimization hints
    
    // Optimize voice manager for realtime use
    voice_manager_->SetMaxPolyphony(16);  // Reasonable limit for realtime performance
    
    // Prepare for consistent block sizes
    prepareToPlay(getSampleRate(), getBlockSize());
    
    // Note: Thread priority and denormal optimization are handled automatically by JUCE's audio thread
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BraidyAudioProcessor();
}