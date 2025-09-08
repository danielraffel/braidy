#pragma once

#include "BraidsReference.h"
#include "AudioComparator.h"
#include "ModeRegistry.h"
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class BraidyVoice;

/**
 * Test configuration parameters for a single test run.
 */
struct TestConfig {
    std::string algorithmName;     // Algorithm name (e.g., "CSAW", "MORPH")
    float frequency;               // Test frequency in Hz
    float timbre;                  // Timbre parameter (0.0 - 1.0)
    float color;                   // Color parameter (0.0 - 1.0)
    float aux;                     // Auxiliary parameter (0.0 - 1.0)
    float duration;                // Test duration in seconds
    uint32_t randomSeed;           // Random seed for deterministic testing
    
    TestConfig() {
        algorithmName = "CSAW";
        frequency = 440.0f;
        timbre = 0.5f;
        color = 0.5f;
        aux = 0.0f;
        duration = 1.0f;
        randomSeed = 42;
    }
};

/**
 * Test result containing configuration, metrics, and generated audio.
 */
struct TestResult {
    TestConfig config;                  // Test configuration used
    ComparisonMetrics metrics;          // Audio comparison results
    std::vector<float> referenceAudio; // Reference audio samples
    std::vector<float> testAudio;      // Test audio samples
    std::string timestamp;             // When the test was run
    bool success;                      // Whether test completed successfully
    std::string errorMessage;          // Error description if failed
    
    TestResult() {
        success = false;
    }
};

/**
 * Batch test configuration for running multiple tests.
 */
struct BatchTestConfig {
    std::vector<std::string> algorithms;  // Algorithms to test (empty = all)
    std::vector<float> frequencies;       // Frequencies to test
    std::vector<float> timbreValues;      // Timbre values to test
    std::vector<float> colorValues;       // Color values to test
    std::vector<float> auxValues;         // Aux values to test
    float duration;                       // Duration per test
    uint32_t baseSeed;                   // Base seed (incremented per test)
    QualityThresholds thresholds;        // Quality thresholds for pass/fail
    
    BatchTestConfig() {
        // Default test frequencies
        frequencies = {110.0f, 220.0f, 440.0f, 880.0f, 1760.0f};
        
        // Default parameter sweeps (fewer values for manageable test count)
        timbreValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        colorValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        auxValues = {0.0f, 0.5f, 1.0f};
        
        duration = 0.5f;  // Shorter duration for batch tests
        baseSeed = 42;
    }
};

/**
 * Main test orchestrator for comparing Braids reference with JUCE wrapper.
 * Supports both single tests and comprehensive batch testing.
 */
class TestRunner {
public:
    /**
     * Constructor initializes the test runner.
     * @param sampleRate Sample rate for testing (default 48kHz)
     * @param blockSize Processing block size (default 24 samples)
     */
    TestRunner(float sampleRate = 48000.0f, int blockSize = 24);
    
    /**
     * Destructor cleans up resources.
     */
    ~TestRunner();
    
    /**
     * Initialize the test runner and its components.
     * @param randomSeed Base random seed for deterministic testing
     * @return True if initialization successful
     */
    bool initialize(uint32_t randomSeed = 42);
    
    /**
     * Run a single comparison test between reference and JUCE implementation.
     * @param config Test configuration
     * @return Test result with metrics and audio data
     */
    TestResult runSingleTest(const TestConfig& config);
    
    /**
     * Generate reference audio only (useful for creating reference files).
     * @param config Test configuration
     * @param outputBuffer Buffer to write audio samples
     * @param numSamples Number of samples to generate
     * @return Number of samples actually generated
     */
    int generateReference(const TestConfig& config, float* outputBuffer, int numSamples);
    
    /**
     * Generate test audio from JUCE wrapper only.
     * @param config Test configuration  
     * @param outputBuffer Buffer to write audio samples
     * @param numSamples Number of samples to generate
     * @return Number of samples actually generated
     */
    int generateTest(const TestConfig& config, float* outputBuffer, int numSamples);
    
    /**
     * Run a comprehensive batch of tests across multiple parameters.
     * @param batchConfig Batch test configuration
     * @return Vector of all test results
     */
    std::vector<TestResult> runBatchTests(const BatchTestConfig& batchConfig);
    
    /**
     * Run tests for all algorithms with default parameters.
     * @return Vector of test results for all algorithms
     */
    std::vector<TestResult> runAllAlgorithmTests();
    
    /**
     * Set quality thresholds for pass/fail determination.
     * @param thresholds Quality thresholds
     */
    void setQualityThresholds(const QualityThresholds& thresholds);
    
    /**
     * Get current quality thresholds.
     * @return Current quality thresholds
     */
    const QualityThresholds& getQualityThresholds() const;
    
    /**
     * Save test results to a comprehensive report file.
     * @param results Test results to save
     * @param filename Output filename
     * @param format Output format ("txt", "csv", or "json")
     * @return True if successful
     */
    bool saveResults(const std::vector<TestResult>& results, 
                     const std::string& filename, 
                     const std::string& format = "txt");
    
    /**
     * Save individual test audio files for manual inspection.
     * @param result Test result containing audio data
     * @param baseFilename Base filename (will add "_ref" and "_test" suffixes)
     * @return True if successful
     */
    bool saveAudioFiles(const TestResult& result, const std::string& baseFilename);
    
    /**
     * Generate a summary report from multiple test results.
     * @param results Test results to summarize
     * @return Summary report string
     */
    std::string generateSummaryReport(const std::vector<TestResult>& results);
    
    /**
     * Get statistics about test results (pass rate, etc.).
     * @param results Test results to analyze
     * @return Summary statistics
     */
    struct TestStatistics {
        int totalTests;
        int passedTests;
        int failedTests;
        double passRate;
        double avgRmsError;
        double avgSnr;
        double avgCorrelation;
        std::string worstAlgorithm;
        std::string bestAlgorithm;
        
        TestStatistics() {
            totalTests = passedTests = failedTests = 0;
            passRate = avgRmsError = avgSnr = avgCorrelation = 0.0;
        }
    };
    
    TestStatistics getTestStatistics(const std::vector<TestResult>& results);

private:
    float sampleRate_;
    int blockSize_;
    uint32_t baseSeed_;
    
    std::unique_ptr<BraidsReference> reference_;
    std::unique_ptr<AudioComparator> comparator_;
    std::unique_ptr<BraidyVoice> braidyVoice_;  // JUCE wrapper instance
    
    // Helper methods
    bool initializeBraidyVoice();
    void configureReference(const TestConfig& config);
    void configureBraidyVoice(const TestConfig& config);
    std::string generateTimestamp();
    std::string formatDuration(double seconds);
    bool writeWavFile(const std::string& filename, const std::vector<float>& audio, float sampleRate);
    
    // Report generation helpers
    std::string generateTextReport(const std::vector<TestResult>& results);
    std::string generateCSVReport(const std::vector<TestResult>& results);
    std::string generateJSONReport(const std::vector<TestResult>& results);
};