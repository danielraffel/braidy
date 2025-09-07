#pragma once

#include <JuceHeader.h>
#include "LFO.h"
#include <vector>

namespace braidy {

/**
 * ModulationMatrix - Routes LFO sources to parameter destinations
 * Manages multiple LFOs and their routing to various Braidy parameters.
 */
class ModulationMatrix
{
public:
    // Available modulation destinations for Braidy
    enum Destination
    {
        // Core synthesis parameters
        ALGORITHM_SELECTION,  // META mode - morph between algorithms
        TIMBRE,              // Main timbre parameter
        COLOR,               // Main color parameter
        
        // Oscillator parameters
        PITCH,               // Oscillator pitch
        DETUNE,              // Oscillator detune
        OCTAVE,              // Octave shift
        
        // Envelope parameters
        ENV_ATTACK,          // Attack time
        ENV_DECAY,           // Decay time
        ENV_SUSTAIN,         // Sustain level
        ENV_RELEASE,         // Release time
        ENV_FM_AMOUNT,       // Envelope to FM amount
        ENV_TIMBRE_AMOUNT,  // Envelope to timbre amount
        ENV_COLOR_AMOUNT,   // Envelope to color amount
        
        // Filter/effects
        BIT_CRUSHER_BITS,    // Bit depth reduction
        BIT_CRUSHER_RATE,    // Sample rate reduction
        
        // Mixer
        VOLUME,              // Overall volume
        PAN,                 // Stereo pan
        
        // Quantizer
        QUANTIZE_SCALE,      // Scale selection
        QUANTIZE_ROOT,       // Root note
        
        // Modulation
        VIBRATO_AMOUNT,      // Vibrato depth
        VIBRATO_RATE,        // Vibrato speed
        
        NUM_DESTINATIONS
    };
    
    // Get human-readable name for destination
    static const char* getDestinationName(Destination dest)
    {
        switch (dest)
        {
            case ALGORITHM_SELECTION: return "Algorithm (META)";
            case TIMBRE: return "Timbre";
            case COLOR: return "Color";
            case PITCH: return "Pitch";
            case DETUNE: return "Detune";
            case OCTAVE: return "Octave";
            case ENV_ATTACK: return "Env Attack";
            case ENV_DECAY: return "Env Decay";
            case ENV_SUSTAIN: return "Env Sustain";
            case ENV_RELEASE: return "Env Release";
            case ENV_FM_AMOUNT: return "Env→FM Amount";
            case ENV_TIMBRE_AMOUNT: return "Env→Timbre Amount";
            case ENV_COLOR_AMOUNT: return "Env→Color Amount";
            case BIT_CRUSHER_BITS: return "Bit Depth";
            case BIT_CRUSHER_RATE: return "Sample Rate";
            case VOLUME: return "Volume";
            case PAN: return "Pan";
            case QUANTIZE_SCALE: return "Quantize Scale";
            case QUANTIZE_ROOT: return "Quantize Root";
            case VIBRATO_AMOUNT: return "Vibrato Amount";
            case VIBRATO_RATE: return "Vibrato Rate";
            default: return "Unknown";
        }
    }
    
    // Modulation routing configuration
    struct Routing
    {
        int sourceId;           // LFO index (0 or 1)
        Destination dest;       // Target parameter
        float amount;          // Modulation amount (-1 to +1)
        bool bipolar;          // True for bipolar (-1 to +1), false for unipolar (0 to 1)
        bool enabled;          // Is this routing active?
        
        Routing() : sourceId(0), dest(TIMBRE), amount(0.0f), bipolar(true), enabled(false) {}
        
        Routing(int source, Destination d, float amt, bool bi = true)
            : sourceId(source), dest(d), amount(amt), bipolar(bi), enabled(true) {}
    };
    
    ModulationMatrix()
    {
        // Initialize with empty routings for each destination
        routings_.resize(NUM_DESTINATIONS);
        
        // Create 2 LFOs
        lfos_.resize(2);
    }
    
    // Update LFOs (called from audio thread)
    void processBlock(double sampleRate, int numSamples, double bpm = 120.0)
    {
        for (auto& lfo : lfos_)
        {
            lfo.advance(sampleRate, numSamples, bpm);
        }
    }
    
    // Add or update a routing
    void setRouting(int lfoId, Destination dest, float amount, bool bipolar = true)
    {
        if (lfoId < 0 || lfoId >= static_cast<int>(lfos_.size())) return;
        if (dest < 0 || dest >= NUM_DESTINATIONS) return;
        
        routings_[dest] = Routing(lfoId, dest, amount, bipolar);
    }
    
    // Remove a routing
    void clearRouting(Destination dest)
    {
        if (dest < 0 || dest >= NUM_DESTINATIONS) return;
        routings_[dest].enabled = false;
    }
    
