#pragma once

#include "BraidySettings.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <string>
#include <memory>

namespace braidy {

/**
 * Preset data structure for Braidy synthesizer
 */
struct BraidyPreset {
    std::string name;
    std::string author;
    std::string description;
    std::string category;
    std::vector<float> parameters;
    
    BraidyPreset();
    BraidyPreset(const std::string& preset_name);
    
    void InitFromSettings(const BraidySettings& settings);
    void ApplyToSettings(BraidySettings& settings) const;
    
    // Serialization
    juce::XmlElement* ToXml() const;
    static std::unique_ptr<BraidyPreset> FromXml(const juce::XmlElement& xml);
    
    // Validation
    bool IsValid() const;
};

/**
 * Manages presets for the Braidy synthesizer
 */
class PresetManager {
public:
    PresetManager();
    ~PresetManager() = default;
    
    // Preset management
    void AddPreset(const BraidyPreset& preset);
    void AddPreset(const std::string& name, const BraidySettings& settings);
    bool RemovePreset(const std::string& name);
    bool RemovePreset(size_t index);
    
    // Preset access
    const BraidyPreset* GetPreset(const std::string& name) const;
    const BraidyPreset* GetPreset(size_t index) const;
    size_t GetPresetCount() const { return presets_.size(); }
    std::vector<std::string> GetPresetNames() const;
    std::vector<std::string> GetCategories() const;
    std::vector<const BraidyPreset*> GetPresetsInCategory(const std::string& category) const;
    
    // Current preset tracking
    void SetCurrentPreset(size_t index);
    void SetCurrentPreset(const std::string& name);
    int GetCurrentPresetIndex() const { return current_preset_index_; }
    const std::string& GetCurrentPresetName() const;
    
    // Navigation
    void NextPreset();
    void PreviousPreset();
    
    // File operations
    bool LoadPresetFile(const juce::File& file);
    bool SavePresetFile(const juce::File& file, const BraidyPreset& preset) const;
    bool LoadPresetBank(const juce::File& file);
    bool SavePresetBank(const juce::File& file) const;
    
    // Factory presets
    void LoadFactoryPresets();
    void ResetToFactoryPresets();
    
    // Search and filtering
    std::vector<const BraidyPreset*> SearchPresets(const std::string& query) const;
    void SortPresetsByName();
    void SortPresetsByCategory();
    
private:
    std::vector<std::unique_ptr<BraidyPreset>> presets_;
    int current_preset_index_;
    
    // Factory preset creation
    void CreateFactoryPreset(const std::string& name, const std::string& category, 
                           const std::string& description,
                           std::function<void(BraidySettings&)> setup_func);
    
    // Utility functions
    size_t FindPresetIndex(const std::string& name) const;
    bool IsValidIndex(size_t index) const;
    
    DISALLOW_COPY_AND_ASSIGN(PresetManager);
};

/**
 * Preset browser component interface
 */
class PresetBrowser {
public:
    enum class SortMode {
        NAME,
        CATEGORY,
        AUTHOR,
        RECENT
    };
    
    PresetBrowser(PresetManager& preset_manager);
    virtual ~PresetBrowser() = default;
    
    // Browser state
    void SetSortMode(SortMode mode);
    SortMode GetSortMode() const { return sort_mode_; }
    void SetFilterCategory(const std::string& category);
    const std::string& GetFilterCategory() const { return filter_category_; }
    
    // Search functionality
    void SetSearchQuery(const std::string& query);
    const std::string& GetSearchQuery() const { return search_query_; }
    std::vector<const BraidyPreset*> GetFilteredPresets() const;
    
    // Selection
    virtual void SelectPreset(size_t index) = 0;
    virtual void PresetSelected(const BraidyPreset& preset) = 0;
    
protected:
    PresetManager& preset_manager_;
    SortMode sort_mode_;
    std::string filter_category_;
    std::string search_query_;
    
private:
    DISALLOW_COPY_AND_ASSIGN(PresetBrowser);
};

}  // namespace braidy