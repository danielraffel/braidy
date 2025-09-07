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
    waveform_state_manager_ = std::make_unique<braidy::WaveformStateManager>();
    modulation_matrix_ = std::make_unique<braidy::ModulationMatrix>();
    
    // Initialize performance optimization variables
    parameter_update_pending_ = false;
    samples_since_parameter_update_ = 0;
    
    braidy_settings_->Init();
    voice_manager_->Init();
    waveform_state_manager_->Init();
    
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
    
    // Update LFOs and apply modulation
    if (auto* playHead = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playHead->getCurrentPosition(info)) {
            double bpm = info.bpm > 0 ? info.bpm : 120.0;
            
            // Advance LFOs
            modulation_matrix_->getLFO(0).advance(getSampleRate(), buffer.getNumSamples(), bpm);
            modulation_matrix_->getLFO(1).advance(getSampleRate(), buffer.getNumSamples(), bpm);
            
            // Apply LFO modulation to parameters
            for (int dest = 0; dest < braidy::ModulationMatrix::NUM_DESTINATIONS; ++dest) {
                auto destination = static_cast<braidy::ModulationMatrix::Destination>(dest);
                float modValue = modulation_matrix_->getModulation(destination);
                
                // Apply modulation based on destination
                switch (destination) {
                    case braidy::ModulationMatrix::TIMBRE:
                        if (modValue != 0.0f) {
                            float currentTimbre = braidy_settings_->GetParameter(braidy::BraidyParameter::TIMBRE);
                            float modulatedTimbre = std::max(0.0f, std::min(32767.0f, currentTimbre + modValue * 16384.0f));
                            braidy_settings_->SetParameter(braidy::BraidyParameter::TIMBRE, modulatedTimbre);
                        }
                        break;
                    case braidy::ModulationMatrix::COLOR:
                        if (modValue != 0.0f) {
                            float currentColor = braidy_settings_->GetParameter(braidy::BraidyParameter::COLOR);
                            float modulatedColor = std::max(0.0f, std::min(32767.0f, currentColor + modValue * 16384.0f));
                            braidy_settings_->SetParameter(braidy::BraidyParameter::COLOR, modulatedColor);
                        }
                        break;
                    case braidy::ModulationMatrix::ALGORITHM_SELECTION:
                        if (modValue != 0.0f && braidy_settings_->GetParameter(braidy::BraidyParameter::META_ENABLED) > 0.5f) {
                            // META mode: LFO controls algorithm position
                            float metaPosition = (modValue + 1.0f) * 0.5f; // Convert -1..1 to 0..1
                            braidy_settings_->SetParameter(braidy::BraidyParameter::META_SPEED, metaPosition);
                        }
                        break;
                    // Add more destinations as needed
                    default:
                        break;
                }
            }
        }
    }
    
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
    // Create a combined state that includes both APVTS and waveform states
    juce::MemoryOutputStream stream(destData, false);
    
    // Save APVTS state
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    
    // Add waveform state data as a child element
    juce::XmlElement* waveformStates = xml->createNewChildElement("WaveformStates");
    juce::MemoryBlock waveformData;
    waveform_state_manager_->SaveToMemoryBlock(waveformData);
    waveformStates->setAttribute("data", waveformData.toBase64Encoding());
    
    // Add modulation matrix state
    juce::XmlElement* modulationState = xml->createNewChildElement("ModulationMatrix");
    
    // Save LFO states
    for (int i = 0; i < 2; ++i) {
        juce::XmlElement* lfoElement = modulationState->createNewChildElement("LFO" + juce::String(i));
        const auto& lfo = modulation_matrix_->getLFO(i);
        lfoElement->setAttribute("enabled", lfo.isEnabled());
        lfoElement->setAttribute("shape", static_cast<int>(lfo.getShape()));
        lfoElement->setAttribute("rate", lfo.getRate());
        lfoElement->setAttribute("depth", lfo.getDepth());
        lfoElement->setAttribute("phaseOffset", lfo.getPhaseOffset());
        lfoElement->setAttribute("tempoSync", lfo.isTempoSynced());
    }
    
    // Save routing information
    for (int d = 0; d < braidy::ModulationMatrix::NUM_DESTINATIONS; ++d) {
        auto dest = static_cast<braidy::ModulationMatrix::Destination>(d);
        const auto& routing = modulation_matrix_->getRouting(dest);
        if (routing.enabled) {
            juce::XmlElement* routeElement = modulationState->createNewChildElement("Route");
            routeElement->setAttribute("destination", d);
            routeElement->setAttribute("sourceId", routing.sourceId);
            routeElement->setAttribute("amount", routing.amount);
            routeElement->setAttribute("bipolar", routing.bipolar);
        }
    }
    
    // Save global modulation settings
    modulationState->setAttribute("metaModeEnabled", braidy_settings_->GetParameter(braidy::BraidyParameter::META_ENABLED));
    modulationState->setAttribute("quantizerEnabled", braidy_settings_->GetParameter(braidy::BraidyParameter::QUANTIZER_SCALE) > 0);
    modulationState->setAttribute("bitCrusherBits", braidy_settings_->GetParameter(braidy::BraidyParameter::BIT_CRUSHER_BITS));
    
    copyXmlToBinary(*xml, destData);
}

void BraidyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // Restore APVTS state
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts_.state.getType())) {
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
            
            // Restore waveform states if present
            juce::XmlElement* waveformStates = xmlState->getChildByName("WaveformStates");
            if (waveformStates != nullptr) {
                juce::String base64Data = waveformStates->getStringAttribute("data");
                if (base64Data.isNotEmpty()) {
                    juce::MemoryBlock waveformData;
                    waveformData.fromBase64Encoding(base64Data);
                    waveform_state_manager_->LoadFromMemoryBlock(waveformData.getData(), waveformData.getSize());
                }
            }
            
            // Restore modulation matrix state if present
            juce::XmlElement* modulationState = xmlState->getChildByName("ModulationMatrix");
            if (modulationState != nullptr) {
                // Restore LFO states
                for (int i = 0; i < 2; ++i) {
                    juce::XmlElement* lfoElement = modulationState->getChildByName("LFO" + juce::String(i));
                    if (lfoElement != nullptr) {
                        auto& lfo = modulation_matrix_->getLFO(i);
                        lfo.setEnabled(lfoElement->getBoolAttribute("enabled", false));
                        lfo.setShape(static_cast<braidy::LFO::Shape>(lfoElement->getIntAttribute("shape", 0)));
                        lfo.setRate(static_cast<float>(lfoElement->getDoubleAttribute("rate", 1.0)));
                        lfo.setDepth(static_cast<float>(lfoElement->getDoubleAttribute("depth", 0.5)));
                        lfo.setPhaseOffset(static_cast<float>(lfoElement->getDoubleAttribute("phaseOffset", 0.0)));
                        lfo.setTempoSync(lfoElement->getBoolAttribute("tempoSync", false));
                    }
                }
                
                // Clear existing routings first
                for (int d = 0; d < braidy::ModulationMatrix::NUM_DESTINATIONS; ++d) {
                    auto dest = static_cast<braidy::ModulationMatrix::Destination>(d);
                    modulation_matrix_->clearRouting(dest);
                }
                
                // Restore routing information
                for (auto* routeElement : modulationState->getChildIterator()) {
                    if (routeElement->hasTagName("Route")) {
                        int destIndex = routeElement->getIntAttribute("destination", 0);
                        auto dest = static_cast<braidy::ModulationMatrix::Destination>(destIndex);
                        int sourceId = routeElement->getIntAttribute("sourceId", 0);
                        float amount = static_cast<float>(routeElement->getDoubleAttribute("amount", 0.0));
                        bool bipolar = routeElement->getBoolAttribute("bipolar", true);
                        modulation_matrix_->setRouting(sourceId, dest, amount, bipolar);
                    }
                }
                
                // Restore global modulation settings
                bool metaEnabled = modulationState->getBoolAttribute("metaModeEnabled", false);
                bool quantizerEnabled = modulationState->getBoolAttribute("quantizerEnabled", false);
                float bitCrusherBits = static_cast<float>(modulationState->getDoubleAttribute("bitCrusherBits", 16.0));
                
                braidy_settings_->SetParameter(braidy::BraidyParameter::META_ENABLED, metaEnabled ? 1.0f : 0.0f);
                braidy_settings_->SetParameter(braidy::BraidyParameter::QUANTIZER_SCALE, quantizerEnabled ? 1.0f : 0.0f);
                braidy_settings_->SetParameter(braidy::BraidyParameter::BIT_CRUSHER_BITS, bitCrusherBits);
                
                // Update voices with new settings
                voice_manager_->UpdateFromSettings(*braidy_settings_);
            }
        }
    }
    
    // If no saved state, reset to defaults
    if (xmlState.get() == nullptr) {
        waveform_state_manager_->ResetAllToDefaults();
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

void BraidyAudioProcessor::setMetaModeEnabled(bool enabled) {
    // Store the setting for when voices are updated
    // The actual enabling happens through UpdateFromSettings
    braidy_settings_->SetParameter(braidy::BraidyParameter::META_ENABLED, enabled ? 1.0f : 0.0f);
    voice_manager_->UpdateFromSettings(*braidy_settings_);
}

void BraidyAudioProcessor::setQuantizerEnabled(bool enabled) {
    // Store the setting for when voices are updated
    braidy_settings_->SetParameter(braidy::BraidyParameter::QUANTIZER_SCALE, enabled ? 1.0f : 0.0f);
    voice_manager_->UpdateFromSettings(*braidy_settings_);
}

void BraidyAudioProcessor::setBitCrusherEnabled(bool enabled) {
    // Store the setting for when voices are updated
    braidy_settings_->SetParameter(braidy::BraidyParameter::BIT_CRUSHER_BITS, enabled ? 8.0f : 16.0f);
    voice_manager_->UpdateFromSettings(*braidy_settings_);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BraidyAudioProcessor();
}