    // Get modulation value for a destination
    float getModulation(Destination dest) const
    {
        if (dest < 0 || dest >= NUM_DESTINATIONS) return 0.0f;
        
        const auto& routing = routings_[dest];
        if (!routing.enabled) return 0.0f;
        
        if (routing.sourceId < 0 || routing.sourceId >= static_cast<int>(lfos_.size())) return 0.0f;
        
        const auto& lfo = lfos_[routing.sourceId];
        if (!lfo.isEnabled()) return 0.0f;
        
        float value = routing.bipolar ? lfo.getValue() : lfo.getUnipolarValue();
        return value * routing.amount;
    }
    
    // Apply modulation to a parameter value
    float applyModulation(Destination dest, float baseValue, float minValue = 0.0f, float maxValue = 1.0f) const
    {
        float modulation = getModulation(dest);
        float range = maxValue - minValue;
        float modulated = baseValue + (modulation * range);
        return juce::jlimit(minValue, maxValue, modulated);
    }
    
    // Apply modulation to integer parameter (for scales, algorithms, etc.)
    int applyModulationInt(Destination dest, int baseValue, int minValue, int maxValue) const
    {
        float normalized = static_cast<float>(baseValue - minValue) / static_cast<float>(maxValue - minValue);
        float modulated = applyModulation(dest, normalized, 0.0f, 1.0f);
        return minValue + static_cast<int>(modulated * (maxValue - minValue));
    }
    
    // Get LFO references for configuration
    LFO& getLFO(int index)
    {
        jassert(index >= 0 && index < static_cast<int>(lfos_.size()));
        return lfos_[index];
    }
    
    const LFO& getLFO(int index) const
    {
        jassert(index >= 0 && index < static_cast<int>(lfos_.size()));
        return lfos_[index];
    }
    
    // Get routing for a destination
    const Routing& getRouting(Destination dest) const
    {
        static Routing emptyRouting;
        if (dest < 0 || dest >= NUM_DESTINATIONS) return emptyRouting;
        return routings_[dest];
    }
    
    // Check if a destination is modulated
    bool isModulated(Destination dest) const
    {
        if (dest < 0 || dest >= NUM_DESTINATIONS) return false;
        const auto& routing = routings_[dest];
        if (!routing.enabled) return false;
        if (routing.sourceId < 0 || routing.sourceId >= static_cast<int>(lfos_.size())) return false;
        return lfos_[routing.sourceId].isEnabled();
    }
    
    // Reset all LFOs to phase 0
    void reset()
    {
        for (auto& lfo : lfos_)
        {
            lfo.reset();
        }
    }
    
    // Clear all routings
    void clearAllRoutings()
    {
        for (auto& routing : routings_)
        {
            routing.enabled = false;
        }
    }
    
    // Save/restore state
    void saveToValueTree(juce::ValueTree& tree) const
    {
        // Save LFO states
        for (size_t i = 0; i < lfos_.size(); ++i)
        {
            auto lfoTree = tree.getOrCreateChildWithName("LFO" + juce::String(i + 1), nullptr);
            lfos_[i].saveToValueTree(lfoTree);
        }
        
        // Save routings
        auto routingsTree = tree.getOrCreateChildWithName("Routings", nullptr);
        for (size_t i = 0; i < routings_.size(); ++i)
        {
            if (routings_[i].enabled)
            {
                auto routingTree = juce::ValueTree("Routing");
                routingTree.setProperty("destination", static_cast<int>(i), nullptr);
                routingTree.setProperty("sourceId", routings_[i].sourceId, nullptr);
                routingTree.setProperty("amount", routings_[i].amount, nullptr);
                routingTree.setProperty("bipolar", routings_[i].bipolar, nullptr);
                routingsTree.appendChild(routingTree, nullptr);
            }
        }
    }
    
    void loadFromValueTree(const juce::ValueTree& tree)
    {
        // Load LFO states
        for (size_t i = 0; i < lfos_.size(); ++i)
        {
            auto lfoTree = tree.getChildWithName("LFO" + juce::String(i + 1));
            if (lfoTree.isValid())
            {
                lfos_[i].loadFromValueTree(lfoTree);
            }
        }
        
        // Clear and load routings
        clearAllRoutings();
        auto routingsTree = tree.getChildWithName("Routings");
        if (routingsTree.isValid())
        {
            for (int i = 0; i < routingsTree.getNumChildren(); ++i)
            {
                auto routingTree = routingsTree.getChild(i);
                int dest = routingTree.getProperty("destination", -1);
                if (dest >= 0 && dest < NUM_DESTINATIONS)
                {
                    routings_[dest].sourceId = routingTree.getProperty("sourceId", 0);
                    routings_[dest].dest = static_cast<Destination>(dest);
                    routings_[dest].amount = routingTree.getProperty("amount", 0.0f);
                    routings_[dest].bipolar = routingTree.getProperty("bipolar", true);
                    routings_[dest].enabled = true;
                }
            }
        }
    }
    
private:
    std::vector<LFO> lfos_;
    std::vector<Routing> routings_;
};

} // namespace braidy