#include "MacroOscillator.h"
#include "DigitalOscillator.h"
#include <cstdio>

namespace braidy {

// Function table mapping shapes to render functions
const MacroOscillator::RenderFn MacroOscillator::fn_table_[] = {
    &MacroOscillator::RenderCSaw,           // CSAW
    &MacroOscillator::RenderMorph,          // MORPH
    &MacroOscillator::RenderSawSquare,      // SAW_SQUARE
    &MacroOscillator::RenderSineTriangle,   // SINE_TRIANGLE
    &MacroOscillator::RenderBuzz,           // BUZZ
    
    &MacroOscillator::RenderSub,            // SQUARE_SUB
    &MacroOscillator::RenderSub,            // SAW_SUB
    &MacroOscillator::RenderDualSync,       // SQUARE_SYNC
    &MacroOscillator::RenderDualSync,       // SAW_SYNC
    &MacroOscillator::RenderTriple,         // TRIPLE_SAW
    &MacroOscillator::RenderTriple,         // TRIPLE_SQUARE
    &MacroOscillator::RenderTriple,         // TRIPLE_TRIANGLE
    &MacroOscillator::RenderTriple,         // TRIPLE_SINE
    &MacroOscillator::RenderTriple,         // TRIPLE_RING_MOD
    &MacroOscillator::RenderSawSquare,      // SAW_SWARM (placeholder)
    &MacroOscillator::RenderDigital,        // SAW_COMB
    &MacroOscillator::RenderDigital,        // TOY
    
    // Digital models - placeholder for now
    &MacroOscillator::RenderDigital,        // DIGITAL_FILTER_LP
    &MacroOscillator::RenderDigital,        // DIGITAL_FILTER_PK
    &MacroOscillator::RenderDigital,        // DIGITAL_FILTER_BP
    &MacroOscillator::RenderDigital,        // DIGITAL_FILTER_HP
    &MacroOscillator::RenderDigital,        // VOSIM
    &MacroOscillator::RenderDigital,        // VOWEL
    &MacroOscillator::RenderDigital,        // VOWEL_FOF
    &MacroOscillator::RenderDigital,        // HARMONICS
    &MacroOscillator::RenderDigital,        // FM
    &MacroOscillator::RenderDigital,        // FEEDBACK_FM
    &MacroOscillator::RenderDigital,        // CHAOTIC_FEEDBACK_FM
    &MacroOscillator::RenderDigital,        // PLUCKED
    &MacroOscillator::RenderDigital,        // BOWED
    &MacroOscillator::RenderDigital,        // BLOWN
    &MacroOscillator::RenderDigital,        // FLUTED
    &MacroOscillator::RenderDigital,        // STRUCK_BELL
    &MacroOscillator::RenderDigital,        // STRUCK_DRUM
    &MacroOscillator::RenderDigital,        // KICK
    &MacroOscillator::RenderDigital,        // CYMBAL
    &MacroOscillator::RenderDigital,        // SNARE
    &MacroOscillator::RenderDigital,        // WAVETABLES
    &MacroOscillator::RenderDigital,        // WAVE_MAP
    &MacroOscillator::RenderDigital,        // WAVE_LINE
    &MacroOscillator::RenderDigital,        // WAVE_PARAPHONIC
    &MacroOscillator::RenderDigital,        // FILTERED_NOISE
    &MacroOscillator::RenderDigital,        // TWIN_PEAKS_NOISE
    &MacroOscillator::RenderDigital,        // CLOCKED_NOISE
    &MacroOscillator::RenderDigital,        // GRANULAR_CLOUD
    &MacroOscillator::RenderDigital,        // PARTICLE_NOISE
    &MacroOscillator::RenderDigital,        // DIGITAL_MODULATION
    &MacroOscillator::RenderDigital,        // QUESTION_MARK
};

MacroOscillator::MacroOscillator() : digital_oscillator_(nullptr) {
    digital_oscillator_ = new DigitalOscillator();
    Init();
}

MacroOscillator::~MacroOscillator() {
    delete digital_oscillator_;
}

void MacroOscillator::Init() {
    shape_ = MacroOscillatorShape::CSAW;
    pitch_ = kPitchC4;
    parameter_[0] = 0;
    parameter_[1] = 0;
    previous_parameter_[0] = 0;
    previous_parameter_[1] = 0;
    
    lp_state_ = 0;
    
    // Initialize oscillators
    for (int i = 0; i < 3; ++i) {
        analog_oscillator_[i].Init();
    }
    
    if (digital_oscillator_) {
        digital_oscillator_->Init();
    }
    
    // Clear buffers
    for (int i = 0; i < kBlockSize; ++i) {
        sync_buffer_[i] = 0;
        temp_buffer_[i] = 0;
    }
    
    // Initialize META mode
    meta_mode_enabled_ = false;
    meta_position_ = 0.0f;
    meta_shape_a_ = MacroOscillatorShape::CSAW;
    meta_shape_b_ = MacroOscillatorShape::MORPH;
    meta_morph_ = 0.0f;
    
    // Initialize quantizer (disabled by default)
    quantizer_.setEnabled(false);
    quantizer_.setScale(Quantizer::CHROMATIC);
    quantizer_.setRootNote(0);  // C
    
    // Initialize bit crusher (disabled by default)
    bit_crusher_.setEnabled(false);
    bit_crusher_.setBitDepth(16);
    bit_crusher_.setSampleRateReduction(1);
}

void MacroOscillator::Strike() {
    if (digital_oscillator_) {
        digital_oscillator_->Strike();
    }
}

void MacroOscillator::UpdateParameters() {
    // Update parameter interpolation for smooth changes
    parameter_interpolation_.Update(0, parameter_[0]);
    parameter_interpolation_.Update(1, parameter_[1]);
    parameter_interpolation_.Process();
    
    previous_parameter_[0] = parameter_[0];
    previous_parameter_[1] = parameter_[1];
}

void MacroOscillator::SetMetaPosition(float position) {
    meta_position_ = std::max(0.0f, std::min(1.0f, position));
    
    if (meta_mode_enabled_) {
        // Map position to algorithm selection
        // We have 48 algorithms total
        const int numAlgorithms = static_cast<int>(MacroOscillatorShape::LAST);
        float scaledPosition = meta_position_ * (numAlgorithms - 1);
        int algorithmIndex = static_cast<int>(scaledPosition);
        
        // Get the two algorithms to morph between
        meta_shape_a_ = static_cast<MacroOscillatorShape>(algorithmIndex);
        meta_shape_b_ = static_cast<MacroOscillatorShape>(
            std::min(algorithmIndex + 1, numAlgorithms - 1));
        
        // Calculate morph amount between them
        meta_morph_ = scaledPosition - algorithmIndex;
    }
}

void MacroOscillator::Render(const uint8_t* sync_buffer, int16_t* buffer, size_t size) {
    if (size > kBlockSize) {
        size = kBlockSize;  // Ensure we don't overflow our temp buffers
    }
    
    UpdateParameters();
    
    // Apply pitch quantization if enabled
    int16_t quantized_pitch = pitch_;
    if (quantizer_.isEnabled()) {
        quantized_pitch = quantizer_.quantize(pitch_);
    }
    
    // Debug output for shape changes
    static MacroOscillatorShape last_shape = MacroOscillatorShape::LAST;
    if (shape_ != last_shape) {
        printf("=== MACRO OSCILLATOR RENDER DEBUG ===\n");
        printf("Shape changed to: %d\n", static_cast<int>(shape_));
        printf("Shape index: %d\n", static_cast<int>(shape_));
        printf("Max shapes: %d\n", static_cast<int>(MacroOscillatorShape::LAST));
        last_shape = shape_;
        
        // Print which render function will be called
        int shape_index = static_cast<int>(shape_);
        if (shape_index >= 0 && shape_index < static_cast<int>(MacroOscillatorShape::LAST)) {
            const char* render_function_names[] = {
                "RenderCSaw", "RenderMorph", "RenderSawSquare", "RenderSineTriangle", "RenderBuzz",
                "RenderSub", "RenderSub", "RenderDualSync", "RenderDualSync", "RenderTriple", 
                "RenderTriple", "RenderTriple", "RenderTriple", "RenderTriple", "RenderSawSquare",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital", "RenderDigital",
                "RenderDigital", "RenderDigital", "RenderDigital"
            };
            
            if (shape_index < 48) {
                printf("Will call: %s\n", render_function_names[shape_index]);
            }
        } else {
            printf("Shape index out of range, will call RenderCSaw fallback\n");
        }
        printf("=== END MACRO OSCILLATOR RENDER DEBUG ===\n");
    }
    
    // Store original pitch and apply quantized pitch
    int16_t original_pitch = pitch_;
    if (quantizer_.isEnabled()) {
        pitch_ = quantized_pitch;
    }
    
    // Handle META mode
    if (meta_mode_enabled_) {
        // Render two algorithms and morph between them
        int16_t buffer_a[kBlockSize];
        int16_t buffer_b[kBlockSize];
        
        // Render algorithm A
        shape_ = meta_shape_a_;
        int shape_index_a = static_cast<int>(meta_shape_a_);
        if (shape_index_a >= 0 && shape_index_a < static_cast<int>(MacroOscillatorShape::LAST)) {
            (this->*fn_table_[shape_index_a])(sync_buffer, buffer_a, size);
        }
        
        // Render algorithm B
        shape_ = meta_shape_b_;
        int shape_index_b = static_cast<int>(meta_shape_b_);
        if (shape_index_b >= 0 && shape_index_b < static_cast<int>(MacroOscillatorShape::LAST)) {
            (this->*fn_table_[shape_index_b])(sync_buffer, buffer_b, size);
        }
        
        // Morph between the two
        for (size_t i = 0; i < size; ++i) {
            int32_t a = buffer_a[i];
            int32_t b = buffer_b[i];
            buffer[i] = static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(meta_morph_ * 256)) / 256);
        }
        
