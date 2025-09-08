/**
 * Example usage of the Braids test harness.
 * 
 * This demonstrates how to use the test harness to compare
 * the original Braids implementation with the JUCE wrapper.
 */

#include "TestRunner.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Braids Test Harness Example" << std::endl;
    std::cout << "===========================" << std::endl << std::endl;
    
    // Create test runner with standard Braids settings
    auto testRunner = std::make_unique<TestRunner>(48000.0f, 24);
    
    // Initialize with deterministic seed
    if (!testRunner->initialize(42)) {
        std::cerr << "Failed to initialize test runner!" << std::endl;
        return 1;
    }
    
    std::cout << "Test runner initialized successfully." << std::endl << std::endl;
    
    // Example 1: Single algorithm test
    std::cout << "=== Single Algorithm Test ===" << std::endl;
    {
        TestConfig config;
        config.algorithmName = "CSAW";
        config.frequency = 440.0f;
        config.timbre = 0.5f;
        config.color = 0.3f;
        config.aux = 0.0f;
        config.duration = 1.0f;
        config.randomSeed = 42;
        
        TestResult result = testRunner->runSingleTest(config);
        
        if (result.success) {
            std::cout << "Test completed successfully!" << std::endl;
            std::cout << AudioComparator::generateSummary(result.metrics) << std::endl;
            std::cout << std::endl << AudioComparator::generateReport(result.metrics) << std::endl;
        } else {
            std::cout << "Test failed: " << result.errorMessage << std::endl;
        }
    }
    
    // Example 2: Test all algorithms with default parameters
    std::cout << std::endl << "=== All Algorithms Test ===" << std::endl;
    {
        std::vector<TestResult> results = testRunner->runAllAlgorithmTests();
        
        if (!results.empty()) {
            std::cout << "Tested " << results.size() << " algorithms." << std::endl;
            
            // Show summary
            std::cout << testRunner->generateSummaryReport(results) << std::endl;
            
            // Save results
            if (testRunner->saveResults(results, "all_algorithms_test.txt")) {
                std::cout << "Results saved to all_algorithms_test.txt" << std::endl;
            }
            
            if (testRunner->saveResults(results, "all_algorithms_test.csv", "csv")) {
                std::cout << "Results saved to all_algorithms_test.csv" << std::endl;
            }
        } else {
            std::cout << "No results returned from algorithm tests!" << std::endl;
        }
    }
    
    // Example 3: Custom batch test
    std::cout << std::endl << "=== Custom Batch Test ===" << std::endl;
    {
        BatchTestConfig batchConfig;
        
        // Test specific algorithms
        batchConfig.algorithms = {"CSAW", "MORPH", "SAW_SQUARE", "BUZZ"};
        
        // Test a few frequencies
        batchConfig.frequencies = {220.0f, 440.0f, 880.0f};
        
        // Test timbre sweep
        batchConfig.timbreValues = {0.0f, 0.5f, 1.0f};
        
        // Keep other parameters simple
        batchConfig.colorValues = {0.5f};
        batchConfig.auxValues = {0.0f};
        
        // Short duration for faster testing
        batchConfig.duration = 0.5f;
        
        // Set quality thresholds
        batchConfig.thresholds.maxRmsError = 0.01;
        batchConfig.thresholds.minSnr = 60.0;
        batchConfig.thresholds.minCorrelation = 0.95;
        
        std::vector<TestResult> results = testRunner->runBatchTests(batchConfig);
        
        if (!results.empty()) {
            std::cout << "Batch test completed with " << results.size() << " tests." << std::endl;
            
            // Get statistics
            auto stats = testRunner->getTestStatistics(results);
            std::cout << "Pass rate: " << stats.passRate << "%" << std::endl;
            std::cout << "Average SNR: " << stats.avgSnr << " dB" << std::endl;
            std::cout << "Average correlation: " << stats.avgCorrelation << std::endl;
            
            // Save batch results
            if (testRunner->saveResults(results, "batch_test_results.csv", "csv")) {
                std::cout << "Batch results saved to batch_test_results.csv" << std::endl;
            }
        }
    }
    
    // Example 4: Generate reference audio only
    std::cout << std::endl << "=== Reference Audio Generation ===" << std::endl;
    {
        TestConfig config;
        config.algorithmName = "MORPH";
        config.frequency = 440.0f;
        config.timbre = 0.7f;
        config.color = 0.4f;
        config.aux = 0.2f;
        config.duration = 2.0f;
        
        int numSamples = static_cast<int>(config.duration * 48000.0f);
        std::vector<float> referenceAudio(numSamples);
        
        int generated = testRunner->generateReference(config, referenceAudio.data(), numSamples);
        
        if (generated == numSamples) {
            std::cout << "Generated " << generated << " samples of reference audio." << std::endl;
            
            // Calculate some basic stats
            double rms = 0.0;
            double peak = 0.0;
            for (float sample : referenceAudio) {
                rms += sample * sample;
                peak = std::max(peak, static_cast<double>(std::abs(sample)));
            }
            rms = std::sqrt(rms / referenceAudio.size());
            
            std::cout << "RMS level: " << rms << std::endl;
            std::cout << "Peak level: " << peak << std::endl;
        } else {
            std::cout << "Failed to generate complete reference audio." << std::endl;
        }
    }
    
    std::cout << std::endl << "Test harness example completed!" << std::endl;
    
    return 0;
}