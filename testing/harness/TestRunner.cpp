#include "TestRunner.h"
#include "../../Source/BraidyVoice/BraidyVoice.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cmath>

TestRunner::TestRunner(float sampleRate, int blockSize)
    : sampleRate_(sampleRate)
    , blockSize_(blockSize)
    , baseSeed_(42) {
    
    // Initialize components
    reference_ = std::make_unique<BraidsReference>(sampleRate, blockSize);
    comparator_ = std::make_unique<AudioComparator>();
    braidyVoice_ = nullptr; // Will be initialized in initialize()
}

TestRunner::~TestRunner() {
}

bool TestRunner::initialize(uint32_t randomSeed) {
    baseSeed_ = randomSeed;
    
    // Initialize the Braids reference
    if (!reference_) {
        std::cerr << "Failed to create BraidsReference instance" << std::endl;
        return false;
    }
    
    reference_->initialize(randomSeed);
    
    // Initialize the JUCE wrapper
    if (!initializeBraidyVoice()) {
        std::cerr << "Failed to initialize BraidyVoice" << std::endl;
        return false;
    }
    
    // Initialize the mode registry
    ModeRegistry::initialize();
    
    return true;
}

bool TestRunner::initializeBraidyVoice() {
    try {
        // Create BraidyVoice instance
        braidyVoice_ = std::make_unique<BraidyVoice>();
        
        // Initialize with our sample rate
        braidyVoice_->setSampleRate(static_cast<double>(sampleRate_));
        braidyVoice_->reset();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception initializing BraidyVoice: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown exception initializing BraidyVoice" << std::endl;
        return false;
    }
}

TestResult TestRunner::runSingleTest(const TestConfig& config) {
    TestResult result;
    result.config = config;
    result.timestamp = generateTimestamp();
    
    try {
        // Calculate number of samples needed
        int numSamples = static_cast<int>(config.duration * sampleRate_);
        
        // Allocate buffers
        result.referenceAudio.resize(numSamples);
        result.testAudio.resize(numSamples);
        
        // Generate reference audio
        int refSamples = generateReference(config, result.referenceAudio.data(), numSamples);
        if (refSamples != numSamples) {
            result.success = false;
            result.errorMessage = "Failed to generate complete reference audio";
            return result;
        }
        
        // Generate test audio
        int testSamples = generateTest(config, result.testAudio.data(), numSamples);
        if (testSamples != numSamples) {
            result.success = false;
            result.errorMessage = "Failed to generate complete test audio";
            return result;
        }
        
        // Compare the audio
        result.metrics = comparator_->compare(result.referenceAudio, result.testAudio, sampleRate_);
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Exception during test: ") + e.what();
    } catch (...) {
        result.success = false;
        result.errorMessage = "Unknown exception during test";
    }
    
    return result;
}

int TestRunner::generateReference(const TestConfig& config, float* outputBuffer, int numSamples) {
    if (!reference_ || !outputBuffer || numSamples <= 0) {
        return 0;
    }
    
    try {
        // Configure reference oscillator
        configureReference(config);
        
        // Generate audio
        return reference_->generateSamples(outputBuffer, numSamples);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception generating reference: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "Unknown exception generating reference" << std::endl;
        return 0;
    }
}

int TestRunner::generateTest(const TestConfig& config, float* outputBuffer, int numSamples) {
    if (!braidyVoice_ || !outputBuffer || numSamples <= 0) {
        return 0;
    }
    
    try {
        // Configure JUCE wrapper
        configureBraidyVoice(config);
        
        // Generate audio by processing blocks
        int totalGenerated = 0;
        float* writePtr = outputBuffer;
        
        while (totalGenerated < numSamples) {
            int samplesToProcess = std::min(blockSize_, numSamples - totalGenerated);
            
            // Process a block
            braidyVoice_->renderNextBlock(writePtr, samplesToProcess);
            
            writePtr += samplesToProcess;
            totalGenerated += samplesToProcess;
        }
        
        return totalGenerated;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception generating test audio: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "Unknown exception generating test audio" << std::endl;
        return 0;
    }
}