        // Restore original shape
        shape_ = static_cast<MacroOscillatorShape>(shape_index_a);
    } else {
        // Normal rendering
        int shape_index = static_cast<int>(shape_);
        if (shape_index >= 0 && shape_index < static_cast<int>(MacroOscillatorShape::LAST)) {
            (this->*fn_table_[shape_index])(sync_buffer, buffer, size);
        } else {
            // Fallback to CSAW
            RenderCSaw(sync_buffer, buffer, size);
        }
    }
    
    // Apply bit crusher if enabled
    if (bit_crusher_.isEnabled()) {
        bit_crusher_.processBlock(buffer, size);
    }
    
    // Restore original pitch
    pitch_ = original_pitch;
}

void MacroOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Classic sawtooth with parameter morphing
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::CSAW);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_parameter(parameter_interpolation_.Read(0));
    analog_oscillator_[0].set_aux_parameter(parameter_interpolation_.Read(1));
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
}

void MacroOscillator::RenderMorph(const uint8_t* sync, int16_t* buffer, size_t size) {
    // MORPH: continuously variable waveform
    // TIMBRE morphs through triangle→saw→square→pulse
    // COLOR adds fold/wrap distortion
    
    int16_t morph = parameter_interpolation_.Read(0);  // TIMBRE
    int16_t distortion = parameter_interpolation_.Read(1);  // COLOR
    
    // Morph regions (each ~8192 units wide):
    // 0-8192: Triangle
    // 8192-16384: Triangle→Saw
    // 16384-24576: Saw→Square  
    // 24576-32767: Square→Pulse
    
    if (morph < 8192) {
        // Pure triangle
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[0].set_parameter(0);
        analog_oscillator_[0].set_aux_parameter(distortion);
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        
    } else if (morph < 16384) {
        // Triangle to Saw morph
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SAW);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        analog_oscillator_[0].set_aux_parameter(distortion);
        analog_oscillator_[1].set_aux_parameter(distortion);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Crossfade
        uint16_t fade = static_cast<uint16_t>((morph - 8192) * 8);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
        
    } else if (morph < 24576) {
        // Saw to Square morph
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        analog_oscillator_[0].set_aux_parameter(distortion);
        analog_oscillator_[1].set_aux_parameter(distortion);
        analog_oscillator_[1].set_parameter(16384);  // 50% pulse width for square
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Crossfade
        uint16_t fade = static_cast<uint16_t>((morph - 16384) * 8);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
        
    } else {
        // Square to Pulse (narrow pulse width)
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SQUARE);
        analog_oscillator_[0].set_pitch(pitch_);
        
        // Morph pulse width from 50% down to ~10%
        int16_t pulse_width = 16384 - ((morph - 24576) * 2);
        if (pulse_width < 3277) pulse_width = 3277;  // ~10% minimum
        
        analog_oscillator_[0].set_parameter(pulse_width);
        analog_oscillator_[0].set_aux_parameter(distortion);
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    }
    
    // Apply COLOR distortion if present
    if (distortion > 8192) {
        // Apply fold distortion for positive COLOR values
        int16_t fold_amount = (distortion - 8192) >> 1;
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            sample = (sample * (32767 - fold_amount)) >> 15;
            
            // Folding
            if (sample > 16383) {
                sample = 32767 - sample;
            } else if (sample < -16384) {
                sample = -32768 - sample;
            }
            buffer[i] = ClipS16(sample);
        }
    } else if (distortion < -8192) {
        // Apply wrap distortion for negative COLOR values
        int16_t wrap_amount = (-distortion - 8192) >> 1;
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            sample = (sample * (32767 + wrap_amount)) >> 14;
            // Wrapping
            while (sample > 32767) sample -= 65536;
            while (sample < -32768) sample += 65536;
            buffer[i] = static_cast<int16_t>(sample);
        }
    }
}

