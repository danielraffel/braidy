#pragma once

#include "BraidyTypes.h"
#include <cmath>

namespace braidy {

/**
 * BitCrusher - Bit depth and sample rate reduction effect
 * Provides lo-fi digital distortion effects
 */
class BitCrusher
{
public:
    BitCrusher() : enabled_(false), bitDepth_(16), sampleRateReduction_(1),
                   sampleCounter_(0), heldSample_(0) {}
    
    // Enable/disable bit crushing
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set bit depth (1-16 bits)
    void setBitDepth(int bits)
    {
        bitDepth_ = std::max(1, std::min(16, bits));
        updateBitMask();
    }
    int getBitDepth() const { return bitDepth_; }
    
    // Set sample rate reduction factor (1-128)
    void setSampleRateReduction(int factor)
    {
        sampleRateReduction_ = std::max(1, std::min(128, factor));
    }
    int getSampleRateReduction() const { return sampleRateReduction_; }
    
    // Process a single sample
    int16_t processSample(int16_t input)
    {
        if (!enabled_) return input;
        
        // Sample rate reduction (sample and hold)
        if (++sampleCounter_ >= sampleRateReduction_)
        {
            sampleCounter_ = 0;
            
            // Bit depth reduction
            if (bitDepth_ < 16)
            {
                // Quantize to reduced bit depth
                int32_t quantized = input;
                
                // Scale to reduced bit range
                int32_t maxValue = (1 << (bitDepth_ - 1)) - 1;
                quantized = (quantized * maxValue) / 32767;
                
                // Quantize
                quantized = (quantized * 32767) / maxValue;
                
                // Apply bit mask for authentic digital aliasing
                heldSample_ = static_cast<int16_t>(quantized & bitMask_);
            }
            else
            {
                heldSample_ = input;
            }
        }
        
        return heldSample_;
    }
    
    // Process a block of samples
    void processBlock(int16_t* buffer, size_t size)
    {
        if (!enabled_) return;
        
        for (size_t i = 0; i < size; ++i)
        {
            buffer[i] = processSample(buffer[i]);
        }
    }
    
    // Process stereo block
    void processBlockStereo(int16_t* leftBuffer, int16_t* rightBuffer, size_t size)
    {
        if (!enabled_) return;
        
        for (size_t i = 0; i < size; ++i)
        {
            // Process left and right channels independently for stereo width
            if (++sampleCounter_ >= sampleRateReduction_)
            {
                sampleCounter_ = 0;
                
                if (bitDepth_ < 16)
                {
                    // Process left channel
                    int32_t maxValue = (1 << (bitDepth_ - 1)) - 1;
                    int32_t quantizedL = (leftBuffer[i] * maxValue) / 32767;
                    quantizedL = (quantizedL * 32767) / maxValue;
                    heldSampleLeft_ = static_cast<int16_t>(quantizedL & bitMask_);
                    
                    // Process right channel
                    int32_t quantizedR = (rightBuffer[i] * maxValue) / 32767;
                    quantizedR = (quantizedR * 32767) / maxValue;
                    heldSampleRight_ = static_cast<int16_t>(quantizedR & bitMask_);
                }
                else
                {
                    heldSampleLeft_ = leftBuffer[i];
                    heldSampleRight_ = rightBuffer[i];
                }
            }
            
            leftBuffer[i] = heldSampleLeft_;
            rightBuffer[i] = heldSampleRight_;
        }
    }
    
    // Reset the bit crusher state
    void reset()
    {
        sampleCounter_ = 0;
        heldSample_ = 0;
        heldSampleLeft_ = 0;
        heldSampleRight_ = 0;
    }
    
    // Set parameters from normalized values (0-1)
    void setBitDepthNormalized(float value)
    {
        // Map 0-1 to 1-16 bits
        setBitDepth(1 + static_cast<int>(value * 15));
    }
    
    void setSampleRateReductionNormalized(float value)
    {
        // Map 0-1 to 1-128 with exponential curve for better control
        float exp = value * value * value;  // Cubic curve
        setSampleRateReduction(1 + static_cast<int>(exp * 127));
    }
    
private:
    bool enabled_;
    int bitDepth_;
    int sampleRateReduction_;
    int sampleCounter_;
    int16_t heldSample_;
    int16_t heldSampleLeft_;
    int16_t heldSampleRight_;
    uint32_t bitMask_;
    
    void updateBitMask()
    {
        if (bitDepth_ >= 16)
        {
            bitMask_ = 0xFFFF;
        }
        else
        {
            // Create mask for reduced bit depth
            bitMask_ = 0xFFFF;
            int shift = 16 - bitDepth_;
            bitMask_ = (bitMask_ >> shift) << shift;
        }
    }
};

} // namespace braidy