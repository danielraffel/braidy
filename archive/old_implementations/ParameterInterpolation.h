#pragma once

#include "BraidyTypes.h"

namespace braidy {

// Parameter interpolation for smooth parameter changes at audio rate
// Adapted from Mutable Instruments Braids parameter_interpolation.h

template<int num_parameters, int update_period>
class ParameterInterpolation {
public:
    ParameterInterpolation() {
        for (int i = 0; i < num_parameters; ++i) {
            target_[i] = 0;
            current_[i] = 0;
            increment_[i] = 0;
        }
        update_counter_ = 0;
    }
    
    ~ParameterInterpolation() { }
    
    inline void Update(int parameter, int16_t target) {
        target_[parameter] = target;
    }
    
    inline int16_t Read(int parameter) const {
        return current_[parameter];
    }
    
    inline void Process() {
        ++update_counter_;
        if (update_counter_ >= update_period) {
            update_counter_ = 0;
            for (int i = 0; i < num_parameters; ++i) {
                int32_t error = target_[i] - current_[i];
                if (error != 0) {
                    increment_[i] = error / update_period;
                    if (increment_[i] == 0) {
                        increment_[i] = error > 0 ? 1 : -1;
                    }
                }
            }
        }
        
        for (int i = 0; i < num_parameters; ++i) {
            current_[i] += increment_[i];
        }
    }
    
private:
    int16_t target_[num_parameters];
    int16_t current_[num_parameters];
    int16_t increment_[num_parameters];
    int update_counter_;
    
    DISALLOW_COPY_AND_ASSIGN(ParameterInterpolation);
};

// Common parameter interpolator for macro oscillator
// 2 parameters (timbre, color) updated every 24 samples (kBlockSize)
using MacroParameterInterpolation = ParameterInterpolation<2, kBlockSize>;

// Smooth parameter changes for UI
template<typename T>
class ParameterSmoother {
public:
    ParameterSmoother() : target_(0), current_(0), slew_(0.1f) {}
    
    void SetTarget(T target) {
        target_ = target;
    }
    
    void SetSlew(float slew) {
        slew_ = slew;
    }
    
    T Process() {
        T error = target_ - current_;
        current_ += static_cast<T>(error * slew_);
        
        // Snap to target when very close
        if (error * error < static_cast<T>(0.0001)) {
            current_ = target_;
        }
        
        return current_;
    }
    
    T value() const { return current_; }
    void set_value(T value) { current_ = value; target_ = value; }
    
private:
    T target_;
    T current_;
    float slew_;
};

using FloatSmoother = ParameterSmoother<float>;
using Int16Smoother = ParameterSmoother<int16_t>;

}  // namespace braidy