void MacroOscillator::RenderSawSquare(const uint8_t* sync, int16_t* buffer, size_t size) {
    // /\\-_ oscillator with continuously variable waveform
    // TIMBRE: morphs from sawtooth to square
    // COLOR: pulse width when square (full CCW = 5%, center = 50%, full CW = 95%)
    
    int16_t morph = parameter_interpolation_.Read(0);  // TIMBRE
    int16_t pulse_width = parameter_interpolation_.Read(1);  // COLOR
    
    // Convert COLOR to pulse width (5% to 95%)
    // -32768 = 5%, 0 = 50%, 32767 = 95%
    int32_t pw = 16384;  // Default 50%
    if (pulse_width < 0) {
        // 5% to 50% range
        pw = 1638 + ((pulse_width + 32768) * 14746 >> 15);  // Maps to 5%-50%
    } else {
        // 50% to 95% range  
        pw = 16384 + (pulse_width * 14746 >> 15);  // Maps to 50%-95%
    }
    
    if (morph < 16384) {
        // Pure saw or morphing to square
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        analog_oscillator_[1].set_parameter(pw);  // Apply pulse width to square
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Crossfade based on TIMBRE
        uint16_t fade = static_cast<uint16_t>(morph * 4);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
    } else {
        // Full square with variable pulse width
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SQUARE);
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[0].set_parameter(pw);
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    }
}

