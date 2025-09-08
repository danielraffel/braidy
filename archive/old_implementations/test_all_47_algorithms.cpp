#include "../Source/BraidyCore/MacroOscillator.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

using namespace braidy;

// WAV header structure for 48kHz 16-bit mono
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

// Algorithm definitions
struct AlgorithmDef {
    MacroOscillatorShape shape;
    const char* name;
    uint16_t default_timbre;
    uint16_t default_color;
};

// All 47 algorithms from Braids
const std::vector<AlgorithmDef> algorithms = {
    // Analog shapes (0-7)
    {MacroOscillatorShape::CSAW, "CSAW", 16384, 16384},
    {MacroOscillatorShape::MORPH, "MORPH", 16384, 16384},
    {MacroOscillatorShape::SAW_SQUARE, "SAW_SQUARE", 16384, 16384},
    {MacroOscillatorShape::SINE_TRIANGLE, "SINE_TRIANGLE", 16384, 16384},
    {MacroOscillatorShape::BUZZ, "BUZZ", 16384, 16384},
    {MacroOscillatorShape::SQUARE_SUB, "SQUARE_SUB", 16384, 16384},
    {MacroOscillatorShape::SAW_SUB, "SAW_SUB", 16384, 16384},
    {MacroOscillatorShape::SQUARE_SYNC, "SQUARE_SYNC", 16384, 16384},
    
    // Waveshaping (8-11)
    {MacroOscillatorShape::SAW_SYNC, "SAW_SYNC", 16384, 16384},
    {MacroOscillatorShape::TRIPLE_SAW, "TRIPLE_SAW", 16384, 16384},
    {MacroOscillatorShape::TRIPLE_SQUARE, "TRIPLE_SQUARE", 16384, 16384},
    {MacroOscillatorShape::TRIPLE_TRIANGLE, "TRIPLE_TRIANGLE", 16384, 16384},
    
    // FM/Ring mod (12-15)
    {MacroOscillatorShape::TRIPLE_SINE, "TRIPLE_SINE", 16384, 16384},
    {MacroOscillatorShape::TRIPLE_RING_MOD, "TRIPLE_RING_MOD", 16384, 16384},
    {MacroOscillatorShape::SAW_SWARM, "SAW_SWARM", 16384, 16384},
    {MacroOscillatorShape::SAW_COMB, "SAW_COMB", 16384, 16384},
    
    // Formant/Chord (16-19)
    {MacroOscillatorShape::TOY, "TOY", 16384, 16384},
    {MacroOscillatorShape::DIGITAL_FILTER_LP, "DIGITAL_FILTER_LP", 16384, 16384},
    {MacroOscillatorShape::DIGITAL_FILTER_PK, "DIGITAL_FILTER_PK", 16384, 16384},
    {MacroOscillatorShape::DIGITAL_FILTER_BP, "DIGITAL_FILTER_BP", 16384, 16384},
    
    // Wavetables (20-23)
    {MacroOscillatorShape::DIGITAL_FILTER_HP, "DIGITAL_FILTER_HP", 16384, 16384},
    {MacroOscillatorShape::VOSIM, "VOSIM", 16384, 16384},
    {MacroOscillatorShape::VOWEL, "VOWEL", 16384, 16384},
    {MacroOscillatorShape::VOWEL_FOF, "VOWEL_FOF", 16384, 16384},
    
    // Digital/harmonics (24-27)
    {MacroOscillatorShape::HARMONICS, "HARMONICS", 16384, 16384},
    {MacroOscillatorShape::FM, "FM", 16384, 16384},
    {MacroOscillatorShape::FEEDBACK_FM, "FEEDBACK_FM", 16384, 16384},
    {MacroOscillatorShape::CHAOTIC_FEEDBACK_FM, "CHAOTIC_FEEDBACK_FM", 16384, 16384},
    
    // Plucked/struck (28-31)
    {MacroOscillatorShape::PLUCKED, "PLUCKED", 16384, 16384},
    {MacroOscillatorShape::BOWED, "BOWED", 16384, 16384},
    {MacroOscillatorShape::BLOWN, "BLOWN", 16384, 16384},
    {MacroOscillatorShape::FLUTED, "FLUTED", 16384, 16384},
    
    // Physical models (32-35)
    {MacroOscillatorShape::STRUCK_BELL, "STRUCK_BELL", 16384, 16384},
    {MacroOscillatorShape::STRUCK_DRUM, "STRUCK_DRUM", 16384, 16384},
    {MacroOscillatorShape::KICK, "KICK", 16384, 16384},
    {MacroOscillatorShape::CYMBAL, "CYMBAL", 16384, 16384},
    
    // Noise (36-39)
    {MacroOscillatorShape::SNARE, "SNARE", 16384, 16384},
    {MacroOscillatorShape::WAVETABLES, "WAVETABLES", 16384, 16384},
    {MacroOscillatorShape::WAVE_MAP, "WAVE_MAP", 16384, 16384},
    {MacroOscillatorShape::WAVE_LINE, "WAVE_LINE", 16384, 16384},
    
    // Additional (40-46)
    {MacroOscillatorShape::WAVE_PARAPHONIC, "WAVE_PARAPHONIC", 16384, 16384},
    {MacroOscillatorShape::FILTERED_NOISE, "FILTERED_NOISE", 16384, 16384},
    {MacroOscillatorShape::TWIN_PEAKS_NOISE, "TWIN_PEAKS_NOISE", 16384, 16384},
    {MacroOscillatorShape::CLOCKED_NOISE, "CLOCKED_NOISE", 16384, 16384},
    {MacroOscillatorShape::GRANULAR_CLOUD, "GRANULAR_CLOUD", 16384, 16384},
    {MacroOscillatorShape::PARTICLE_NOISE, "PARTICLE_NOISE", 16384, 16384},
    {MacroOscillatorShape::DIGITAL_MODULATION, "DIGITAL_MODULATION", 16384, 16384}
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

void GenerateAlgorithmTest(const AlgorithmDef& algo, const char* output_dir) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/braidy_%02d_%s.wav", 
             output_dir, static_cast<int>(algo.shape), algo.name);
    
    printf("\n=== Testing %s (Shape %d) ===\n", algo.name, static_cast<int>(algo.shape));
    
    MacroOscillator osc;
    osc.Init();
    osc.set_shape(algo.shape);
    osc.set_pitch(60 << 7);  // MIDI 60 (Middle C) in Braids format
    osc.set_parameters(algo.default_timbre, algo.default_color);
    
    // Generate 1 second of audio at 48kHz
    const size_t kSampleRate = 48000;
    const size_t kBlockSize = 24;
    const size_t kNumBlocks = kSampleRate / kBlockSize;
    
    std::vector<int16_t> output(kSampleRate, 0);
    uint8_t sync[kBlockSize];
    memset(sync, 0, sizeof(sync));
    
    // Render audio
    bool has_signal = false;
    int peak_value = 0;
    int zero_crossings = 0;
    int16_t last_sample = 0;
    
    for (size_t block = 0; block < kNumBlocks; ++block) {
        int16_t block_buffer[kBlockSize];
        osc.Render(sync, block_buffer, kBlockSize);
        
        for (size_t i = 0; i < kBlockSize; ++i) {
            size_t idx = block * kBlockSize + i;
            if (idx < kSampleRate) {
                output[idx] = block_buffer[i];
                
                // Track signal characteristics
                int abs_val = abs(block_buffer[i]);
                if (abs_val > peak_value) peak_value = abs_val;
                if (abs_val > 100) has_signal = true;
                
                // Count zero crossings
                if (i > 0 && ((last_sample < 0 && block_buffer[i] >= 0) || 
                              (last_sample >= 0 && block_buffer[i] < 0))) {
                    zero_crossings++;
                }
                last_sample = block_buffer[i];
            }
        }
    }
    
    // Calculate DC offset
    long long dc_sum = 0;
    for (const auto& sample : output) {
        dc_sum += sample;
    }
    int dc_offset = dc_sum / output.size();
    
    // Write WAV file
    if (WriteWav(filename, output.data(), output.size())) {
        printf("  ✅ Generated: %s\n", filename);
        printf("  Peak: %d (%.1f%% of max)\n", peak_value, (peak_value * 100.0 / 32768.0));
        printf("  DC Offset: %d\n", dc_offset);
        printf("  Zero Crossings: %d (~%.1f Hz)\n", zero_crossings, zero_crossings / 2.0);
        printf("  Signal: %s\n", has_signal ? "YES" : "NO (silence)");
    } else {
        printf("  ❌ Failed to write file\n");
    }
}

int main() {
    printf("=== JUCE/BRAIDY ALGORITHM TEST ===\n");
    printf("Testing all %zu algorithms at MIDI 60 (Middle C)\n", algorithms.size());
    printf("Output directory: testing/juce_wavs/\n\n");
    
    // Create output directory
    system("mkdir -p testing/juce_wavs");
    
    // Test each algorithm
    int working_count = 0;
    int broken_count = 0;
    
    for (const auto& algo : algorithms) {
        GenerateAlgorithmTest(algo, "testing/juce_wavs");
    }
    
    printf("\n=== TEST COMPLETE ===\n");
    printf("Generated %zu JUCE test files in testing/juce_wavs/\n", algorithms.size());
    printf("\nNext step: Analyze with: python3 testing/analysis/batch_analyze_wavs.py\n");
    
    return 0;
}