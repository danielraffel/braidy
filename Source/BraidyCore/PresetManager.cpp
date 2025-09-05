#include "PresetManager.h"
#include <algorithm>
#include <sstream>
#include <ctime>

namespace braidy {

// BraidyPreset implementation
BraidyPreset::BraidyPreset() 
    : name("Untitled")
    , author("Unknown")
    , description("")
    , category("User")
{
    parameters.resize(static_cast<size_t>(BraidyParameter::PARAMETER_COUNT), 0.0f);
}

BraidyPreset::BraidyPreset(const std::string& preset_name) 
    : name(preset_name)
    , author("Unknown")
    , description("")
    , category("User")
{
    parameters.resize(static_cast<size_t>(BraidyParameter::PARAMETER_COUNT), 0.0f);
}

void BraidyPreset::InitFromSettings(const BraidySettings& settings) {
    parameters.resize(static_cast<size_t>(BraidyParameter::PARAMETER_COUNT));
    
    for (int i = 0; i < static_cast<int>(BraidyParameter::PARAMETER_COUNT); ++i) {
        auto param = static_cast<BraidyParameter>(i);
        parameters[i] = settings.GetParameter(param);
    }
}

void BraidyPreset::ApplyToSettings(BraidySettings& settings) const {
    if (parameters.size() != static_cast<size_t>(BraidyParameter::PARAMETER_COUNT)) {
        return;  // Invalid preset
    }
    
    for (int i = 0; i < static_cast<int>(BraidyParameter::PARAMETER_COUNT); ++i) {
        auto param = static_cast<BraidyParameter>(i);
        settings.SetParameter(param, parameters[i]);
    }
}

juce::XmlElement* BraidyPreset::ToXml() const {
    auto xml = std::make_unique<juce::XmlElement>("BraidyPreset");
    
    xml->setAttribute("name", name);
    xml->setAttribute("author", author);
    xml->setAttribute("category", category);
    xml->setAttribute("description", description);
    xml->setAttribute("version", "1.0");
    
    auto params_xml = xml->createNewChildElement("Parameters");
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto param_xml = params_xml->createNewChildElement("Parameter");
        param_xml->setAttribute("index", static_cast<int>(i));
        param_xml->setAttribute("value", parameters[i]);
    }
    
    return xml.release();
}

std::unique_ptr<BraidyPreset> BraidyPreset::FromXml(const juce::XmlElement& xml) {
    if (!xml.hasTagName("BraidyPreset")) {
        return nullptr;
    }
    
    auto preset = std::make_unique<BraidyPreset>();
    preset->name = xml.getStringAttribute("name", "Untitled").toStdString();
    preset->author = xml.getStringAttribute("author", "Unknown").toStdString();
    preset->category = xml.getStringAttribute("category", "User").toStdString();
    preset->description = xml.getStringAttribute("description", "").toStdString();
    
    auto params_xml = xml.getChildByName("Parameters");
    if (params_xml) {
        preset->parameters.resize(static_cast<size_t>(BraidyParameter::PARAMETER_COUNT), 0.0f);
        
        for (auto* param_xml : params_xml->getChildIterator()) {
            int index = param_xml->getIntAttribute("index");
            float value = static_cast<float>(param_xml->getDoubleAttribute("value"));
            
            if (index >= 0 && index < static_cast<int>(BraidyParameter::PARAMETER_COUNT)) {
                preset->parameters[index] = value;
            }
        }
    }
    
    return preset;
}

bool BraidyPreset::IsValid() const {
    return !name.empty() && 
           parameters.size() == static_cast<size_t>(BraidyParameter::PARAMETER_COUNT);
}

// PresetManager implementation
PresetManager::PresetManager() : current_preset_index_(-1) {
    LoadFactoryPresets();
}

void PresetManager::AddPreset(const BraidyPreset& preset) {
    if (!preset.IsValid()) return;
    
    presets_.push_back(std::make_unique<BraidyPreset>(preset));
}

void PresetManager::AddPreset(const std::string& name, const BraidySettings& settings) {
    BraidyPreset preset(name);
    preset.InitFromSettings(settings);
    AddPreset(preset);
}

bool PresetManager::RemovePreset(const std::string& name) {
    size_t index = FindPresetIndex(name);
    if (index < presets_.size()) {
        return RemovePreset(index);
    }
    return false;
}

bool PresetManager::RemovePreset(size_t index) {
    if (!IsValidIndex(index)) return false;
    
    presets_.erase(presets_.begin() + index);
    
    // Update current preset index
    if (current_preset_index_ == static_cast<int>(index)) {
        current_preset_index_ = -1;
    } else if (current_preset_index_ > static_cast<int>(index)) {
        --current_preset_index_;
    }
    
    return true;
}

