#pragma once

#include <cstdint>
#include <cstddef>

// Core types for Braidy synthesizer - adapted from Mutable Instruments Braids
// Based on stmlib types but adapted for JUCE/modern C++

namespace braidy {

// Basic integer types
using int8_t = std::int8_t;
using uint8_t = std::uint8_t;
using int16_t = std::int16_t;
using uint16_t = std::uint16_t;
using int32_t = std::int32_t;
using uint32_t = std::uint32_t;

// Floating point
using float_t = float;
using double_t = double;

// Size type
using size_t = std::size_t;

// Macro oscillator shapes - all 45+ models from Braids
enum class MacroOscillatorShape {
    // Basic analog-style waveforms
    CSAW = 0,                    // \/\/\_   Classic saw
    MORPH,                       // /\/\|_   Morphing between waves  
    SAW_SQUARE,                  // /|/|\_   Saw/Square morph
    SINE_TRIANGLE,               // ~~~\_    Sine/Triangle morph
    BUZZ,                        // BUZZ     Square wave buzz

    // Sub-oscillator variants  
    SQUARE_SUB,                  // /|/|+    Square with sub
    SAW_SUB,                     // \/\/+    Saw with sub
    SQUARE_SYNC,                 // /|/|*    Hard sync square
    SAW_SYNC,                    // \/\/*    Hard sync saw
    TRIPLE_SAW,                  // \/3      Triple saw
    TRIPLE_SQUARE,               // /|3      Triple square
    TRIPLE_TRIANGLE,             // /\3      Triple triangle
    TRIPLE_SINE,                 // ~~3      Triple sine
    TRIPLE_RING_MOD,             // ><3      Triple ring mod
    SAW_SWARM,                   // \/\/s    Saw swarm
    SAW_COMB,                    // \/\/#    Saw through comb filter
    TOY,                         // TOY      Toy box

    // Digital filter models
    DIGITAL_FILTER_LP,           // FLTR     Digital lowpass
    DIGITAL_FILTER_PK,           // PEAK     Peak filter
    DIGITAL_FILTER_BP,           // BAND     Bandpass filter
    DIGITAL_FILTER_HP,           // HIGH     Highpass filter
    VOSIM,                       // VOSM     VOSIM synthesis
    VOWEL,                       // VOWL     Vowel synthesis
    VOWEL_FOF,                   // VOW2     Vowel FOF synthesis
    
    HARMONICS,                   // HARM     Harmonic oscillator

    // FM synthesis
    FM,                          // FM       2-operator FM
    FEEDBACK_FM,                 // FBFM     Feedback FM
    CHAOTIC_FEEDBACK_FM,         // WTFM     Chaotic feedback FM

    // Physical modeling
    PLUCKED,                     // PLUK     Plucked string
    BOWED,                       // BOWD     Bowed string
    BLOWN,                       // BLOW     Blown pipe
    FLUTED,                      // FLUT     Flute model
    STRUCK_BELL,                 // BELL     Bell model
    STRUCK_DRUM,                 // DRUM     Drum model
    KICK,                        // KICK     Kick drum
    CYMBAL,                      // CYMB     Cymbal
    SNARE,                       // SNAR     Snare drum

    // Wavetable synthesis
    WAVETABLES,                  // WTBL     Wavetable lookup
    WAVE_MAP,                    // WMAP     2D wavetable
    WAVE_LINE,                   // WLIN     1D wavetable sweep
    WAVE_PARAPHONIC,             // WPAR     Paraphonic wavetables

    // Noise and granular
    FILTERED_NOISE,              // NOIS     Filtered noise
    TWIN_PEAKS_NOISE,            // TWLN     Twin peaks noise
    CLOCKED_NOISE,               // CLKN     Clocked noise
    GRANULAR_CLOUD,              // CLDS     Granular cloud
    PARTICLE_NOISE,              // PART     Particle noise
    
    DIGITAL_MODULATION,          // DIGI     Digital modulation

    QUESTION_MARK,               // ????     Mystery algorithm
    
    // Meta-oscillator (cycling through algorithms)
    LAST,
    LAST_ACCESSIBLE_FROM_META = DIGITAL_MODULATION
};

// Audio processing constants
constexpr int kBlockSize = 24;           // Process audio in blocks of 24 samples
constexpr int kSampleRate = 48000;       // Base sample rate
constexpr int kMaxPolyphony = 16;        // Maximum number of voices
constexpr int kControlRate = kSampleRate / kBlockSize;  // ~2kHz control rate

// Parameter ranges
constexpr int16_t kParameterMin = 0;
constexpr int16_t kParameterMax = 32767;
constexpr int16_t kParameterCenter = 16384;

// Pitch constants  
constexpr int16_t kPitchC4 = 60 << 7;   // MIDI note 60 (C4) in 7-bit fractional format
constexpr int16_t kOctave = 12 << 7;    // One octave in pitch units

// MIDI constants
constexpr int kMidiNoteOff = 0x80;
constexpr int kMidiNoteOn = 0x90;
constexpr int kMidiCC = 0xB0;
constexpr int kMidiPitchBend = 0xE0;

// Utility macros
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete; \
    void operator=(const TypeName&) = delete

// Math constants
constexpr float kPi = 3.14159265358979323846f;
constexpr float k2Pi = 2.0f * kPi;
constexpr float kSqrt2 = 1.41421356237309504880f;

}  // namespace braidy