// Reference WAV generator using original Braids code
// This creates ground truth WAV files for comparison

#include "macro_oscillator.h"
#include "analog_oscillator.h"
#include "digital_oscillator.h"
#include "../stmlib/utils/dsp.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace braids;
using namespace stmlib;

// WAV header structure
struct WavHeader {
    char chunkID[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize;
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1ID[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = 48000;
    uint32_t byteRate = 96000;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char subchunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size;
};

// Algorithm definitions matching JUCE implementation
struct AlgorithmDef {
    MacroOscillatorShape shape;
    const char* name;
    int16_t default_timbre;
    int16_t default_color;
};

// All 47 algorithms from Braids
const std::vector<AlgorithmDef> algorithms = {
    // Analog shapes (0-7)
    {MACRO_OSC_SHAPE_CSAW, "CSAW", 16384, 16384},
    {MACRO_OSC_SHAPE_MORPH, "MORPH", 16384, 16384},
    {MACRO_OSC_SHAPE_SAW_SQUARE, "SAW_SQUARE", 16384, 16384},
    {MACRO_OSC_SHAPE_SINE_TRIANGLE, "SINE_TRIANGLE", 16384, 16384},
    {MACRO_OSC_SHAPE_BUZZ, "BUZZ", 16384, 16384},
    {MACRO_OSC_SHAPE_SQUARE_SUB, "SQUARE_SUB", 16384, 16384},
    {MACRO_OSC_SHAPE_SAW_SUB, "SAW_SUB", 16384, 16384},
    {MACRO_OSC_SHAPE_SQUARE_SYNC, "SQUARE_SYNC", 16384, 16384},
    
    // Waveshaping (8-11)
    {MACRO_OSC_SHAPE_SAW_SYNC, "SAW_SYNC", 16384, 16384},
    {MACRO_OSC_SHAPE_TRIPLE_SAW, "TRIPLE_SAW", 16384, 16384},
    {MACRO_OSC_SHAPE_TRIPLE_SQUARE, "TRIPLE_SQUARE", 16384, 16384},
    {MACRO_OSC_SHAPE_TRIPLE_TRIANGLE, "TRIPLE_TRIANGLE", 16384, 16384},
    
    // FM/Ring mod (12-15)
    {MACRO_OSC_SHAPE_TRIPLE_SINE, "TRIPLE_SINE", 16384, 16384},
    {MACRO_OSC_SHAPE_TRIPLE_RING_MOD, "TRIPLE_RING_MOD", 16384, 16384},
    {MACRO_OSC_SHAPE_SAW_SWARM, "SAW_SWARM", 16384, 16384},
    {MACRO_OSC_SHAPE_SAW_COMB, "SAW_COMB", 16384, 16384},
    
    // Formant/Chord (16-19)
    {MACRO_OSC_SHAPE_TOY, "TOY", 16384, 16384},
    {MACRO_OSC_SHAPE_DIGITAL_FILTER_LP, "DIGITAL_FILTER_LP", 16384, 16384},
    {MACRO_OSC_SHAPE_DIGITAL_FILTER_PK, "DIGITAL_FILTER_PK", 16384, 16384},
    {MACRO_OSC_SHAPE_DIGITAL_FILTER_BP, "DIGITAL_FILTER_BP", 16384, 16384},
    
    // Wavetables (20-23)
    {MACRO_OSC_SHAPE_DIGITAL_FILTER_HP, "DIGITAL_FILTER_HP", 16384, 16384},
    {MACRO_OSC_SHAPE_VOSIM, "VOSIM", 16384, 16384},
    {MACRO_OSC_SHAPE_VOWEL, "VOWEL", 16384, 16384},
    {MACRO_OSC_SHAPE_VOWEL_FOF, "VOWEL_FOF", 16384, 16384},
    
    // Digital/harmonics (24-27)
    {MACRO_OSC_SHAPE_HARMONICS, "HARMONICS", 16384, 16384},
    {MACRO_OSC_SHAPE_FM, "FM", 16384, 16384},
    {MACRO_OSC_SHAPE_FEEDBACK_FM, "FEEDBACK_FM", 16384, 16384},
    {MACRO_OSC_SHAPE_CHAOTIC_FEEDBACK_FM, "CHAOTIC_FEEDBACK_FM", 16384, 16384},
    
    // Plucked/struck (28-31)
    {MACRO_OSC_SHAPE_PLUCKED, "PLUCKED", 16384, 16384},
    {MACRO_OSC_SHAPE_BOWED, "BOWED", 16384, 16384},
    {MACRO_OSC_SHAPE_BLOWN, "BLOWN", 16384, 16384},
    {MACRO_OSC_SHAPE_FLUTED, "FLUTED", 16384, 16384},
    
    // Physical models (32-35)
    {MACRO_OSC_SHAPE_STRUCK_BELL, "STRUCK_BELL", 16384, 16384},
    {MACRO_OSC_SHAPE_STRUCK_DRUM, "STRUCK_DRUM", 16384, 16384},
    {MACRO_OSC_SHAPE_KICK, "KICK", 16384, 16384},
    {MACRO_OSC_SHAPE_CYMBAL, "CYMBAL", 16384, 16384},
    
    // Noise (36-39)
    {MACRO_OSC_SHAPE_SNARE, "SNARE", 16384, 16384},
    {MACRO_OSC_SHAPE_WAVETABLES, "WAVETABLES", 16384, 16384},
    {MACRO_OSC_SHAPE_WAVE_MAP, "WAVE_MAP", 16384, 16384},
    {MACRO_OSC_SHAPE_WAVE_LINE, "WAVE_LINE", 16384, 16384},
    
    // Additional (40-46)
    {MACRO_OSC_SHAPE_WAVE_PARAPHONIC, "WAVE_PARAPHONIC", 16384, 16384},
    {MACRO_OSC_SHAPE_FILTERED_NOISE, "FILTERED_NOISE", 16384, 16384},
    {MACRO_OSC_SHAPE_TWIN_PEAKS_NOISE, "TWIN_PEAKS_NOISE", 16384, 16384},
    {MACRO_OSC_SHAPE_CLOCKED_NOISE, "CLOCKED_NOISE", 16384, 16384},
    {MACRO_OSC_SHAPE_GRANULAR_CLOUD, "GRANULAR_CLOUD", 16384, 16384},
    {MACRO_OSC_SHAPE_PARTICLE_NOISE, "PARTICLE_NOISE", 16384, 16384},
    {MACRO_OSC_SHAPE_DIGITAL_MODULATION, "DIGITAL_MODULATION", 16384, 16384}
};

bool WriteWav(const char* filename, const int16_t* data, size_t numSamples) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("❌ Failed to open %s for writing\n", filename);
        return false;
    }
    