const BraidyPreset* PresetManager::GetPreset(const std::string& name) const {
    size_t index = FindPresetIndex(name);
    return GetPreset(index);
}

const BraidyPreset* PresetManager::GetPreset(size_t index) const {
    if (IsValidIndex(index)) {
        return presets_[index].get();
    }
    return nullptr;
}

std::vector<std::string> PresetManager::GetPresetNames() const {
    std::vector<std::string> names;
    names.reserve(presets_.size());
    
    for (const auto& preset : presets_) {
        names.push_back(preset->name);
    }
    
    return names;
}

std::vector<std::string> PresetManager::GetCategories() const {
    std::vector<std::string> categories;
    
    for (const auto& preset : presets_) {
        if (std::find(categories.begin(), categories.end(), preset->category) == categories.end()) {
            categories.push_back(preset->category);
        }
    }
    
    std::sort(categories.begin(), categories.end());
    return categories;
}

std::vector<const BraidyPreset*> PresetManager::GetPresetsInCategory(const std::string& category) const {
    std::vector<const BraidyPreset*> presets;
    
    for (const auto& preset : presets_) {
        if (preset->category == category) {
            presets.push_back(preset.get());
        }
    }
    
    return presets;
}

void PresetManager::SetCurrentPreset(size_t index) {
    if (IsValidIndex(index)) {
        current_preset_index_ = static_cast<int>(index);
    }
}

void PresetManager::SetCurrentPreset(const std::string& name) {
    size_t index = FindPresetIndex(name);
    SetCurrentPreset(index);
}

const std::string& PresetManager::GetCurrentPresetName() const {
    static const std::string empty_name = "";
    
    if (current_preset_index_ >= 0 && current_preset_index_ < static_cast<int>(presets_.size())) {
        return presets_[current_preset_index_]->name;
    }
    
    return empty_name;
}

void PresetManager::NextPreset() {
    if (!presets_.empty()) {
        current_preset_index_ = (current_preset_index_ + 1) % static_cast<int>(presets_.size());
    }
}

void PresetManager::PreviousPreset() {
    if (!presets_.empty()) {
        current_preset_index_ = (current_preset_index_ - 1 + static_cast<int>(presets_.size())) % static_cast<int>(presets_.size());
    }
}

bool PresetManager::LoadPresetFile(const juce::File& file) {
    if (!file.exists()) return false;
    
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    if (!xml) return false;
    
    auto preset = BraidyPreset::FromXml(*xml);
    if (!preset) return false;
    
    AddPreset(*preset);
    return true;
}

bool PresetManager::SavePresetFile(const juce::File& file, const BraidyPreset& preset) const {
    if (!preset.IsValid()) return false;
    
    std::unique_ptr<juce::XmlElement> xml(preset.ToXml());
    if (!xml) return false;
    
    return xml->writeTo(file);
}

bool PresetManager::LoadPresetBank(const juce::File& file) {
    if (!file.exists()) return false;
    
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    if (!xml || !xml->hasTagName("BraidyPresetBank")) return false;
    
    int loaded_count = 0;
    for (auto* preset_xml : xml->getChildIterator()) {
        auto preset = BraidyPreset::FromXml(*preset_xml);
        if (preset) {
            AddPreset(*preset);
            ++loaded_count;
        }
    }
    
    return loaded_count > 0;
}

bool PresetManager::SavePresetBank(const juce::File& file) const {
    auto bank_xml = std::make_unique<juce::XmlElement>("BraidyPresetBank");
    bank_xml->setAttribute("version", "1.0");
    bank_xml->setAttribute("count", static_cast<int>(presets_.size()));
    
    for (const auto& preset : presets_) {
        std::unique_ptr<juce::XmlElement> preset_xml(preset->ToXml());
        if (preset_xml) {
            bank_xml->addChildElement(preset_xml.release());
        }
    }
    
    return bank_xml->writeTo(file);
}

