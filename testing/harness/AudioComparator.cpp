#include "AudioComparator.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <fstream>
#include <iomanip>

AudioComparator::AudioComparator(const QualityThresholds& thresholds)
    : thresholds_(thresholds) {
}

ComparisonMetrics AudioComparator::compare(const std::vector<float>& reference, 
                                          const std::vector<float>& test, 
                                          float sampleRate) {
    if (reference.empty() || test.empty()) {
        return ComparisonMetrics(); // Return default/invalid metrics
    }
    
    // Ensure both signals are the same length (use shorter)
    size_t minSize = std::min(reference.size(), test.size());
    return compare(reference.data(), test.data(), static_cast<int>(minSize), sampleRate);
}

ComparisonMetrics AudioComparator::compare(const float* reference, 
                                          const float* test, 
                                          int numSamples,
                                          float sampleRate) {
    ComparisonMetrics metrics;
    
    if (!reference || !test || numSamples <= 0) {
        return metrics; // Return default/invalid metrics
    }
    
    // Calculate basic statistics
    metrics.referenceDC = calculateDCOffset(reference, numSamples);
    metrics.testDC = calculateDCOffset(test, numSamples);
    metrics.dcDifference = std::abs(metrics.referenceDC - metrics.testDC);
    
    metrics.referenceRMS = calculateRMS(reference, numSamples);
    metrics.testRMS = calculateRMS(test, numSamples);
    
    // Avoid division by zero in level calculations
    if (metrics.referenceRMS > 1e-10 && metrics.testRMS > 1e-10) {
        metrics.levelDifference = 20.0 * std::log10(metrics.testRMS / metrics.referenceRMS);
    }
    
    // Calculate error metrics
    std::vector<float> errorSignal(numSamples);
    double sumSquaredError = 0.0;
    double sumAbsoluteError = 0.0;
    double maxError = 0.0;
    
    for (int i = 0; i < numSamples; ++i) {
        float error = test[i] - reference[i];
        errorSignal[i] = error;
        
        double absError = std::abs(error);
        sumSquaredError += error * error;
        sumAbsoluteError += absError;
        maxError = std::max(maxError, absError);
    }
    
    metrics.rmsError = std::sqrt(sumSquaredError / numSamples);
    metrics.meanError = sumAbsoluteError / numSamples;
    metrics.peakError = maxError;
    
    // Calculate correlation
    metrics.correlation = calculateCorrelation(reference, test, numSamples);
    
    // Calculate SNR
    if (metrics.rmsError > 1e-10) {
        metrics.snr = 20.0 * std::log10(metrics.referenceRMS / metrics.rmsError);
    } else {
        metrics.snr = 120.0; // Very high SNR for essentially identical signals
    }
    
    // Calculate spectral metrics (if we have enough samples)
    if (numSamples >= 512 && sampleRate > 0) {
        std::vector<float> refVector(reference, reference + numSamples);
        std::vector<float> testVector(test, test + numSamples);
        
        double refCentroid = calculateSpectralCentroid(refVector, sampleRate);
        double testCentroid = calculateSpectralCentroid(testVector, sampleRate);
        metrics.spectralCentroid = testCentroid - refCentroid;
        
        double refRolloff = calculateSpectralRolloff(refVector, sampleRate);
        double testRolloff = calculateSpectralRolloff(testVector, sampleRate);
        metrics.spectralRolloff = testRolloff - refRolloff;
        
        // Calculate THD for the error signal if we can find a fundamental
        double fundamental = findFundamentalFrequency(refVector, sampleRate);
        if (fundamental > 0) {
            metrics.thd = calculateTHD(errorSignal, sampleRate, static_cast<float>(fundamental));
        }
    }
    
    // Calculate SINAD (Signal to Noise and Distortion)
    if (metrics.rmsError > 1e-10) {
        metrics.sinad = 20.0 * std::log10(metrics.referenceRMS / metrics.rmsError);
    } else {
        metrics.sinad = metrics.snr;
    }
    
    // Quality assessment
    metrics.verdict = assessQuality(metrics);
    metrics.isPassing = (metrics.rmsError <= thresholds_.maxRmsError) &&
                       (metrics.peakError <= thresholds_.maxPeakError) &&
                       (metrics.snr >= thresholds_.minSnr) &&
                       (metrics.thd <= thresholds_.maxThd) &&
                       (metrics.dcDifference <= thresholds_.maxDcDifference) &&
                       (std::abs(metrics.levelDifference) <= thresholds_.maxLevelDiff) &&
                       (metrics.correlation >= thresholds_.minCorrelation);
    
    return metrics;
}

