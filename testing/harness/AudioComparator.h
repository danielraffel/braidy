#pragma once

#include <vector>
#include <string>

/**
 * Audio comparison metrics and analysis tools.
 * Provides comprehensive audio quality metrics for comparing reference and test signals.
 */
struct ComparisonMetrics {
    // Error metrics
    double rmsError;           // Root Mean Square error
    double peakError;          // Maximum absolute difference
    double meanError;          // Average absolute difference
    
    // Signal quality metrics
    double snr;                // Signal-to-Noise Ratio in dB
    double thd;                // Total Harmonic Distortion as percentage
    double sinad;              // Signal-to-Noise and Distortion ratio in dB
    
    // DC and bias metrics
    double referenceDC;        // DC offset of reference signal
    double testDC;             // DC offset of test signal
    double dcDifference;       // Difference in DC offset
    
    // Level metrics
    double referenceRMS;       // RMS level of reference signal
    double testRMS;            // RMS level of test signal
    double levelDifference;    // Level difference in dB
    
    // Correlation metrics
    double correlation;        // Cross-correlation coefficient (-1 to 1)
    double coherence;          // Average coherence (0 to 1)
    
    // Frequency domain metrics
    double spectralCentroid;   // Difference in spectral centroid
    double spectralRolloff;    // Difference in spectral rolloff point
    
    // Quality assessment
    bool isPassing;            // Overall pass/fail based on thresholds
    std::string verdict;       // Human-readable assessment
    
    ComparisonMetrics() {
        rmsError = peakError = meanError = 0.0;
        snr = thd = sinad = 0.0;
        referenceDC = testDC = dcDifference = 0.0;
        referenceRMS = testRMS = levelDifference = 0.0;
        correlation = coherence = 0.0;
        spectralCentroid = spectralRolloff = 0.0;
        isPassing = false;
        verdict = "Not analyzed";
    }
};

/**
 * Quality thresholds for pass/fail determination.
 */
struct QualityThresholds {
    double maxRmsError;        // Maximum acceptable RMS error
    double maxPeakError;       // Maximum acceptable peak error
    double minSnr;             // Minimum acceptable SNR in dB
    double maxThd;             // Maximum acceptable THD percentage
    double maxDcDifference;    // Maximum acceptable DC difference
    double maxLevelDiff;       // Maximum acceptable level difference in dB
    double minCorrelation;     // Minimum acceptable correlation
    
    QualityThresholds() {
        maxRmsError = 0.01;      // 1% RMS error
        maxPeakError = 0.1;      // 10% peak error
        minSnr = 60.0;           // 60 dB SNR
        maxThd = 1.0;            // 1% THD
        maxDcDifference = 0.001; // 0.1% DC difference
        maxLevelDiff = 0.5;      // 0.5 dB level difference
        minCorrelation = 0.95;   // 95% correlation
    }
};

class AudioComparator {
public:
    /**
     * Constructor with optional custom thresholds.
     * @param thresholds Quality thresholds for pass/fail assessment
     */
    AudioComparator(const QualityThresholds& thresholds = QualityThresholds());
    
    /**
     * Compare two audio signals and compute comprehensive metrics.
     * @param reference Reference audio signal
     * @param test Test audio signal
     * @param sampleRate Sample rate of the signals
     * @return Comprehensive comparison metrics
     */
    ComparisonMetrics compare(const std::vector<float>& reference, 
                             const std::vector<float>& test, 
                             float sampleRate = 48000.0f);
    
    /**
     * Compare two audio buffers (raw arrays).
     * @param reference Reference audio buffer
     * @param test Test audio buffer
     * @param numSamples Number of samples in each buffer
     * @param sampleRate Sample rate of the signals
     * @return Comprehensive comparison metrics
     */
    ComparisonMetrics compare(const float* reference, 
                             const float* test, 
                             int numSamples,
                             float sampleRate = 48000.0f);
    
    /**
     * Set custom quality thresholds.
     * @param thresholds New quality thresholds
     */
    void setThresholds(const QualityThresholds& thresholds);
    
    /**
     * Get current quality thresholds.
     * @return Current quality thresholds
     */
    const QualityThresholds& getThresholds() const { return thresholds_; }
    
    /**
     * Generate a detailed report string from metrics.
     * @param metrics Comparison metrics to format
     * @return Formatted report string
     */
    static std::string generateReport(const ComparisonMetrics& metrics);
    
    /**
     * Generate a concise summary string from metrics.
     * @param metrics Comparison metrics to summarize
     * @return Concise summary string
     */
    static std::string generateSummary(const ComparisonMetrics& metrics);
    
    /**
     * Save comparison results to a CSV file for analysis.
     * @param metrics Metrics to save
     * @param filename Output filename
     * @param append True to append to existing file
     * @return True if successful
     */
    static bool saveToCSV(const ComparisonMetrics& metrics, 
                          const std::string& filename, 
                          bool append = false);

private:
    QualityThresholds thresholds_;
    
    // Internal calculation methods
    double calculateRMS(const float* signal, int numSamples);
    double calculateDCOffset(const float* signal, int numSamples);
    double calculatePeakValue(const float* signal, int numSamples);
    double calculateCorrelation(const float* sig1, const float* sig2, int numSamples);
    double calculateSNR(const float* signal, const float* noise, int numSamples);
    double calculateTHD(const std::vector<float>& signal, float sampleRate, float fundamental);
    double calculateSpectralCentroid(const std::vector<float>& signal, float sampleRate);
    double calculateSpectralRolloff(const std::vector<float>& signal, float sampleRate);
    
    // Helper methods
    void performFFT(const std::vector<float>& input, 
                   std::vector<float>& magnitude, 
                   std::vector<float>& phase);
    double findFundamentalFrequency(const std::vector<float>& signal, float sampleRate);
    std::string assessQuality(const ComparisonMetrics& metrics);
    
    // Windowing function for spectral analysis
    std::vector<float> applyHanningWindow(const std::vector<float>& signal);
};