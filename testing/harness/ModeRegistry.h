#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Forward declare the braids enum
namespace braids {
    enum MacroOscillatorShape;
}

/**
 * Registry that maps algorithm names to Braids MacroOscillatorShape values.
 * Provides bidirectional mapping between human-readable names and enum values.
 */
class ModeRegistry {
public:
    /**
     * Initialize the registry with all 47 Braids algorithms.
     */
    static void initialize();
    
    /**
     * Get the MacroOscillatorShape for a given algorithm name.
     * @param name Algorithm name (case-insensitive)
     * @return Shape enum value, or -1 if not found
     */
    static int getShapeForName(const std::string& name);
    
    /**
     * Get the algorithm name for a given MacroOscillatorShape.
     * @param shape Shape enum value
     * @return Algorithm name, or empty string if not found
     */
    static std::string getNameForShape(int shape);
    
    /**
     * Get all registered algorithm names.
     * @return Vector of all algorithm names
     */
    static std::vector<std::string> getAllNames();
    
    /**
     * Get all registered shape values.
     * @return Vector of all shape enum values
     */
    static std::vector<int> getAllShapes();
    
    /**
     * Check if an algorithm name is registered.
     * @param name Algorithm name to check
     * @return True if the name exists in the registry
     */
    static bool hasName(const std::string& name);
    
    /**
     * Check if a shape value is registered.
     * @param shape Shape enum value to check
     * @return True if the shape exists in the registry
     */
    static bool hasShape(int shape);
    
private:
    static std::unordered_map<std::string, int> nameToShape_;
    static std::unordered_map<int, std::string> shapeToName_;
    static bool initialized_;
    
    // Helper function to normalize names for lookup
    static std::string normalizeName(const std::string& name);
};