// Braids Reference WAV Generator for Parity Testing
// Based on the original braids_test.cc but simplified

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <vector>

#include "macro_oscillator.h"

using namespace braids;

const uint32_t kSampleRate = 48000;  // Use 48kHz to match JUCE
const uint16_t kAudioBlockSize = 24;

// Simple WAV writer (no stmlib dependency)
class SimpleWavWriter {
private:
    std::ofstream file;
    std::vector<int16_t> samples;
    
public:
    bool Open(const char* filename) {
        file.open(filename, std::ios::binary);
        return file.is_open();
    }
    
    void WriteFrames(int16_t* buffer, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            samples.push_back(buffer[i]);
        }
    }
    
    void Close() {
        if (!file.is_open()) return;
        
        // Write WAV header
        file.write("RIFF", 4);
        uint32_t file_size = 36 + samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&file_size), 4);
        file.write("WAVE", 4);
        
        // Format chunk
        file.write("fmt ", 4);
        uint32_t fmt_size = 16;
        file.write(reinterpret_cast<const char*>(&fmt_size), 4);
        uint16_t audio_format = 1;  // PCM
        file.write(reinterpret_cast<const char*>(&audio_format), 2);
        uint16_t num_channels = 1;
        file.write(reinterpret_cast<const char*>(&num_channels), 2);
        uint32_t sample_rate = kSampleRate;
        file.write(reinterpret_cast<const char*>(&sample_rate), 4);
        uint32_t byte_rate = sample_rate * 2;
        file.write(reinterpret_cast<const char*>(&byte_rate), 4);
        uint16_t block_align = 2;
        file.write(reinterpret_cast<const char*>(&block_align), 2);
        uint16_t bits_per_sample = 16;
        file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        
        // Data chunk
        file.write("data", 4);
        uint32_t data_size = samples.size() * 2;
        file.write(reinterpret_cast<const char*>(&data_size), 4);
        file.write(reinterpret_cast<const char*>(samples.data()), data_size);
        
        file.close();
        printf("Wrote %zu samples to WAV\n", samples.size());
    }
    
    ~SimpleWavWriter() {
        Close();
    }
};

void GenerateCSAWReference() {
    MacroOscillator osc;
    SimpleWavWriter wav_writer;
    
    if (!wav_writer.Open("braids_csaw_reference.wav")) {
        printf("Failed to open output file\n");
        return;
    }
    
    // Initialize with deterministic seed
    // Note: Braids uses internal random generator, we need to ensure determinism
    
    osc.Init();
    osc.set_shape(MACRO_OSC_SHAPE_CSAW);  // CSAW algorithm
    osc.set_pitch(60 << 7);  // Middle C (MIDI 60) in Braids format
    osc.set_parameters(16384, 16384);  // TIMBRE=50%, COLOR=50%
    
    printf("Generating CSAW reference at 48kHz...\n");
    printf("Parameters: MIDI=60, TIMBRE=16384, COLOR=16384\n");
    
    // Generate 1 second of audio
    uint32_t total_blocks = kSampleRate / kAudioBlockSize;
    
    for (uint32_t i = 0; i < total_blocks; ++i) {
        int16_t buffer[kAudioBlockSize];
        uint8_t sync_buffer[kAudioBlockSize];
        
        // No sync for this test
        memset(sync_buffer, 0, sizeof(sync_buffer));
        
        // Keep parameters constant
        osc.set_parameters(16384, 16384);
        osc.set_pitch(60 << 7);
        
        osc.Render(sync_buffer, buffer, kAudioBlockSize);
        wav_writer.WriteFrames(buffer, kAudioBlockSize);
    }
    
    printf("Generated braids_csaw_reference.wav successfully!\n");
}

int main(void) {
    GenerateCSAWReference();
    return 0;
}