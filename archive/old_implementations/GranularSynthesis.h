// Granular Synthesis Implementation
// For CLDS (Granular Cloud) and PART (Particle Noise) algorithms

#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace braidy {

// Grain envelope types
enum class GrainEnvelope {
    RECTANGULAR,
    TRIANGULAR,
    HANNING,
    GAUSSIAN,
    EXPONENTIAL
};

// Single grain structure
struct Grain {
    uint32_t phase;           // Current phase in buffer
    uint32_t phase_increment; // Playback speed
    uint32_t start_position;  // Start position in buffer
    uint32_t length;          // Grain length in samples
    uint32_t position;        // Current position within grain
    int16_t amplitude;        // Grain amplitude
    int16_t pan;             // Stereo position (-32768 to 32767)
    bool active;              // Is grain currently playing
    GrainEnvelope envelope;   // Envelope type
};

class GranularSynthesis {
public:
    static constexpr size_t kMaxGrains = 64;
    static constexpr size_t kBufferSize = 65536;  // 1.36 seconds at 48kHz
    
    GranularSynthesis() {
        Clear();
    }
    
    void Clear() {
        // Initialize buffer with synthesized material for granulation
        for (size_t i = 0; i < kBufferSize; ++i) {
            // Generate rich harmonic content for granulation
            float phase = (float)i / kBufferSize * 8.0f * M_PI;
            float sample = std::sin(phase) + 0.5f * std::sin(3.0f * phase) + 0.25f * std::sin(5.0f * phase);
            buffer_[i] = static_cast<int16_t>(sample * 16384.0f);
        }
        memset(grains_, 0, sizeof(grains_));
        write_position_ = 0;
        next_grain_counter_ = 0;
    }
    
    // Record input into circular buffer
    void RecordBuffer(const int16_t* input, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            buffer_[write_position_] = input[i];
            write_position_ = (write_position_ + 1) & (kBufferSize - 1);
        }
    }
    
    // Generate granular cloud
    void RenderGranularCloud(int16_t pitch, int16_t* parameter,
                            const uint8_t* sync, int16_t* buffer, size_t size) {
        // Parameters:
        // parameter[0] (TIMBRE): Grain density / spawn rate
        // parameter[1] (COLOR): Grain size / texture
        
        uint16_t density = parameter[0] + 32768;  // 0-65535
        uint16_t grain_size = parameter[1] + 32768;  // 0-65535
        
        // Calculate grain parameters
        uint32_t spawn_rate = 48 + (density >> 10);  // 48-112 grains/sec
        uint32_t grain_length = 480 + (grain_size >> 6);  // 10-1000ms
        
        // Pitch affects playback speed
        uint32_t base_increment = 65536;  // 1.0x speed
        if (pitch != 0) {
            float pitch_mult = std::pow(2.0f, pitch / 4096.0f);
            base_increment = static_cast<uint32_t>(65536 * pitch_mult);
        }
        
        // Clear output buffer
        memset(buffer, 0, size * sizeof(int16_t));
        
        // Process each sample
        for (size_t i = 0; i < size; ++i) {
            // Spawn new grains based on density
            if (++next_grain_counter_ >= (48000 / spawn_rate)) {
                next_grain_counter_ = 0;
                SpawnGrain(grain_length, base_increment);
            }
            
            // Mix all active grains
            int32_t sample = 0;
            int active_grains = 0;
            
            for (size_t g = 0; g < kMaxGrains; ++g) {
                if (grains_[g].active) {
                    sample += ProcessGrain(grains_[g]);
                    active_grains++;
                }
            }
            
            // Normalize output to prevent clipping
            if (active_grains > 0) {
                sample = sample / std::max(1, active_grains / 4);
            }
            
            // Soft clip and output
            buffer[i] = SoftClip(sample);
            
            // Handle sync if provided
            if (sync && sync[i]) {
                // Reset grain spawning on sync
                next_grain_counter_ = 0;
            }
        }
    }
    
    // Generate particle noise (sparse granular synthesis)
    void RenderParticleNoise(int16_t pitch, int16_t* parameter,
                            const uint8_t* sync, int16_t* buffer, size_t size) {
        // Parameters:
        // parameter[0] (TIMBRE): Particle density
        // parameter[1] (COLOR): Particle brightness/filtering
        
        uint16_t density = parameter[0] + 32768;
        uint16_t brightness = parameter[1] + 32768;
        
        // Sparse grains with noise source
        uint32_t spawn_probability = density >> 8;  // 0-255
        uint16_t filter_cutoff = brightness;
        
        // Clear output
        memset(buffer, 0, size * sizeof(int16_t));
        
        for (size_t i = 0; i < size; ++i) {
            // Randomly spawn particles
            if ((Random() & 0xFF) < spawn_probability) {
                SpawnParticle(pitch, filter_cutoff);
            }
            
            // Process active particles
            int32_t sample = 0;
            for (size_t g = 0; g < kMaxGrains; ++g) {
                if (grains_[g].active) {
                    sample += ProcessParticle(grains_[g], filter_cutoff);
                }
            }
            
            buffer[i] = SoftClip(sample);
            
            if (sync && sync[i]) {
                // Clear all particles on sync
                for (auto& grain : grains_) {
                    grain.active = false;
                }
            }
        }
    }
    
