/**
 * test_wrapper.cpp
 * Test program to validate BraidsEngine wrapper against original Braids
 */

#include "../../Source/adapters/BraidsEngine.h"
#include "BraidsReference.h"
#include "AudioComparator.h"
#include "ModeRegistry.h"
#include <iostream>
#include <vector>
#include <iomanip>

const int SAMPLE_RATE = 48000;
const int TEST_DURATION = SAMPLE_RATE; // 1 second
const int BLOCK_SIZE = 512; // JUCE typical block size

struct TestResult {
    std::string algorithm;
    float rmsError;
    float peakError;
    float snr;
    bool passed;
};

TestResult testAlgorithm(const std::string& algorithmName) {
    TestResult result;
    result.algorithm = algorithmName;
    
    // Get shape from registry
    int shape = ModeRegistry::getShapeForName(algorithmName);
    if (shape == -1) {
        std::cerr << "Unknown algorithm: " << algorithmName << std::endl;
        result.passed = false;
        return result;
    }
    
    // Create reference and wrapper instances
    BraidsReference reference(SAMPLE_RATE, BLOCK_SIZE);
    
    BraidyAdapter::BraidsEngine wrapper;
    wrapper.initialize(SAMPLE_RATE);
    
    // Set same parameters
    int algorithmId = static_cast<int>(shape);
    reference.setShape(shape);
    wrapper.setAlgorithm(algorithmId);
    
    // Set middle C
    float frequency = 261.63f; // Middle C
    reference.setFrequency(frequency);
    wrapper.setPitch(60.0f); // MIDI note 60
    
    // Set default parameters
    reference.setTimbre(0.5f);
    reference.setColor(0.5f);
    wrapper.setParameters(0.5f, 0.5f);
    
    // Trigger for percussion algorithms
    if (algorithmId >= 32 && algorithmId <= 38) {
        // BraidsReference doesn't have strike, Braids handles it internally
        wrapper.strike();
    }
    
    // Generate audio
    std::vector<float> referenceAudio(TEST_DURATION);
    std::vector<float> wrapperAudio(TEST_DURATION);
    
    // Process in blocks
    int refIdx = 0;
    int wrapIdx = 0;
    
    while (refIdx < TEST_DURATION && wrapIdx < TEST_DURATION) {
        // Generate reference (24-sample blocks)
        int refSamples = reference.generateBlock(&referenceAudio[refIdx]);
        refIdx += refSamples;
        
        // Generate wrapper output (variable blocks)
        int samplesToProcess = std::min(BLOCK_SIZE, TEST_DURATION - wrapIdx);
        wrapper.processAudio(&wrapperAudio[wrapIdx], samplesToProcess);
        wrapIdx += samplesToProcess;
    }
    
    // Compare audio
    AudioComparator comparator;
    // Use relaxed thresholds for initial testing
    QualityThresholds relaxedThresholds;
    relaxedThresholds.maxRmsError = 0.1;      // 10% RMS error
    relaxedThresholds.maxPeakError = 0.3;     // 30% peak error
    relaxedThresholds.minSnr = 20.0;          // 20 dB SNR
    relaxedThresholds.maxThd = 10.0;          // 10% THD
    comparator.setThresholds(relaxedThresholds);
    auto metrics = comparator.compare(
        referenceAudio.data(), 
        wrapperAudio.data(),
        TEST_DURATION
    );
    
    result.rmsError = metrics.rmsError;
    result.peakError = metrics.peakError;
    result.snr = metrics.snr;
    result.passed = metrics.isPassing;
    
    return result;
}

void printHeader() {
    std::cout << "\n";
    std::cout << "====================================================================\n";
    std::cout << "                    BRAIDS WRAPPER VALIDATION TEST                  \n";
    std::cout << "====================================================================\n";
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz\n";
    std::cout << "Test Duration: " << TEST_DURATION << " samples (" 
              << (float)TEST_DURATION/SAMPLE_RATE << " seconds)\n";
    std::cout << "Block Size: " << BLOCK_SIZE << " samples\n";
    std::cout << "\n";
}

void printResultsTable(const std::vector<TestResult>& results) {
    std::cout << "\n";
    std::cout << std::left << std::setw(25) << "Algorithm" 
              << std::right << std::setw(12) << "RMS Error" 
              << std::setw(12) << "Peak Error"
              << std::setw(12) << "SNR (dB)"
              << std::setw(10) << "Status" << "\n";
    std::cout << std::string(71, '-') << "\n";
    
    int passed = 0;
    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.algorithm
                  << std::right << std::setw(12) << std::fixed << std::setprecision(6) << result.rmsError
                  << std::setw(12) << std::fixed << std::setprecision(6) << result.peakError
                  << std::setw(12) << std::fixed << std::setprecision(1) << result.snr
                  << std::setw(10) << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n";
        
        if (result.passed) passed++;
    }
    
    std::cout << std::string(71, '-') << "\n";
    std::cout << "Summary: " << passed << "/" << results.size() 
              << " algorithms passed (" 
              << std::fixed << std::setprecision(1) 
              << (100.0f * passed / results.size()) << "%)\n";
}

int main(int argc, char* argv[]) {
    printHeader();
    
    // Test critical algorithms from each category
    std::vector<std::string> criticalAlgorithms = {
        "CSAW",           // Analog
        "MORPH",          // Morphing
        "TOY",            // Digital
        "FM",             // FM synthesis
        "WAVETABLES",     // Wavetable
        "VOWEL",          // Speech
        "KICK",           // Drums
        "PLUCKED",        // Physical modeling
        "FILTERED_NOISE", // Noise
        "GRANULAR_CLOUD"  // Granular
    };
    
    std::cout << "Testing " << criticalAlgorithms.size() << " critical algorithms...\n";
    std::cout << "\n";
    
    std::vector<TestResult> results;
    
    for (const auto& algo : criticalAlgorithms) {
        std::cout << "Testing " << algo << "..." << std::flush;
        TestResult result = testAlgorithm(algo);
        results.push_back(result);
        std::cout << " " << (result.passed ? "✅" : "❌") << "\n";
    }
    
    printResultsTable(results);
    
    // Determine overall success
    int passCount = 0;
    for (const auto& result : results) {
        if (result.passed) passCount++;
    }
    
    float passRate = 100.0f * passCount / results.size();
    
    std::cout << "\n";
    if (passRate >= 90.0f) {
        std::cout << "✅ VALIDATION SUCCESSFUL! Wrapper is working correctly.\n";
        return 0;
    } else if (passRate >= 70.0f) {
        std::cout << "⚠️  PARTIAL SUCCESS. Some algorithms need attention.\n";
        return 1;
    } else {
        std::cout << "❌ VALIDATION FAILED. Major issues detected.\n";
        return 2;
    }
}