void TestRunner::configureReference(const TestConfig& config) {
    if (!reference_) return;
    
    // Reset and configure
    reference_->reset();
    
    // Set algorithm
    int shape = ModeRegistry::getShapeForName(config.algorithmName);
    if (shape >= 0) {
        reference_->setShape(shape);
    }
    
    // Set parameters
    reference_->setFrequency(config.frequency);
    reference_->setTimbre(config.timbre);
    reference_->setColor(config.color);
    reference_->setAux(config.aux);
    reference_->setLevel(1.0f);
    reference_->setFilterEnabled(false); // Match JUCE wrapper default
}

void TestRunner::configureBraidyVoice(const TestConfig& config) {
    if (!braidyVoice_) return;
    
    // Reset voice
    braidyVoice_->reset();
    
    // Set algorithm
    braidyVoice_->setAlgorithm(config.algorithmName);
    
    // Set parameters
    braidyVoice_->setFrequency(config.frequency);
    braidyVoice_->setTimbre(config.timbre);
    braidyVoice_->setColor(config.color);
    braidyVoice_->setAux(config.aux);
    
    // Start the voice
    braidyVoice_->startNote();
}

std::vector<TestResult> TestRunner::runBatchTests(const BatchTestConfig& batchConfig) {
    std::vector<TestResult> results;
    
    // Set quality thresholds
    comparator_->setThresholds(batchConfig.thresholds);
    
    // Determine algorithms to test
    std::vector<std::string> algorithms = batchConfig.algorithms;
    if (algorithms.empty()) {
        algorithms = ModeRegistry::getAllNames();
    }
    
    uint32_t currentSeed = batchConfig.baseSeed;
    
    std::cout << "Running batch tests..." << std::endl;
    std::cout << "Algorithms: " << algorithms.size() << std::endl;
    std::cout << "Frequencies: " << batchConfig.frequencies.size() << std::endl;
    std::cout << "Timbre values: " << batchConfig.timbreValues.size() << std::endl;
    std::cout << "Color values: " << batchConfig.colorValues.size() << std::endl;
    std::cout << "Aux values: " << batchConfig.auxValues.size() << std::endl;
    
    int totalTests = algorithms.size() * batchConfig.frequencies.size() * 
                    batchConfig.timbreValues.size() * batchConfig.colorValues.size() * 
                    batchConfig.auxValues.size();
    std::cout << "Total tests: " << totalTests << std::endl;
    
    int testCount = 0;
    
    // Test each combination
    for (const auto& algorithm : algorithms) {
        for (float freq : batchConfig.frequencies) {
            for (float timbre : batchConfig.timbreValues) {
                for (float color : batchConfig.colorValues) {
                    for (float aux : batchConfig.auxValues) {
                        TestConfig config;
                        config.algorithmName = algorithm;
                        config.frequency = freq;
                        config.timbre = timbre;
                        config.color = color;
                        config.aux = aux;
                        config.duration = batchConfig.duration;
                        config.randomSeed = currentSeed++;
                        
                        TestResult result = runSingleTest(config);
                        results.push_back(result);
                        
                        testCount++;
                        if (testCount % 10 == 0 || testCount == totalTests) {
                            std::cout << "Progress: " << testCount << "/" << totalTests 
                                     << " (" << (100 * testCount / totalTests) << "%)" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    return results;
}

std::vector<TestResult> TestRunner::runAllAlgorithmTests() {
    BatchTestConfig batchConfig;
    
    // Use simplified parameters for quick algorithm coverage
    batchConfig.frequencies = {440.0f}; // Single frequency
    batchConfig.timbreValues = {0.5f};  // Single timbre
    batchConfig.colorValues = {0.5f};   // Single color  
    batchConfig.auxValues = {0.0f};     // Single aux
    batchConfig.duration = 0.25f;       // Short duration
    
    return runBatchTests(batchConfig);
}

void TestRunner::setQualityThresholds(const QualityThresholds& thresholds) {
    comparator_->setThresholds(thresholds);
}

const QualityThresholds& TestRunner::getQualityThresholds() const {
    return comparator_->getThresholds();
}

std::string TestRunner::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string TestRunner::generateSummaryReport(const std::vector<TestResult>& results) {
    if (results.empty()) {
        return "No test results to summarize.";
    }
    
    TestStatistics stats = getTestStatistics(results);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "=== Test Summary Report ===" << std::endl;
    oss << "Generated: " << generateTimestamp() << std::endl << std::endl;
    
    oss << "Overall Statistics:" << std::endl;
    oss << "  Total Tests: " << stats.totalTests << std::endl;
    oss << "  Passed: " << stats.passedTests << std::endl;
    oss << "  Failed: " << stats.failedTests << std::endl;
    oss << "  Pass Rate: " << stats.passRate << "%" << std::endl << std::endl;
    
    oss << "Average Metrics:" << std::endl;
    oss << "  RMS Error: " << std::setprecision(6) << stats.avgRmsError << std::endl;
    oss << "  SNR: " << std::setprecision(1) << stats.avgSnr << " dB" << std::endl;
    oss << "  Correlation: " << std::setprecision(4) << stats.avgCorrelation << std::endl << std::endl;
    
    if (!stats.bestAlgorithm.empty() && !stats.worstAlgorithm.empty()) {
        oss << "Algorithm Performance:" << std::endl;
        oss << "  Best: " << stats.bestAlgorithm << std::endl;
        oss << "  Worst: " << stats.worstAlgorithm << std::endl << std::endl;
    }
    
    // Show failing tests
    oss << "Failed Tests:" << std::endl;
    for (const auto& result : results) {
        if (!result.metrics.isPassing) {
            oss << "  " << result.config.algorithmName 
                << " @" << result.config.frequency << "Hz: "
                << AudioComparator::generateSummary(result.metrics) << std::endl;
        }
    }
    
    return oss.str();
}

TestRunner::TestStatistics TestRunner::getTestStatistics(const std::vector<TestResult>& results) {
    TestStatistics stats;
    
    if (results.empty()) {
        return stats;
    }
    
    stats.totalTests = static_cast<int>(results.size());
    
    double totalRmsError = 0.0;
    double totalSnr = 0.0;
    double totalCorrelation = 0.0;
    
    std::map<std::string, std::pair<int, double>> algorithmScores; // name -> (count, total_correlation)
    
    for (const auto& result : results) {
        if (result.success) {
            if (result.metrics.isPassing) {
                stats.passedTests++;
            } else {
                stats.failedTests++;
            }
            
            totalRmsError += result.metrics.rmsError;
            totalSnr += result.metrics.snr;
            totalCorrelation += result.metrics.correlation;
            
            // Track algorithm performance
            auto& algorithmScore = algorithmScores[result.config.algorithmName];
            algorithmScore.first++;
            algorithmScore.second += result.metrics.correlation;
        }
    }
    
    stats.passRate = (stats.totalTests > 0) ? (100.0 * stats.passedTests / stats.totalTests) : 0.0;
    stats.avgRmsError = totalRmsError / stats.totalTests;
    stats.avgSnr = totalSnr / stats.totalTests;
    stats.avgCorrelation = totalCorrelation / stats.totalTests;
    
    // Find best and worst algorithms
    double bestScore = -1.0;
    double worstScore = 2.0;
    
    for (const auto& pair : algorithmScores) {
        double avgScore = pair.second.second / pair.second.first;
        if (avgScore > bestScore) {
            bestScore = avgScore;
            stats.bestAlgorithm = pair.first;
        }
        if (avgScore < worstScore) {
            worstScore = avgScore;
            stats.worstAlgorithm = pair.first;
        }
    }
    
    return stats;
}

bool TestRunner::saveResults(const std::vector<TestResult>& results, 
                            const std::string& filename, 
                            const std::string& format) {
    try {
        std::string content;
        
        if (format == "csv") {
            content = generateCSVReport(results);
        } else if (format == "json") {
            content = generateJSONReport(results);
        } else {
            content = generateTextReport(results);
        }
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception saving results: " << e.what() << std::endl;
        return false;
    }
}

std::string TestRunner::generateTextReport(const std::vector<TestResult>& results) {
    std::ostringstream oss;
    
    oss << generateSummaryReport(results) << std::endl;
    
    oss << "=== Detailed Results ===" << std::endl;
    for (const auto& result : results) {
        oss << std::endl << "Test: " << result.config.algorithmName 
            << " @" << result.config.frequency << "Hz" << std::endl;
        oss << "Parameters: T=" << result.config.timbre 
            << ", C=" << result.config.color 
            << ", A=" << result.config.aux << std::endl;
        oss << AudioComparator::generateReport(result.metrics) << std::endl;
    }
    
    return oss.str();
}

std::string TestRunner::generateCSVReport(const std::vector<TestResult>& results) {
    std::ostringstream oss;
    
    // CSV header
    oss << "Algorithm,Frequency,Timbre,Color,Aux,Duration,Success,"
        << "RMS_Error,Peak_Error,SNR_dB,Correlation,Reference_RMS,Test_RMS,"
        << "Level_Diff_dB,DC_Difference,Pass,Verdict,Timestamp" << std::endl;
    
    // CSV data
    for (const auto& result : results) {
        oss << result.config.algorithmName << ","
            << result.config.frequency << ","
            << result.config.timbre << ","
            << result.config.color << ","
            << result.config.aux << ","
            << result.config.duration << ","
            << (result.success ? "1" : "0") << ","
            << std::fixed << std::setprecision(6)
            << result.metrics.rmsError << ","
            << result.metrics.peakError << ","
            << result.metrics.snr << ","
            << result.metrics.correlation << ","
            << result.metrics.referenceRMS << ","
            << result.metrics.testRMS << ","
            << result.metrics.levelDifference << ","
            << result.metrics.dcDifference << ","
            << (result.metrics.isPassing ? "1" : "0") << ","
            << "\"" << result.metrics.verdict << "\","
            << "\"" << result.timestamp << "\"" << std::endl;
    }
    
    return oss.str();
}

std::string TestRunner::generateJSONReport(const std::vector<TestResult>& results) {
    std::ostringstream oss;
    
    oss << "{\n  \"testResults\": [\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        oss << "    {\n";
        oss << "      \"config\": {\n";
        oss << "        \"algorithm\": \"" << result.config.algorithmName << "\",\n";
        oss << "        \"frequency\": " << result.config.frequency << ",\n";
        oss << "        \"timbre\": " << result.config.timbre << ",\n";
        oss << "        \"color\": " << result.config.color << ",\n";
        oss << "        \"aux\": " << result.config.aux << ",\n";
        oss << "        \"duration\": " << result.config.duration << "\n";
        oss << "      },\n";
        oss << "      \"metrics\": {\n";
        oss << std::fixed << std::setprecision(6);
        oss << "        \"rmsError\": " << result.metrics.rmsError << ",\n";
        oss << "        \"peakError\": " << result.metrics.peakError << ",\n";
        oss << "        \"snr\": " << result.metrics.snr << ",\n";
        oss << "        \"correlation\": " << result.metrics.correlation << ",\n";
        oss << "        \"isPassing\": " << (result.metrics.isPassing ? "true" : "false") << "\n";
        oss << "      },\n";
        oss << "      \"success\": " << (result.success ? "true" : "false") << ",\n";
        oss << "      \"timestamp\": \"" << result.timestamp << "\"\n";
        oss << "    }";
        
        if (i < results.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }
    
    oss << "  ]\n}\n";
    
    return oss.str();
}