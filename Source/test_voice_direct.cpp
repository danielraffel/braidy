// Direct test of BraidyVoice to verify audio generation
#include <iostream>
#include <vector>
#include <cmath>
#include "BraidyVoice/BraidyVoice.h"
#include "BraidyCore/BraidySettings.h"
#include "BraidyCore/BraidyResources.h"

using namespace braidy;

class VoiceTest {
private:
    BraidyVoice voice;
    BraidySettings settings;
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr int BLOCK_SIZE = 512;
    
public:
    VoiceTest() {
        InitializeResources();
        voice.Init();
        voice.SetSampleRate(SAMPLE_RATE);
        settings.Init();
        
        // Set CSAW algorithm
        settings.SetParameter(BraidyParameter::ALGORITHM, 0);  // CSAW
        settings.SetParameter(BraidyParameter::TIMBRE, 0.5f);
        settings.SetParameter(BraidyParameter::COLOR, 0.5f);
        settings.SetParameter(BraidyParameter::ATTACK, 0.01f);  // Fast attack
        settings.SetParameter(BraidyParameter::DECAY, 0.5f);
        settings.SetParameter(BraidyParameter::VOLUME, 1.0f);
    }
    
    void testNote(int midiNote) {
        std::cout << "\n=== Testing MIDI Note " << midiNote << " ===" << std::endl;
        
        // Update voice from settings
        voice.UpdateFromSettings(settings);
        
        // Trigger note
        voice.NoteOn(midiNote, 1.0f);  // Full velocity
        
        // Generate 1 second of audio
        int numBlocks = (SAMPLE_RATE / BLOCK_SIZE);
        std::vector<float> audioBuffer;
        
        for (int block = 0; block < numBlocks; ++block) {
            float blockBuffer[BLOCK_SIZE];
            voice.Process(blockBuffer, BLOCK_SIZE);
            
            // Store samples
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                audioBuffer.push_back(blockBuffer[i]);
            }
            
            // Show some samples from first block
            if (block == 0) {
                std::cout << "First 10 samples: ";
                for (int i = 0; i < 10; ++i) {
                    std::cout << blockBuffer[i] << " ";
                }
                std::cout << std::endl;
            }
        }
        
        // Analyze output
        float sum = 0, maxVal = 0, minVal = 0;
        int zeroCrossings = 0;
        
        for (size_t i = 0; i < audioBuffer.size(); ++i) {
            float sample = audioBuffer[i];
            sum += sample * sample;
            if (sample > maxVal) maxVal = sample;
            if (sample < minVal) minVal = sample;
            
            if (i > 0) {
                if ((audioBuffer[i-1] < 0 && audioBuffer[i] >= 0) ||
                    (audioBuffer[i-1] >= 0 && audioBuffer[i] < 0)) {
                    zeroCrossings++;
                }
            }
        }
        
        float rms = std::sqrt(sum / audioBuffer.size());
        float frequency = (zeroCrossings / 2.0f) / (audioBuffer.size() / SAMPLE_RATE);
        float expectedFreq = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
        
        std::cout << "RMS Level: " << rms << std::endl;
        std::cout << "Peak values: [" << minVal << ", " << maxVal << "]" << std::endl;
        std::cout << "Zero crossings: " << zeroCrossings << std::endl;
        std::cout << "Estimated frequency: " << frequency << " Hz" << std::endl;
        std::cout << "Expected frequency: " << expectedFreq << " Hz" << std::endl;
        std::cout << "Voice active: " << (voice.IsActive() ? "YES" : "NO") << std::endl;
        
        // Release note
        voice.NoteOff(midiNote);
    }
    
    void runTest() {
        std::cout << "\n=== BRAIDY VOICE DIRECT TEST ===" << std::endl;
        std::cout << "Testing voice rendering with CSAW algorithm" << std::endl;
        
        // Test a few notes
        testNote(48);  // C3
        testNote(60);  // C4
        testNote(69);  // A4
    }
};

int main() {
    VoiceTest test;
    test.runTest();
    return 0;
}