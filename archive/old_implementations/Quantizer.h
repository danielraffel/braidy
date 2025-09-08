#pragma once

#include "BraidyTypes.h"
#include <array>
#include <cmath>

namespace braidy {

/**
 * Quantizer - Pitch quantization with 12 musical scales
 * Based on Mutable Instruments Braids quantizer
 */
class Quantizer
{
public:
    enum Scale
    {
        CHROMATIC = 0,     // All 12 semitones
        MAJOR,             // Major scale (Ionian)
        MINOR,             // Natural minor (Aeolian)
        HARMONIC_MINOR,    // Harmonic minor
        MELODIC_MINOR,     // Melodic minor
        DORIAN,            // Dorian mode
        PHRYGIAN,          // Phrygian mode
        LYDIAN,            // Lydian mode
        MIXOLYDIAN,        // Mixolydian mode
        LOCRIAN,           // Locrian mode
        PENTATONIC_MAJOR,  // Major pentatonic
        PENTATONIC_MINOR,  // Minor pentatonic
        NUM_SCALES
    };
    
    Quantizer() : currentScale_(CHROMATIC), rootNote_(0), enabled_(false) {}
    
    // Enable/disable quantization
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set the current scale
    void setScale(Scale scale)
    {
        if (scale >= 0 && scale < NUM_SCALES)
        {
            currentScale_ = scale;
        }
    }
    Scale getScale() const { return currentScale_; }
    
    // Set root note (0-11, where 0 = C)
    void setRootNote(int root)
    {
        rootNote_ = root % 12;
        if (rootNote_ < 0) rootNote_ += 12;
    }
    int getRootNote() const { return rootNote_; }
    
    // Quantize a pitch value (in MIDI note units * 128)
    int16_t quantize(int16_t pitch) const
    {
        if (!enabled_) return pitch;
        
        // Convert to MIDI note number
        int midiNote = pitch >> 7;  // Divide by 128
        int fractional = pitch & 0x7F;  // Get fractional part
        
        // Get octave and note within octave
        int octave = midiNote / 12;
        int noteInOctave = midiNote % 12;
        
        // Transpose by root note
        noteInOctave = (noteInOctave - rootNote_ + 12) % 12;
        
        // Find nearest note in scale
        const auto& scaleNotes = scales_[currentScale_];
        int nearestNote = 0;
        int minDistance = 12;
        
        for (int note : scaleNotes)
        {
            if (note == -1) break;  // End of scale
            
            int distance = std::abs(noteInOctave - note);
            if (distance < minDistance)
            {
                minDistance = distance;
                nearestNote = note;
            }
        }
        
        // Transpose back and reconstruct MIDI note
        nearestNote = (nearestNote + rootNote_) % 12;
        int quantizedMidiNote = octave * 12 + nearestNote;
        
        // Convert back to pitch units with fractional part preserved for smooth glides
        return (quantizedMidiNote << 7) + fractional;
    }
    
    // Get scale name
    static const char* getScaleName(Scale scale)
    {
        switch (scale)
        {
            case CHROMATIC: return "CHRM";
            case MAJOR: return "MAJ ";
            case MINOR: return "MIN ";
            case HARMONIC_MINOR: return "HARM";
            case MELODIC_MINOR: return "MELO";
            case DORIAN: return "DORI";
            case PHRYGIAN: return "PHRY";
            case LYDIAN: return "LYDI";
            case MIXOLYDIAN: return "MIXO";
            case LOCRIAN: return "LOCR";
            case PENTATONIC_MAJOR: return "PENT";
            case PENTATONIC_MINOR: return "PENM";
            default: return "????";
        }
    }
    
    // Get root note name
    static const char* getRootNoteName(int root)
    {
        static const char* noteNames[] = {
            "C ", "C#", "D ", "D#", "E ", "F ",
            "F#", "G ", "G#", "A ", "A#", "B "
        };
        return noteNames[root % 12];
    }
    
private:
    Scale currentScale_;
    int rootNote_;
    bool enabled_;
    
    // Scale definitions (intervals from root, -1 = end of scale)
    static constexpr std::array<std::array<int, 13>, NUM_SCALES> scales_ = {{
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1},  // Chromatic
        {0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1, -1},  // Major
        {0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1, -1},  // Minor
        {0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1, -1},  // Harmonic Minor
        {0, 2, 3, 5, 7, 9, 11, -1, -1, -1, -1, -1, -1},  // Melodic Minor
        {0, 2, 3, 5, 7, 9, 10, -1, -1, -1, -1, -1, -1},  // Dorian
        {0, 1, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1, -1},  // Phrygian
        {0, 2, 4, 6, 7, 9, 11, -1, -1, -1, -1, -1, -1},  // Lydian
        {0, 2, 4, 5, 7, 9, 10, -1, -1, -1, -1, -1, -1},  // Mixolydian
        {0, 1, 3, 5, 6, 8, 10, -1, -1, -1, -1, -1, -1},  // Locrian
        {0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1},  // Pentatonic Major
        {0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1, -1}  // Pentatonic Minor
    }};
};

// Definition of the static member
constexpr std::array<std::array<int, 13>, Quantizer::NUM_SCALES> Quantizer::scales_;

} // namespace braidy