/**
 * test_simple.cpp
 * Simple diagnostic test to verify Braids integration
 */

#include "../../eurorack/braids/macro_oscillator.h"
#include "../../Source/adapters/BraidsEngine.h"
#include "BraidsReference.h"
#include "ModeRegistry.h"
#include <iostream>
#include <vector>
#include <cmath>

const int SAMPLE_RATE = 48000;
const int TEST_SAMPLES = 1024;

void printAudioStats(const std::string& name, const float* audio, int numSamples) {
    float min = audio[0], max = audio[0], sum = 0, sumSq = 0;
    int nonZero = 0;
    
    for (int i = 0; i < numSamples; i++) {
        float sample = audio[i];
        if (sample < min) min = sample;
        if (sample > max) max = sample;
        sum += sample;
        sumSq += sample * sample;
        if (std::abs(sample) > 0.0001f) nonZero++;
    }
    
    float mean = sum / numSamples;
    float rms = std::sqrt(sumSq / numSamples);
    
    std::cout << name << ":\n";
    std::cout << "  Min: " << min << ", Max: " << max << "\n";
    std::cout << "  Mean: " << mean << ", RMS: " << rms << "\n";
    std::cout << "  Non-zero samples: " << nonZero << "/" << numSamples << "\n";
}

void testDirectBraids() {
    std::cout << "\n=== Testing Direct Braids MacroOscillator ===\n";
    
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_shape(braids::MACRO_OSC_SHAPE_CSAW);
    osc.set_pitch(60 << 7); // MIDI note 60, shifted left by 7
    osc.set_parameters(16384, 16384); // Mid values
    
    // Allocate buffers
    std::vector<int16_t> render_buffer(TEST_SAMPLES);
    std::vector<uint8_t> sync_buffer(TEST_SAMPLES);
    std::vector<float> float_buffer(TEST_SAMPLES);
    
    // Process in 24-sample blocks
    int pos = 0;
    while (pos < TEST_SAMPLES) {
        int samples = std::min(24, TEST_SAMPLES - pos);
        osc.Render(&sync_buffer[pos], &render_buffer[pos], samples);
        pos += samples;
    }
    
    // Convert to float
    for (int i = 0; i < TEST_SAMPLES; i++) {
        float_buffer[i] = render_buffer[i] / 32768.0f;
    }
    
    printAudioStats("Direct Braids", float_buffer.data(), TEST_SAMPLES);
}

void testBraidsEngine() {
    std::cout << "\n=== Testing BraidsEngine Wrapper ===\n";
    
    BraidyAdapter::BraidsEngine engine;
    engine.initialize(SAMPLE_RATE);
    engine.setAlgorithm(0); // CSAW
    engine.setPitch(60.0f); // MIDI note 60
    engine.setParameters(0.5f, 0.5f); // Mid values
    
    std::vector<float> audio_buffer(TEST_SAMPLES);
    
    // Process all at once
    engine.processAudio(audio_buffer.data(), TEST_SAMPLES);
    
    printAudioStats("BraidsEngine", audio_buffer.data(), TEST_SAMPLES);
}

void testBraidsReference() {
    std::cout << "\n=== Testing BraidsReference ===\n";
    
    ModeRegistry::initialize();
    BraidsReference ref(SAMPLE_RATE, 24); // 24-sample blocks
    ref.setShape(ModeRegistry::getShapeForName("CSAW"));
    ref.setFrequency(261.63f); // Middle C
    ref.setTimbre(0.5f);
    ref.setColor(0.5f);
    
    std::vector<float> audio_buffer(TEST_SAMPLES);
    
    // Process in 24-sample blocks
    int pos = 0;
    while (pos < TEST_SAMPLES) {
        int samples = ref.generateBlock(&audio_buffer[pos]);
        pos += samples;
    }
    
    printAudioStats("BraidsReference", audio_buffer.data(), TEST_SAMPLES);
}

int main() {
    std::cout << "Simple Braids Diagnostic Test\n";
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz\n";
    std::cout << "Test Samples: " << TEST_SAMPLES << "\n";
    
    testDirectBraids();
    testBraidsEngine();
    testBraidsReference();
    
    return 0;
}