void AudioComparator::setThresholds(const QualityThresholds& thresholds) {
    thresholds_ = thresholds;
}

double AudioComparator::calculateRMS(const float* signal, int numSamples) {
    if (!signal || numSamples <= 0) return 0.0;
    
    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        sumSquares += signal[i] * signal[i];
    }
    
    return std::sqrt(sumSquares / numSamples);
}

double AudioComparator::calculateDCOffset(const float* signal, int numSamples) {
    if (!signal || numSamples <= 0) return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        sum += signal[i];
    }
    
    return sum / numSamples;
}

double AudioComparator::calculatePeakValue(const float* signal, int numSamples) {
    if (!signal || numSamples <= 0) return 0.0;
    
    double maxVal = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        maxVal = std::max(maxVal, static_cast<double>(std::abs(signal[i])));
    }
    
    return maxVal;
}

double AudioComparator::calculateCorrelation(const float* sig1, const float* sig2, int numSamples) {
    if (!sig1 || !sig2 || numSamples <= 0) return 0.0;
    
    // Calculate means
    double mean1 = 0.0, mean2 = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        mean1 += sig1[i];
        mean2 += sig2[i];
    }
    mean1 /= numSamples;
    mean2 /= numSamples;
    
    // Calculate correlation coefficient
    double numerator = 0.0, denom1 = 0.0, denom2 = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        double diff1 = sig1[i] - mean1;
        double diff2 = sig2[i] - mean2;
        
        numerator += diff1 * diff2;
        denom1 += diff1 * diff1;
        denom2 += diff2 * diff2;
    }
    
    double denominator = std::sqrt(denom1 * denom2);
    return (denominator > 1e-10) ? (numerator / denominator) : 0.0;
}

double AudioComparator::calculateSNR(const float* signal, const float* noise, int numSamples) {
    double signalRMS = calculateRMS(signal, numSamples);
    double noiseRMS = calculateRMS(noise, numSamples);
    
    if (noiseRMS > 1e-10) {
        return 20.0 * std::log10(signalRMS / noiseRMS);
    }
    
    return 120.0; // Very high SNR for no noise
}

double AudioComparator::calculateTHD(const std::vector<float>& signal, float sampleRate, float fundamental) {
    // Simplified THD calculation - would need proper FFT implementation for accuracy
    // For now, return 0 as placeholder
    return 0.0;
}

double AudioComparator::calculateSpectralCentroid(const std::vector<float>& signal, float sampleRate) {
    // Simplified spectral centroid - would need proper FFT implementation
    // For now, return 0 as placeholder
    return 0.0;
}

double AudioComparator::calculateSpectralRolloff(const std::vector<float>& signal, float sampleRate) {
    // Simplified spectral rolloff - would need proper FFT implementation
    // For now, return 0 as placeholder
    return 0.0;
}

double AudioComparator::findFundamentalFrequency(const std::vector<float>& signal, float sampleRate) {
    // Simplified fundamental detection - would need proper autocorrelation or FFT
    // For now, return 0 as placeholder
    return 0.0;
}