private:
    int16_t buffer_[kBufferSize];
    Grain grains_[kMaxGrains];
    uint32_t write_position_;
    uint32_t next_grain_counter_;
    uint32_t random_state_ = 1;
    
    // Simple random number generator
    uint32_t Random() {
        random_state_ = random_state_ * 1103515245 + 12345;
        return random_state_;
    }
    
    // Spawn a new grain
    void SpawnGrain(uint32_t length, uint32_t speed) {
        // Find inactive grain slot
        for (auto& grain : grains_) {
            if (!grain.active) {
                grain.active = true;
                grain.length = length;
                grain.position = 0;
                grain.phase_increment = speed;
                
                // Random start position in buffer
                grain.start_position = Random() & (kBufferSize - 1);
                grain.phase = grain.start_position << 16;
                
                // Random amplitude variation
                grain.amplitude = 16384 + ((Random() & 0x7FFF) - 16384);
                
                // Random pan position
                grain.pan = (Random() & 0xFFFF) - 32768;
                
                // Random envelope
                grain.envelope = static_cast<GrainEnvelope>(Random() % 5);
                
                break;
            }
        }
    }
    
    // Spawn a particle (short noise burst)
    void SpawnParticle(int16_t pitch, uint16_t brightness) {
        for (auto& grain : grains_) {
            if (!grain.active) {
                grain.active = true;
                grain.length = 48 + (Random() & 0x1FF);  // 1-10ms
                grain.position = 0;
                
                // Pitch affects particle frequency
                grain.phase_increment = 32768 + pitch * 16;
                
                // Brightness affects amplitude
                grain.amplitude = (brightness >> 8) * 128;
                
                // Random position
                grain.start_position = Random() & (kBufferSize - 1);
                grain.phase = grain.start_position << 16;
                
                grain.envelope = GrainEnvelope::EXPONENTIAL;
                break;
            }
        }
    }
    
    // Process a single grain
    int16_t ProcessGrain(Grain& grain) {
        if (!grain.active) return 0;
        
        // Get sample from buffer with interpolation
        uint32_t index = grain.phase >> 16;
        uint32_t frac = grain.phase & 0xFFFF;
        
        int32_t s1 = buffer_[index & (kBufferSize - 1)];
        int32_t s2 = buffer_[(index + 1) & (kBufferSize - 1)];
        int32_t sample = s1 + ((s2 - s1) * frac >> 16);
        
        // Apply envelope
        float env = GetEnvelope(grain.position, grain.length, grain.envelope);
        sample = static_cast<int32_t>(sample * env);
        
        // Apply amplitude
        sample = (sample * grain.amplitude) >> 15;
        
        // Update position
        grain.phase += grain.phase_increment;
        grain.position++;
        
        // Check if grain is finished
        if (grain.position >= grain.length) {
            grain.active = false;
        }
        
        return static_cast<int16_t>(sample);
    }
    
    // Process a particle with filtering
    int16_t ProcessParticle(Grain& grain, uint16_t filter_cutoff) {
        if (!grain.active) return 0;
        
        // Generate noise
        int32_t sample = (Random() & 0xFFFF) - 32768;
        
        // Apply simple lowpass filter
        static int32_t filter_state = 0;
        int32_t cutoff = filter_cutoff >> 8;
        filter_state += ((sample - filter_state) * cutoff) >> 8;
        sample = filter_state;
        
        // Apply envelope
        float env = GetEnvelope(grain.position, grain.length, grain.envelope);
        sample = static_cast<int32_t>((sample * env * grain.amplitude) / 32768.0f);
        
        // Update position
        grain.position++;
        if (grain.position >= grain.length) {
            grain.active = false;
        }
        
        return static_cast<int16_t>(sample);
    }
    
    // Get envelope value for grain position
    float GetEnvelope(uint32_t position, uint32_t length, GrainEnvelope type) {
        float x = static_cast<float>(position) / length;
        
        switch (type) {
            case GrainEnvelope::RECTANGULAR:
                return 1.0f;
                
            case GrainEnvelope::TRIANGULAR:
                return 1.0f - std::abs(2.0f * x - 1.0f);
                
            case GrainEnvelope::HANNING:
                return 0.5f * (1.0f - std::cos(2.0f * M_PI * x));
                
            case GrainEnvelope::GAUSSIAN:
                {
                    float t = 2.0f * x - 1.0f;
                    return std::exp(-5.0f * t * t);
                }
                
            case GrainEnvelope::EXPONENTIAL:
                return std::exp(-5.0f * x);
                
            default:
                return 1.0f;
        }
    }
    
    // Soft clipping function
    int16_t SoftClip(int32_t sample) {
        if (sample > 32767) {
            return 32767;
        } else if (sample < -32768) {
            return -32768;
        }
        return static_cast<int16_t>(sample);
    }
};

} // namespace braidy