void MacroOscillator::RenderSineTriangle(const uint8_t* sync, int16_t* buffer, size_t size) {
    // FOLD: sine and triangle oscillator with wavefolder
    // TIMBRE: wavefolder amount (0 = pure wave, max = heavily folded)
    // COLOR: waveform (full CCW = sine, full CW = triangle)
    
    int16_t fold_amount = parameter_interpolation_.Read(0);  // TIMBRE
    int16_t wave_balance = parameter_interpolation_.Read(1);  // COLOR
    
    // Generate base waveform based on COLOR
    if (wave_balance < -8192) {
        // Mostly sine
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SINE);
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    } else if (wave_balance > 8192) {
        // Mostly triangle
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    } else {
        // Mix of sine and triangle
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SINE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::TRIANGLE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Crossfade between sine and triangle
        uint16_t balance = static_cast<uint16_t>((wave_balance + 32768) >> 1);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
        }
    }
    
    // Apply wavefolding based on TIMBRE
    if (fold_amount > 1024) {
        // Scale fold amount (0 to 32767 -> 1.0 to 8.0 gain before folding)
        int32_t gain = 32768 + (fold_amount * 7);  // 1x to 8x gain
        
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            
            // Apply gain
            sample = (sample * gain) >> 15;
            
            // Wavefold: reflect at boundaries
            while (sample > 32767 || sample < -32768) {
                if (sample > 32767) {
                    sample = 65535 - sample;
                } else if (sample < -32768) {
                    sample = -65536 - sample;
                }
            }
            
            buffer[i] = ClipS16(sample);
        }
    }
}

void MacroOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Buzz harmonics
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::BUZZ);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_parameter(parameter_interpolation_.Read(0));  // Number of harmonics
    analog_oscillator_[0].set_aux_parameter(parameter_interpolation_.Read(1));  // Harmonic balance
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
}

