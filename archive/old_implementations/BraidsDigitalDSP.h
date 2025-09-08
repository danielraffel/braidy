// Braids Digital DSP Algorithms
// Properly imported from original Mutable Instruments Braids source
// This file contains the actual DSP implementations for accurate sound reproduction

#ifndef BRAIDS_DIGITAL_DSP_H_
#define BRAIDS_DIGITAL_DSP_H_

#include <cstdint>
#include <cstring>
#include <algorithm>
#include "BraidyResources.h"
#include "FormantTables.h"
#include "GranularSynthesis.h"
#include "ChaoticSystems.h"
#include "ParameterCurves.h"
#include "DigitalOscillator.h"

namespace braidy {

// Helper functions for wavetable interpolation
inline int16_t Interpolate824(const int16_t* table, uint32_t phase) {
    uint32_t index = phase >> 24;
    uint32_t fraction = (phase >> 8) & 0xFFFF;
    int16_t a = table[index];
    int16_t b = table[index + 1];
    return a + ((b - a) * fraction >> 16);
}

inline uint16_t InterpolateU824(const uint16_t* table, uint32_t phase) {
    uint32_t index = phase >> 24;
    uint32_t fraction = (phase >> 8) & 0xFFFF;
    uint16_t a = table[index];
    uint16_t b = table[index + 1];
    return a + ((b - a) * fraction >> 16);
}

// Formant filter coefficients for vowel synthesis
struct FormantFilterCoefficients {
    int16_t a;
    int16_t b;
    int16_t c;
    int16_t d;
    int16_t e;
};

// State for vowel/formant synthesis
struct VowelState {
    uint32_t formant_phase[4];
    uint32_t formant_increment[4];
    int32_t formant_amplitude[4];
    int32_t formant_amplitude_previous[4];
    int32_t consonant_frames;
};

// State for FM synthesis
struct FMState {
    uint32_t modulator_phase;
    uint32_t carrier_phase;
    int32_t modulator_state;
    int32_t feedback;
    int32_t feedback_previous;
};

// State for physical models
struct PhysicalModelState {
    int16_t delay_line[4096];
    uint16_t write_ptr;
    int32_t filter_state;
    int32_t pluck_damping;
    int32_t string_state;
};

// State for noise generators
struct NoiseState {
    uint32_t rng_state;
    int32_t filter_state[4];
    uint32_t clock_phase;
    int16_t sample_hold;
};

// State for cymbal synthesis
struct CymbalState {
    uint32_t hihat_phase[6];  // 6 metallic oscillators
    int32_t hihat_amplitude[6];
    int32_t filter_state;
};

// State for drum synthesis
struct DrumState {
    uint32_t drum_phase[4];  // 4 drum modes
    int32_t drum_amplitude[4];
};

// Complete digital oscillator state
struct DigitalOscillatorState {
    VowelState vow;
    FMState fm;
    PhysicalModelState phys;
    NoiseState noise;
    CymbalState cymb;
    DrumState drum;
    uint32_t secondary_phase;
};

class BraidsDigitalDSP {
public:
    BraidsDigitalDSP() {
        memset(&state_, 0, sizeof(state_));
        state_.noise.rng_state = 1;
        granular_.Clear();
        chaotic_.Reset();
    }
    
