// JUCE Braidy WAV Generator for Parity Testing
// This generates WAV files using the JUCE Braidy implementation

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "BraidyCore/MacroOscillator.h"
#include "BraidyCore/BraidyResources.h"
#include "BraidyCore/BraidyMath.h"

using namespace braidy;

class JUCEWavWriter {
private:
    std::vector<int16_t> samples;
    uint32_t sample_rate;
    
public:
    JUCEWavWriter(uint32_t sr) : sample_rate(sr) {}
    
    void WriteFrames(int16_t* buffer, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            samples.push_back(buffer[i]);
        }
    }
    
    bool SaveWAV(const char* filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        // WAV header
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
        std::cout << "Wrote " << samples.size() << " samples to " << filename << std::endl;
        return true;
    }
    
    size_t GetSampleCount() const { return samples.size(); }
};

class BraidyTestGenerator {
private:
    MacroOscillator osc;
    const uint32_t kSampleRate = 48000;
    const size_t kBlockSize = 24;
    
public:
    void GenerateCSAWTest() {
        std::cout << "\n=== BRAIDY CSAW WAV GENERATOR ===" << std::endl;
        
        // Initialize resources
        InitializeResources();
        
        // Initialize oscillator
        osc.Init();
        osc.set_shape(MacroOscillatorShape::CSAW);
        osc.set_pitch(60 << 7);  // MIDI 60 (Middle C) in Braids format
        osc.set_parameters(16384, 16384);  // TIMBRE=50%, COLOR=50%
        
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Algorithm: CSAW" << std::endl;
        std::cout << "  Sample Rate: " << kSampleRate << " Hz" << std::endl;
        std::cout << "  Pitch: MIDI 60 (Middle C)" << std::endl;
        std::cout << "  TIMBRE: 16384 (50%)" << std::endl;
        std::cout << "  COLOR: 16384 (50%)" << std::endl;
        std::cout << "  Duration: 1 second" << std::endl;
        
        JUCEWavWriter wav_writer(kSampleRate);
        
        // Generate 1 second of audio
        uint32_t total_samples = kSampleRate;
        uint32_t samples_generated = 0;
        
        while (samples_generated < total_samples) {
            size_t block_size = std::min(kBlockSize, (size_t)(total_samples - samples_generated));
            
            int16_t buffer[24];
            uint8_t sync_buffer[24];
            memset(sync_buffer, 0, sizeof(sync_buffer));  // No sync
            
            // Render block
            osc.Render(sync_buffer, buffer, block_size);
            
            // Write to WAV
            wav_writer.WriteFrames(buffer, block_size);
            
            samples_generated += block_size;
            
            // Progress indicator
            if ((samples_generated % (kSampleRate / 10)) == 0) {
                int progress = (samples_generated * 100) / total_samples;
                std::cout << "Progress: " << progress << "%" << std::endl;
            }
        }
        
        // Save WAV file
        if (wav_writer.SaveWAV("braidy_csaw_candidate.wav")) {
            std::cout << "✅ Generated braidy_csaw_candidate.wav successfully!" << std::endl;
        } else {
            std::cout << "❌ Failed to save WAV file!" << std::endl;
        }
    }
    
    void GenerateMultipleAlgorithms() {
        std::cout << "\n=== GENERATING MULTIPLE ALGORITHMS ===" << std::endl;
        
        struct TestAlgorithm {
            MacroOscillatorShape shape;
            const char* name;
            int16_t timbre;
            int16_t color;
        };
        
        TestAlgorithm algorithms[] = {
            {MacroOscillatorShape::CSAW, "csaw", 16384, 16384},
            {MacroOscillatorShape::MORPH, "morph", 8192, 16384},
            {MacroOscillatorShape::SAW_SQUARE, "saw_square", 16384, 8192},
            {MacroOscillatorShape::BUZZ, "buzz", 24576, 16384},
            {MacroOscillatorShape::HARMONICS, "harmonics", 16384, 24576}
        };
        
        InitializeResources();
        
        for (const auto& algo : algorithms) {
            std::cout << "Generating " << algo.name << "..." << std::endl;
            
            osc.Init();
            osc.set_shape(algo.shape);
            osc.set_pitch(60 << 7);
            osc.set_parameters(algo.timbre, algo.color);
            
            JUCEWavWriter wav_writer(kSampleRate);
            
            // Generate 2 seconds for each algorithm
            uint32_t total_samples = kSampleRate * 2;
            uint32_t samples_generated = 0;
            
            while (samples_generated < total_samples) {
                size_t block_size = std::min(kBlockSize, (size_t)(total_samples - samples_generated));
                
                int16_t buffer[24];
                uint8_t sync_buffer[24];
                memset(sync_buffer, 0, sizeof(sync_buffer));
                
                osc.Render(sync_buffer, buffer, block_size);
                wav_writer.WriteFrames(buffer, block_size);
                
                samples_generated += block_size;
            }
            
            std::string filename = "braidy_" + std::string(algo.name) + "_test.wav";
            wav_writer.SaveWAV(filename.c_str());
        }
        
        std::cout << "✅ Generated test WAVs for multiple algorithms!" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    BraidyTestGenerator generator;
    
    if (argc > 1 && strcmp(argv[1], "multi") == 0) {
        generator.GenerateMultipleAlgorithms();
    } else {
        generator.GenerateCSAWTest();
    }
    
    return 0;
}