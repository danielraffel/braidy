#include "ModeRegistry.h"
#include "../../Source/BraidyCore/MacroOscillator.h" // For braids::MacroOscillatorShape
#include <algorithm>
#include <cctype>

std::unordered_map<std::string, int> ModeRegistry::nameToShape_;
std::unordered_map<int, std::string> ModeRegistry::shapeToName_;
bool ModeRegistry::initialized_ = false;

void ModeRegistry::initialize() {
    if (initialized_) return;
    
    // Map all 47 Braids algorithms to their shapes
    // Based on braids/macro_oscillator.h and the UI strings
    
    // Additive synthesis
    nameToShape_["CSAW"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_CSAW);
    nameToShape_["MORPH"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_MORPH);
    nameToShape_["SAW_SQUARE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SAW_SQUARE);
    nameToShape_["SINE_TRIANGLE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SINE_TRIANGLE);
    nameToShape_["BUZZ"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_BUZZ);
    nameToShape_["SQUARE_SUB"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SQUARE_SUB);
    nameToShape_["SAW_SUB"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SAW_SUB);
    nameToShape_["SQUARE_SYNC"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SQUARE_SYNC);
    nameToShape_["SAW_SYNC"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SAW_SYNC);
    nameToShape_["TRIPLE_SAW"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TRIPLE_SAW);
    nameToShape_["TRIPLE_SQUARE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TRIPLE_SQUARE);
    nameToShape_["TRIPLE_TRIANGLE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TRIPLE_TRIANGLE);
    nameToShape_["TRIPLE_SINE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TRIPLE_SINE);
    nameToShape_["TRIPLE_RING_MOD"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TRIPLE_RING_MOD);
    nameToShape_["SAW_SWARM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SAW_SWARM);
    nameToShape_["SAW_COMB"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SAW_COMB);
    nameToShape_["TOY"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TOY);
    
    // Digital modulation
    nameToShape_["DIGITAL_FILTER_LP"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_DIGITAL_FILTER_LP);
    nameToShape_["DIGITAL_FILTER_PK"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_DIGITAL_FILTER_PK);
    nameToShape_["DIGITAL_FILTER_BP"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_DIGITAL_FILTER_BP);
    nameToShape_["DIGITAL_FILTER_HP"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_DIGITAL_FILTER_HP);
    nameToShape_["VOSIM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_VOSIM);
    nameToShape_["VOWEL"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_VOWEL);
    nameToShape_["VOWEL_FOF"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_VOWEL_FOF);
    
    // FM synthesis
    nameToShape_["HARMONICS"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_HARMONICS);
    nameToShape_["FM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_FM);
    nameToShape_["FEEDBACK_FM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_FEEDBACK_FM);
    nameToShape_["CHAOTIC_FEEDBACK_FM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_CHAOTIC_FEEDBACK_FM);
    nameToShape_["PLUCKED"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_PLUCKED);
    nameToShape_["BOWED"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_BOWED);
    nameToShape_["BLOWN"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_BLOWN);
    nameToShape_["FLUTED"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_FLUTED);
    nameToShape_["STRUCK_BELL"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_STRUCK_BELL);
    nameToShape_["STRUCK_DRUM"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_STRUCK_DRUM);
    nameToShape_["KICK"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_KICK);
    nameToShape_["CYMBAL"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_CYMBAL);
    nameToShape_["SNARE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_SNARE);
    
    // Wavetable
    nameToShape_["WAVETABLES"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_WAVETABLES);
    nameToShape_["WAVE_MAP"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_WAVE_MAP);
    nameToShape_["WAVE_LINE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_WAVE_LINE);
    nameToShape_["WAVE_PARAPHONIC"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_WAVE_PARAPHONIC);
    
    // Speech
    nameToShape_["FILTERED_NOISE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_FILTERED_NOISE);
    nameToShape_["TWIN_PEAKS_NOISE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_TWIN_PEAKS_NOISE);
    nameToShape_["CLOCKED_NOISE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_CLOCKED_NOISE);
    nameToShape_["GRANULAR_CLOUD"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_GRANULAR_CLOUD);
    nameToShape_["PARTICLE_NOISE"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_PARTICLE_NOISE);
    
    // Additional modes
    nameToShape_["DIGITAL_MODULATION"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_DIGITAL_MODULATION);
    nameToShape_["QUESTION_MARK"] = static_cast<int>(braids::MACRO_OSCILLATOR_SHAPE_QUESTION_MARK);
    
    // Build reverse mapping
    for (const auto& pair : nameToShape_) {
        shapeToName_[pair.second] = pair.first;
    }
    
    initialized_ = true;
}

std::string ModeRegistry::normalizeName(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    return normalized;
}

int ModeRegistry::getShapeForName(const std::string& name) {
    initialize();
    
    std::string normalized = normalizeName(name);
    auto it = nameToShape_.find(normalized);
    return (it != nameToShape_.end()) ? it->second : -1;
}

std::string ModeRegistry::getNameForShape(int shape) {
    initialize();
    
    auto it = shapeToName_.find(shape);
    return (it != shapeToName_.end()) ? it->second : "";
}

std::vector<std::string> ModeRegistry::getAllNames() {
    initialize();
    
    std::vector<std::string> names;
    names.reserve(nameToShape_.size());
    
    for (const auto& pair : nameToShape_) {
        names.push_back(pair.first);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<int> ModeRegistry::getAllShapes() {
    initialize();
    
    std::vector<int> shapes;
    shapes.reserve(shapeToName_.size());
    
    for (const auto& pair : shapeToName_) {
        shapes.push_back(pair.first);
    }
    
    std::sort(shapes.begin(), shapes.end());
    return shapes;
}

bool ModeRegistry::hasName(const std::string& name) {
    initialize();
    
    std::string normalized = normalizeName(name);
    return nameToShape_.find(normalized) != nameToShape_.end();
}

bool ModeRegistry::hasShape(int shape) {
    initialize();
    
    return shapeToName_.find(shape) != shapeToName_.end();
}