    // VOSIM synthesis - creates formant-like tones
    void RenderVosim(int16_t pitch, int16_t* parameter, 
                     uint32_t phase, uint32_t phase_increment,
                     const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Vowel synthesis using formant filters
    void RenderVowel(int16_t pitch, int16_t* parameter,
                     uint32_t phase, uint32_t phase_increment,
                     const uint8_t* sync, int16_t* buffer, size_t size);
    
    // FOF (Fonction d'Onde Formantique) synthesis
    void RenderVowelFOF(int16_t pitch, int16_t* parameter,
                        uint32_t phase, uint32_t phase_increment,
                        const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Harmonic synthesis with controllable harmonics
    void RenderHarmonics(int16_t pitch, int16_t* parameter,
                         uint32_t phase, uint32_t phase_increment,
                         const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Triple ring modulation
    void RenderTripleRingMod(int16_t pitch, int16_t* parameter,
                             uint32_t phase, uint32_t phase_increment,
                             const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Saw swarm - multiple detuned saws
    void RenderSawSwarm(int16_t pitch, int16_t* parameter,
                        uint32_t phase, uint32_t phase_increment,
                        const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Comb filter synthesis
    void RenderCombFilter(int16_t pitch, int16_t* parameter,
                          uint32_t phase, uint32_t phase_increment,
                          const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Toy/lo-fi synthesis
    void RenderToy(int16_t pitch, int16_t* parameter,
                   uint32_t phase, uint32_t phase_increment,
                   const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Digital filter synthesis (LP/HP/BP/PK)
    void RenderDigitalFilter(int filter_type, int16_t pitch, int16_t* parameter,
                             uint32_t phase, uint32_t phase_increment,
                             const uint8_t* sync, int16_t* buffer, size_t size);
    
    // FM synthesis with proper ratios
    void RenderFM(int16_t pitch, int16_t* parameter,
                  uint32_t phase, uint32_t phase_increment,
                  const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Feedback FM
    void RenderFeedbackFM(int16_t pitch, int16_t* parameter,
                          uint32_t phase, uint32_t phase_increment,
                          const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Chaotic feedback FM
    void RenderChaoticFeedbackFM(int16_t pitch, int16_t* parameter,
                                  uint32_t phase, uint32_t phase_increment,
                                  const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Karplus-Strong plucked string
    void RenderPlucked(int16_t pitch, int16_t* parameter,
                       const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Bowed string physical model
    void RenderBowed(int16_t pitch, int16_t* parameter,
                     const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Wind instrument physical models
    void RenderBlown(int16_t pitch, int16_t* parameter,
                     const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderFluted(int16_t pitch, int16_t* parameter,
                      const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Percussion synthesis
    void RenderStruckBell(int16_t pitch, int16_t* parameter,
                          const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderStruckDrum(int16_t pitch, int16_t* parameter,
                          const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderKick(int16_t pitch, int16_t* parameter,
                    const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderCymbal(int16_t pitch, int16_t* parameter,
                      const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderSnare(int16_t pitch, int16_t* parameter,
                     const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Wavetable synthesis
    void RenderWavetables(int16_t pitch, int16_t* parameter,
                          uint32_t phase, uint32_t phase_increment,
                          const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Noise generators
    void RenderFilteredNoise(int16_t pitch, int16_t* parameter,
                             const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderTwinPeaksNoise(int16_t pitch, int16_t* parameter,
                              const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderClockedNoise(int16_t pitch, int16_t* parameter,
                            const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderGranularCloud(int16_t pitch, int16_t* parameter,
                             const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderParticleNoise(int16_t pitch, int16_t* parameter,
                             const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderDigitalModulation(int16_t pitch, int16_t* parameter,
                                const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Strike trigger for percussive sounds
    void Strike() {
        state_.phys.pluck_damping = 32767;
        state_.fm.feedback = 0;
        // Reset delay line for physical models
        memset(state_.phys.delay_line, 0, sizeof(state_.phys.delay_line));
        state_.phys.write_ptr = 0;
    }
    
    // Compute phase increment from MIDI pitch
    static uint32_t ComputePhaseIncrement(int16_t midi_pitch);
    
    // Compute delay length for physical models
    static uint32_t ComputeDelay(int16_t midi_pitch);
    
private:
    DigitalOscillatorState state_;
    GranularSynthesis granular_;
    ChaoticSystems chaotic_;
    
    // Helper function for noise generation
    inline uint32_t GetNoise() {
        state_.noise.rng_state = state_.noise.rng_state * 1664525 + 1013904223;
        return state_.noise.rng_state;
    }
    
    // Soft clipping
    inline int16_t SoftClip(int32_t x) {
        if (x < -32768) x = -32768;
        if (x > 32767) x = 32767;
        return static_cast<int16_t>(x);
    }
    
    // One-pole lowpass filter
    inline int32_t OnePole(int32_t input, int32_t state, int16_t coefficient) {
        return state + ((coefficient * (input - state)) >> 15);
    }
};

} // namespace braidy

#endif // BRAIDS_DIGITAL_DSP_H_