void PresetManager::LoadFactoryPresets() {
    // Clear existing presets
    presets_.clear();
    current_preset_index_ = -1;
    
    // Create basic factory presets showing off different synthesis modes
    CreateFactoryPreset("Classic Saw", "Basic", "Classic sawtooth wave", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, 0.0f);  // CSAW
        s.SetParameter(BraidyParameter::TIMBRE, 0.5f);
        s.SetParameter(BraidyParameter::COLOR, 0.3f);
        s.SetParameter(BraidyParameter::ATTACK, 0.01f);
        s.SetParameter(BraidyParameter::DECAY, 0.3f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.7f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.5f);
        s.SetParameter(BraidyParameter::VOLUME, 0.8f);
    });
    
    CreateFactoryPreset("FM Bell", "Physical", "FM synthesis bell sound", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::FM)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.7f);
        s.SetParameter(BraidyParameter::COLOR, 0.4f);
        s.SetParameter(BraidyParameter::FM_AMOUNT, 0.6f);
        s.SetParameter(BraidyParameter::FM_RATIO, 3.5f);
        s.SetParameter(BraidyParameter::ATTACK, 0.05f);
        s.SetParameter(BraidyParameter::DECAY, 0.8f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.2f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 1.0f);
        s.SetParameter(BraidyParameter::VOLUME, 0.7f);
    });
    
    CreateFactoryPreset("Crushed Lead", "Electronic", "Bit-crushed lead sound", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::SAW_SQUARE)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.8f);
        s.SetParameter(BraidyParameter::COLOR, 0.6f);
        s.SetParameter(BraidyParameter::ATTACK, 0.1f);
        s.SetParameter(BraidyParameter::DECAY, 0.2f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.9f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.3f);
        s.SetParameter(BraidyParameter::BIT_CRUSHER_BITS, 8.0f);
        s.SetParameter(BraidyParameter::BIT_CRUSHER_RATE, 4.0f);
        s.SetParameter(BraidyParameter::WAVESHAPER_AMOUNT, 0.4f);
        s.SetParameter(BraidyParameter::VOLUME, 0.6f);
    });
    
    CreateFactoryPreset("Plucked String", "Physical", "Physical modeling plucked string", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::PLUCKED)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.6f);
        s.SetParameter(BraidyParameter::COLOR, 0.5f);
        s.SetParameter(BraidyParameter::ATTACK, 0.0f);
        s.SetParameter(BraidyParameter::DECAY, 0.7f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.3f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.8f);
        s.SetParameter(BraidyParameter::VOLUME, 0.8f);
    });
    
    CreateFactoryPreset("Vowel Pad", "Spectral", "Vowel synthesis pad", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::VOWEL)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.4f);
        s.SetParameter(BraidyParameter::COLOR, 0.7f);
        s.SetParameter(BraidyParameter::ATTACK, 0.3f);
        s.SetParameter(BraidyParameter::DECAY, 0.4f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.8f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.7f);
        s.SetParameter(BraidyParameter::ENVELOPE_SHAPE, 2.0f);  // Logarithmic
        s.SetParameter(BraidyParameter::VOLUME, 0.6f);
    });
    
    CreateFactoryPreset("Granular Cloud", "Texture", "Granular synthesis texture", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::GRANULAR_CLOUD)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.3f);
        s.SetParameter(BraidyParameter::COLOR, 0.8f);
        s.SetParameter(BraidyParameter::ATTACK, 0.2f);
        s.SetParameter(BraidyParameter::DECAY, 0.6f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.5f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.9f);
        s.SetParameter(BraidyParameter::WAVESHAPER_AMOUNT, 0.2f);
        s.SetParameter(BraidyParameter::WAVESHAPER_TYPE, 3.0f);  // Tube
        s.SetParameter(BraidyParameter::VOLUME, 0.5f);
    });
    
    CreateFactoryPreset("Digital Filter", "Filter", "Digital filter sweep", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::DIGITAL_FILTER_LP)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.2f);
        s.SetParameter(BraidyParameter::COLOR, 0.9f);
        s.SetParameter(BraidyParameter::ATTACK, 0.05f);
        s.SetParameter(BraidyParameter::DECAY, 0.5f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.6f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.4f);
        s.SetParameter(BraidyParameter::VOLUME, 0.7f);
    });
    
    CreateFactoryPreset("Triple Saw", "Harmonic", "Triple sawtooth oscillator", [](BraidySettings& s) {
        s.SetParameter(BraidyParameter::SHAPE, static_cast<float>(static_cast<int>(MacroOscillatorShape::TRIPLE_SAW)));
        s.SetParameter(BraidyParameter::TIMBRE, 0.6f);
        s.SetParameter(BraidyParameter::COLOR, 0.4f);
        s.SetParameter(BraidyParameter::ATTACK, 0.1f);
        s.SetParameter(BraidyParameter::DECAY, 0.3f);
        s.SetParameter(BraidyParameter::ENVELOPE_SUSTAIN, 0.8f);
        s.SetParameter(BraidyParameter::ENVELOPE_RELEASE, 0.5f);
        s.SetParameter(BraidyParameter::PARAPHONY_ENABLED, 1.0f);
        s.SetParameter(BraidyParameter::PARAPHONY_DETUNE, 0.3f);
        s.SetParameter(BraidyParameter::VOLUME, 0.7f);
    });
    
    // Set first preset as current
    if (!presets_.empty()) {
        current_preset_index_ = 0;
    }
}