    WavHeader header;
    header.subchunk2Size = numSamples * sizeof(int16_t);
    header.chunkSize = 36 + header.subchunk2Size;
    
    fwrite(&header, sizeof(header), 1, file);
    fwrite(data, sizeof(int16_t), numSamples, file);
    fclose(file);
    
    return true;
}

void GenerateReferenceWav(const AlgorithmDef& algo, const char* output_dir) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/braids_ref_%02d_%s.wav", 
             output_dir, static_cast<int>(algo.shape), algo.name);
    
    printf("Generating reference: %s\n", algo.name);
    
    // Initialize the macro oscillator
    MacroOscillator osc;
    osc.Init();
    osc.set_shape(algo.shape);
    osc.set_pitch(60 << 7);  // MIDI 60 (Middle C) in Braids format
    osc.set_parameters(algo.default_timbre, algo.default_color);
    
    // Generate 1 second of audio at 48kHz
    const size_t kSampleRate = 48000;
    const size_t kBlockSize = 24;  // Braids standard block size
    const size_t kNumBlocks = kSampleRate / kBlockSize;
    
    std::vector<int16_t> output(kSampleRate, 0);
    uint8_t sync[kBlockSize];
    memset(sync, 0, sizeof(sync));
    
    // Render audio
    for (size_t block = 0; block < kNumBlocks; ++block) {
        int16_t block_buffer[kBlockSize];
        osc.Render(sync, block_buffer, kBlockSize);
        
        // Copy to output buffer
        for (size_t i = 0; i < kBlockSize; ++i) {
            size_t idx = block * kBlockSize + i;
            if (idx < kSampleRate) {
                output[idx] = block_buffer[i];
            }
        }
    }
    
    // Write WAV file
    if (WriteWav(filename, output.data(), output.size())) {
        printf("  ✅ Generated: %s\n", filename);
    } else {
        printf("  ❌ Failed to write\n");
    }
}

int main(int argc, char* argv[]) {
    printf("=== BRAIDS REFERENCE WAV GENERATOR ===\n");
    printf("Generating reference files from original Braids code\n\n");
    
    // Create output directory
    system("mkdir -p braids_reference");
    
    // Generate reference for specific algorithm if requested
    if (argc > 1) {
        // Find matching algorithm
        bool found = false;
        for (const auto& algo : algorithms) {
            if (strcmp(argv[1], algo.name) == 0) {
                GenerateReferenceWav(algo, "braids_reference");
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Algorithm '%s' not found\n", argv[1]);
            printf("Available algorithms:\n");
            for (const auto& algo : algorithms) {
                printf("  %s\n", algo.name);
            }
        }
    } else {
        // Generate all references
        for (const auto& algo : algorithms) {
            GenerateReferenceWav(algo, "braids_reference");
        }
        printf("\n✅ Generated %zu reference files in braids_reference/\n", algorithms.size());
    }
    
    return 0;
}