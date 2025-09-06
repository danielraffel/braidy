#pragma once

#include "BraidyTypes.h"
#include "BraidyMath.h"
#include "BraidyResources.h"

namespace braidy {

// Digital oscillator shapes for render dispatch
enum class DigitalOscillatorShape {
    TRIPLE_RING_MOD = 0,
    SAW_SWARM,
    COMB_FILTER,
    TOY,
    
    // Digital filters
    DIGITAL_FILTER_LP,
    DIGITAL_FILTER_PK,
    DIGITAL_FILTER_BP,
    DIGITAL_FILTER_HP,
    
    // Formant synthesis
    VOSIM,
    VOWEL,
    VOWEL_FOF,
    
    HARMONICS,
    
    // FM synthesis
    FM,
    FEEDBACK_FM,
    CHAOTIC_FEEDBACK_FM,
    
    // Physical modeling - percussion
    STRUCK_BELL,
    STRUCK_DRUM,
    
    KICK,
    CYMBAL,
    SNARE,
    
    // Physical modeling - strings and winds  
    PLUCKED,
    BOWED,
    BLOWN,
    FLUTED,
    
    // Wavetables
    WAVETABLES,
    WAVE_MAP,
    WAVE_LINE,
    WAVE_PARAPHONIC,
    
    // Noise and granular
    FILTERED_NOISE,
    TWIN_PEAKS_NOISE,
    CLOCKED_NOISE,
    GRANULAR_CLOUD,
    PARTICLE_NOISE,
    DIGITAL_MODULATION,
    
    LAST
};

// State for various synthesis models
union OscillatorState {
    struct {
        uint32_t formant_phase[3];
        uint32_t formant_increment[3];
        uint16_t formant_amplitude[3];
        uint16_t consonant_frames;
        int32_t noise;
    } vow;
    
    struct {
        uint32_t carrier_phase;
        uint32_t modulator_phase;
        uint32_t previous_sample[2];
    } fm;
    
    struct {
        int16_t harmonics[14];
        uint32_t phase[14];
        uint32_t increment[14];
    } harm;
    
    struct {
        int32_t amplitude[14];
        int16_t previous_sample;
        uint32_t partial_phase[6];
        int32_t target_partial_amplitude[6];
        int32_t partial_amplitude[6];
    } hrm;
    
    struct {
        uint16_t delay_line[8192];
        uint16_t delay_ptr;
        uint16_t delay_length;
        int32_t lp_state;
    } comb;
    
    struct {
        int16_t string_1[1024];
        int16_t string_2[1024];
        int16_t string_3[1024];
        uint16_t ptr_1, ptr_2, ptr_3;
        uint16_t length_1, length_2, length_3;
        int16_t damping;
        int16_t excitation_state;
    } pluk;
    
    struct {
        float filter_state[4];
        float frequency;
        float resonance;
    } filt;
    
    struct {
        uint32_t phase[8];
        int16_t level[8];
        uint16_t wavetable_index;
    } wtbl;
    
    struct {
        uint32_t lfsr;
        int32_t integrator_state;
        int32_t band_limited_impulse;
        uint32_t next_sample;
    } noise;
    
    struct {
        uint32_t bell_phase[11];
        int16_t bell_amplitude[11];
    } bell;
    
    struct {
        uint32_t drum_phase[6];
        int16_t drum_amplitude[6];
        uint16_t drum_decay[6];
    } drum;
    
    struct {
        int32_t svf_state[2];
        uint32_t punch_phase;
        uint32_t tone_phase;
    } kick;
    
    struct {
        uint32_t hihat_phase[6];
        int16_t hihat_amplitude[6];
        uint32_t click_phase;
        int32_t filter_state;
    } cymb;
    
    struct {
        uint32_t noise_phase;
        uint32_t tone_phase;
        int32_t filter_state_1;
        int32_t filter_state_2;
        uint16_t decay;
    } snar;
};

class DigitalOscillator {
public:
    using RenderFn = void (DigitalOscillator::*)(const uint8_t*, int16_t*, size_t);
    
    DigitalOscillator();
    ~DigitalOscillator() = default;
    
    void Init();
    void Strike();
    void Render(MacroOscillatorShape shape, int16_t pitch, 
               int16_t parameter_1, int16_t parameter_2,
               const uint8_t* sync, int16_t* buffer, size_t size);
    
private:
    // Pitch and phase utilities
    uint32_t ComputePhaseIncrement(int16_t midi_pitch);
    uint32_t ComputeDelay(int16_t midi_pitch);
    
    // Digital synthesis renders
    void RenderTripleRingMod(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSawSwarm(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderCombFilter(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderToy(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Digital filter synthesis
    void RenderDigitalFilterLP(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderDigitalFilterPK(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderDigitalFilterBP(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderDigitalFilterHP(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Formant synthesis
    void RenderVosim(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderVowel(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderVowelFof(const uint8_t* sync, int16_t* buffer, size_t size);
    
    void RenderHarmonics(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // FM synthesis  
    void RenderFM(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderFeedbackFM(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderChaoticFeedbackFM(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Physical modeling - percussion
    void RenderStruckBell(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderStruckDrum(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderKick(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderCymbal(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSnare(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Physical modeling - strings and winds
    void RenderPlucked(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderBowed(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderBlown(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderFluted(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Wavetables
    void RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderWaveMap(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderWaveLine(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderWaveParaphonic(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Noise and granular
    void RenderFilteredNoise(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderTwinPeaksNoise(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderClockedNoise(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderGranularCloud(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderParticleNoise(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderDigitalModulation(const uint8_t* sync, int16_t* buffer, size_t size);
    
    // Helper functions
    int16_t InterpolateWaveform(const int16_t* wavetable, uint32_t phase, uint32_t size);
    void UpdateFormants(int16_t vowel_param);
    void UpdateHarmonics(int16_t harmonic_param);
    void ProcessFilter(float* state, float input, float frequency, float resonance, int16_t* output);
    uint32_t Rng() { return rng_state_ = rng_state_ * 1664525L + 1013904223L; }
    
    // State variables
    DigitalOscillatorShape shape_;
    int16_t pitch_;
    int16_t parameter_[2];
    int16_t previous_parameter_[2];
    
    uint32_t phase_;
    uint32_t phase_increment_;
    
    // Strike/excitation state
    bool struck_;
    uint16_t excitation_fm_;
    int16_t strike_level_;
    
    // Random number generator
    uint32_t rng_state_;
    
    // Complex synthesis state
    OscillatorState state_;
    
    // Function lookup table  
    static const RenderFn fn_table_[];
    
    DISALLOW_COPY_AND_ASSIGN(DigitalOscillator);
};

}  // namespace braidy