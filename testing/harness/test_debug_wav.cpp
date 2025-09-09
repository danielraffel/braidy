/**
 * test_debug_wav.cpp
 * Debug test that saves outputs to WAV files for inspection
 */

#include "../../eurorack/braids/macro_oscillator.h"
#include "../../Source/adapters/BraidsEngine.h"
#include "BraidsReference.h"
#include "ModeRegistry.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>

const int SAMPLE_RATE = 48000;
const int TEST_DURATION = SAMPLE_RATE * 2; // 2 seconds

// Simple WAV file writer
void writeWav(const std::string& filename, const float* data, int numSamples, int sampleRate) {
    std::ofstream file(filename, std::ios::binary);
    
    // WAV header
    file.write("RIFF", 4);
    uint32_t fileSize = 36 + numSamples * 2; // 16-bit audio
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t format = 1; // PCM
    file.write(reinterpret_cast<const char*>(&format), 2);
    uint16_t channels = 1;
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * 2;
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = 2;
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 16;
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    file.write("data", 4);
    uint32_t dataSize = numSamples * 2;
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
    
    // Convert float to int16 and write
    for (int i = 0; i < numSamples; i++) {
        float sample = data[i];
        // Clip
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&intSample), 2);
    }
    
    file.close();
    std::cout << "Wrote " << filename << " (" << numSamples << " samples)\n";
}

void printStats(const std::string& name, const float* audio, int numSamples) {
    float min = audio[0], max = audio[0], sum = 0, sumSq = 0;
    int zeroCount = 0;
    
    for (int i = 0; i < numSamples; i++) {
        float sample = audio[i];
        if (sample < min) min = sample;
        if (sample > max) max = sample;
        sum += sample;
        sumSq += sample * sample;
        if (std::abs(sample) < 0.0001f) zeroCount++;
    }
    
    float mean = sum / numSamples;
    float rms = std::sqrt(sumSq / numSamples);
    
    std::cout << name << " stats:\n";
    std::cout << "  Range: [" << min << ", " << max << "]\n";
    std::cout << "  Mean: " << mean << ", RMS: " << rms << "\n";
    std::cout << "  Zero samples: " << zeroCount << "/" << numSamples 
              << " (" << (100.0f * zeroCount / numSamples) << "%)\n";
}

int main() {
    std::cout << "Debug WAV Test - Comparing Braids implementations\n";
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz\n";
    std::cout << "Duration: " << TEST_DURATION << " samples (" 
              << (float)TEST_DURATION/SAMPLE_RATE << " seconds)\n\n";
    
    ModeRegistry::initialize();
    
    // Test CSAW algorithm
    std::cout << "Testing CSAW algorithm...\n";
    
    // 1. Direct Braids
    {
        std::cout << "\n1. Direct Braids MacroOscillator:\n";
        braids::MacroOscillator osc;
        osc.Init();
        osc.set_shape(braids::MACRO_OSC_SHAPE_CSAW);
        osc.set_pitch(60 << 7); // MIDI note 60
        osc.set_parameters(16384, 16384); // Mid values
        
        std::vector<int16_t> render_buffer(TEST_DURATION);
        std::vector<uint8_t> sync_buffer(TEST_DURATION);
        std::vector<float> float_buffer(TEST_DURATION);
        
        // Process in 24-sample blocks
        int pos = 0;
        while (pos < TEST_DURATION) {
            int samples = std::min(24, TEST_DURATION - pos);
            osc.Render(&sync_buffer[pos], &render_buffer[pos], samples);
            pos += samples;
        }
        
        // Convert to float
        for (int i = 0; i < TEST_DURATION; i++) {
            float_buffer[i] = render_buffer[i] / 32768.0f;
        }
        
        printStats("Direct Braids", float_buffer.data(), TEST_DURATION);
        writeWav("debug_direct_braids.wav", float_buffer.data(), TEST_DURATION, SAMPLE_RATE);
    }
    
    // 2. BraidsReference
    {
        std::cout << "\n2. BraidsReference:\n";
        BraidsReference ref(SAMPLE_RATE, 24);
        ref.setShape(ModeRegistry::getShapeForName("CSAW"));
        ref.setFrequency(261.63f); // Middle C
        ref.setTimbre(0.5f);
        ref.setColor(0.5f);
        
        std::vector<float> audio_buffer(TEST_DURATION);
        
        // Process in 24-sample blocks
        int pos = 0;
        while (pos < TEST_DURATION) {
            int samples = ref.generateBlock(&audio_buffer[pos]);
            pos += samples;
            if (samples == 0) break; // Safety check
        }
        
        printStats("BraidsReference", audio_buffer.data(), TEST_DURATION);
        writeWav("debug_reference.wav", audio_buffer.data(), TEST_DURATION, SAMPLE_RATE);
    }
    
    // 3. BraidsEngine
    {
        std::cout << "\n3. BraidsEngine:\n";
        BraidyAdapter::BraidsEngine engine;
        engine.initialize(SAMPLE_RATE);
        engine.setAlgorithm(0); // CSAW
        engine.setPitch(60.0f); // MIDI note 60
        engine.setParameters(0.5f, 0.5f); // Mid values
        
        std::vector<float> audio_buffer(TEST_DURATION);
        
        // Process all at once
        engine.processAudio(audio_buffer.data(), TEST_DURATION);
        
        printStats("BraidsEngine", audio_buffer.data(), TEST_DURATION);
        writeWav("debug_engine.wav", audio_buffer.data(), TEST_DURATION, SAMPLE_RATE);
    }
    
    std::cout << "\nWAV files written. Use an audio editor to compare:\n";
    std::cout << "  debug_direct_braids.wav - Direct Braids output\n";
    std::cout << "  debug_reference.wav - BraidsReference output\n";
    std::cout << "  debug_engine.wav - BraidsEngine output\n";
    
    return 0;
}