std::string AudioComparator::assessQuality(const ComparisonMetrics& metrics) {
    if (metrics.correlation > 0.99) {
        return "Excellent - Nearly identical signals";
    } else if (metrics.correlation > 0.95 && metrics.snr > 60.0) {
        return "Good - High correlation and SNR";
    } else if (metrics.correlation > 0.90 && metrics.snr > 40.0) {
        return "Fair - Acceptable correlation and SNR";
    } else if (metrics.correlation > 0.80) {
        return "Poor - Low correlation but some similarity";
    } else {
        return "Failed - Signals are significantly different";
    }
}

std::string AudioComparator::generateReport(const ComparisonMetrics& metrics) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    oss << "=== Audio Comparison Report ===" << std::endl;
    oss << "Overall Result: " << (metrics.isPassing ? "PASS" : "FAIL") << std::endl;
    oss << "Quality Assessment: " << metrics.verdict << std::endl << std::endl;
    
    oss << "Error Metrics:" << std::endl;
    oss << "  RMS Error: " << metrics.rmsError << std::endl;
    oss << "  Peak Error: " << metrics.peakError << std::endl;
    oss << "  Mean Error: " << metrics.meanError << std::endl << std::endl;
    
    oss << "Signal Quality:" << std::endl;
    oss << "  SNR: " << std::setprecision(2) << metrics.snr << " dB" << std::endl;
    oss << "  SINAD: " << metrics.sinad << " dB" << std::endl;
    oss << "  Correlation: " << std::setprecision(6) << metrics.correlation << std::endl << std::endl;
    
    oss << "Level Analysis:" << std::endl;
    oss << "  Reference RMS: " << metrics.referenceRMS << std::endl;
    oss << "  Test RMS: " << metrics.testRMS << std::endl;
    oss << "  Level Difference: " << std::setprecision(2) << metrics.levelDifference << " dB" << std::endl << std::endl;
    
    oss << "DC Analysis:" << std::endl;
    oss << "  Reference DC: " << std::setprecision(6) << metrics.referenceDC << std::endl;
    oss << "  Test DC: " << metrics.testDC << std::endl;
    oss << "  DC Difference: " << metrics.dcDifference << std::endl;
    
    return oss.str();
}

std::string AudioComparator::generateSummary(const ComparisonMetrics& metrics) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    
    oss << (metrics.isPassing ? "PASS" : "FAIL") 
        << " | RMS: " << metrics.rmsError 
        << " | Peak: " << metrics.peakError 
        << " | SNR: " << std::setprecision(1) << metrics.snr << "dB"
        << " | Corr: " << std::setprecision(3) << metrics.correlation;
    
    return oss.str();
}

bool AudioComparator::saveToCSV(const ComparisonMetrics& metrics, 
                                const std::string& filename, 
                                bool append) {
    std::ofstream file;
    
    if (append) {
        file.open(filename, std::ios::app);
    } else {
        file.open(filename);
        // Write header if creating new file
        file << "RMS_Error,Peak_Error,Mean_Error,SNR_dB,SINAD_dB,Correlation,"
             << "Reference_RMS,Test_RMS,Level_Diff_dB,Reference_DC,Test_DC,DC_Diff,"
             << "THD,Spectral_Centroid_Diff,Spectral_Rolloff_Diff,Pass,Verdict" << std::endl;
    }
    
    if (!file.is_open()) {
        return false;
    }
    
    file << std::fixed << std::setprecision(6);
    file << metrics.rmsError << ","
         << metrics.peakError << ","
         << metrics.meanError << ","
         << metrics.snr << ","
         << metrics.sinad << ","
         << metrics.correlation << ","
         << metrics.referenceRMS << ","
         << metrics.testRMS << ","
         << metrics.levelDifference << ","
         << metrics.referenceDC << ","
         << metrics.testDC << ","
         << metrics.dcDifference << ","
         << metrics.thd << ","
         << metrics.spectralCentroid << ","
         << metrics.spectralRolloff << ","
         << (metrics.isPassing ? "1" : "0") << ","
         << "\"" << metrics.verdict << "\"" << std::endl;
    
    file.close();
    return true;
}