void MacroOscillator::RenderSub(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Main oscillator + sub oscillator
    AnalogOscillatorShape main_shape = (shape_ == MacroOscillatorShape::SQUARE_SUB) ? 
        AnalogOscillatorShape::SQUARE : AnalogOscillatorShape::SAW;
    
    analog_oscillator_[0].set_shape(main_shape);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_parameter(parameter_interpolation_.Read(0));
    
    // Sub oscillator (one octave lower)
    analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
    analog_oscillator_[1].set_pitch(pitch_ - kOctave);
    analog_oscillator_[1].set_parameter(0);
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
    
    // Mix main and sub
    int16_t sub_level = parameter_interpolation_.Read(1);
    for (size_t i = 0; i < size; ++i) {
        int32_t mixed = buffer[i] + ((temp_buffer_[i] * sub_level) >> 15);
        buffer[i] = ClipS16(mixed);
    }
}

void MacroOscillator::RenderDualSync(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Hard sync between two oscillators
    AnalogOscillatorShape sync_shape = (shape_ == MacroOscillatorShape::SQUARE_SYNC) ? 
        AnalogOscillatorShape::SQUARE : AnalogOscillatorShape::SAW;
    
    // Master oscillator
    analog_oscillator_[0].set_shape(sync_shape);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_sync_mode(SyncMode::MASTER);
    
    // Slave oscillator (detuned)
    int16_t detune = parameter_interpolation_.Read(0);
    analog_oscillator_[1].set_shape(sync_shape);
    analog_oscillator_[1].set_pitch(pitch_ + detune);
    analog_oscillator_[1].set_sync_mode(SyncMode::SLAVE);
    
    // Generate sync signal from master
    analog_oscillator_[0].Render(sync, buffer, sync_buffer_, size);
    
    // Render slave with sync
    analog_oscillator_[1].Render(sync_buffer_, temp_buffer_, nullptr, size);
    
    // Mix based on parameter 2
    uint16_t balance = static_cast<uint16_t>(parameter_interpolation_.Read(1) * 2);
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
    }
}

void MacroOscillator::RenderTriple(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Three detuned oscillators
    AnalogOscillatorShape triple_shape = AnalogOscillatorShape::SAW;
    
    switch (shape_) {
        case MacroOscillatorShape::TRIPLE_SAW:
            triple_shape = AnalogOscillatorShape::SAW;
            break;
        case MacroOscillatorShape::TRIPLE_SQUARE:
            triple_shape = AnalogOscillatorShape::SQUARE;
            break;
        case MacroOscillatorShape::TRIPLE_TRIANGLE:
            triple_shape = AnalogOscillatorShape::TRIANGLE;
            break;
        case MacroOscillatorShape::TRIPLE_SINE:
            triple_shape = AnalogOscillatorShape::SINE;
            break;
        default:
            triple_shape = AnalogOscillatorShape::SAW;
            break;
    }
    
    int16_t detune = parameter_interpolation_.Read(0);  // Detune amount
    int16_t spread = parameter_interpolation_.Read(1);   // Stereo spread
    
    // Three oscillators with slight detuning
    for (int osc = 0; osc < 3; ++osc) {
        analog_oscillator_[osc].set_shape(triple_shape);
        analog_oscillator_[osc].set_pitch(pitch_ + (osc - 1) * detune / 2);
    }
    
    // Render all three
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
    
    // Mix first two
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = (buffer[i] + temp_buffer_[i]) >> 1;
    }
    
    // Add third oscillator
    analog_oscillator_[2].Render(sync, temp_buffer_, nullptr, size);
    for (size_t i = 0; i < size; ++i) {
        int32_t mixed = buffer[i] + (temp_buffer_[i] >> 1);
        buffer[i] = ClipS16(mixed);
    }
}

void MacroOscillator::RenderDigital(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Placeholder for digital synthesis - will be implemented in Phase 2
    if (digital_oscillator_) {
        digital_oscillator_->Render(shape_, pitch_, 
                                  parameter_interpolation_.Read(0),
                                  parameter_interpolation_.Read(1),
                                  sync, buffer, size);
    } else {
        // Fallback to CSAW if no digital oscillator
        RenderCSaw(sync, buffer, size);
    }
}

void MacroOscillator::ConfigureTriple(AnalogOscillatorShape shape) {
    for (int i = 0; i < 3; ++i) {
        analog_oscillator_[i].set_shape(shape);
    }
}

}  // namespace braidy