void PresetManager::ResetToFactoryPresets() {
    LoadFactoryPresets();
}

std::vector<const BraidyPreset*> PresetManager::SearchPresets(const std::string& query) const {
    std::vector<const BraidyPreset*> results;
    
    if (query.empty()) {
        for (const auto& preset : presets_) {
            results.push_back(preset.get());
        }
        return results;
    }
    
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (const auto& preset : presets_) {
        std::string lower_name = preset->name;
        std::string lower_category = preset->category;
        std::string lower_description = preset->description;
        
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        std::transform(lower_category.begin(), lower_category.end(), lower_category.begin(), ::tolower);
        std::transform(lower_description.begin(), lower_description.end(), lower_description.begin(), ::tolower);
        
        if (lower_name.find(lower_query) != std::string::npos ||
            lower_category.find(lower_query) != std::string::npos ||
            lower_description.find(lower_query) != std::string::npos) {
            results.push_back(preset.get());
        }
    }
    
    return results;
}

void PresetManager::SortPresetsByName() {
    std::sort(presets_.begin(), presets_.end(), 
        [](const std::unique_ptr<BraidyPreset>& a, const std::unique_ptr<BraidyPreset>& b) {
            return a->name < b->name;
        });
    
    current_preset_index_ = -1;  // Reset current selection
}

void PresetManager::SortPresetsByCategory() {
    std::sort(presets_.begin(), presets_.end(), 
        [](const std::unique_ptr<BraidyPreset>& a, const std::unique_ptr<BraidyPreset>& b) {
            if (a->category != b->category) {
                return a->category < b->category;
            }
            return a->name < b->name;
        });
    
    current_preset_index_ = -1;  // Reset current selection
}

void PresetManager::CreateFactoryPreset(const std::string& name, const std::string& category, 
                                      const std::string& description,
                                      std::function<void(BraidySettings&)> setup_func) {
    BraidyPreset preset(name);
    preset.author = "Braidy Factory";
    preset.category = category;
    preset.description = description;
    
    // Create temporary settings to configure the preset
    BraidySettings temp_settings;
    temp_settings.Init();
    setup_func(temp_settings);
    preset.InitFromSettings(temp_settings);
    
    AddPreset(preset);
}

size_t PresetManager::FindPresetIndex(const std::string& name) const {
    for (size_t i = 0; i < presets_.size(); ++i) {
        if (presets_[i]->name == name) {
            return i;
        }
    }
    return presets_.size();  // Invalid index
}

bool PresetManager::IsValidIndex(size_t index) const {
    return index < presets_.size();
}

// PresetBrowser implementation
PresetBrowser::PresetBrowser(PresetManager& preset_manager) 
    : preset_manager_(preset_manager)
    , sort_mode_(SortMode::CATEGORY)
    , filter_category_("")
    , search_query_("")
{
}

void PresetBrowser::SetSortMode(SortMode mode) {
    if (sort_mode_ != mode) {
        sort_mode_ = mode;
        
        switch (mode) {
            case SortMode::NAME:
                preset_manager_.SortPresetsByName();
                break;
            case SortMode::CATEGORY:
                preset_manager_.SortPresetsByCategory();
                break;
            case SortMode::AUTHOR:
                // Could implement author sorting
                break;
            case SortMode::RECENT:
                // Could implement recent sorting
                break;
        }
    }
}

void PresetBrowser::SetFilterCategory(const std::string& category) {
    filter_category_ = category;
}

void PresetBrowser::SetSearchQuery(const std::string& query) {
    search_query_ = query;
}

std::vector<const BraidyPreset*> PresetBrowser::GetFilteredPresets() const {
    std::vector<const BraidyPreset*> filtered;
    
    // Start with search results
    auto search_results = preset_manager_.SearchPresets(search_query_);
    
    // Apply category filter
    if (!filter_category_.empty()) {
        for (auto* preset : search_results) {
            if (preset->category == filter_category_) {
                filtered.push_back(preset);
            }
        }
    } else {
        filtered = search_results;
    }
    
    return filtered;
}

}  // namespace braidy