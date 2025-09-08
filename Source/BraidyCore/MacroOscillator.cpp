#include "MacroOscillator.h"
#include "DigitalOscillator.h"
#include <cstdio>

namespace braidy {

// Function table mapping shapes to render functions
const MacroOscillator::RenderFn MacroOscillator::fn_table_[] = {
    &MacroOscillator::RenderCSaw,           // 0: CSAW - Bandlimited saw with phase randomizer
    &MacroOscillator::RenderMorph,          // 1: MORPH - Triangle→Saw→Square→PWM morph
    &MacroOscillator::RenderSawSquare,      // 2: SAW_SQUARE - Saw/Square mix
    &MacroOscillator::RenderSineTriangle,   // 3: SINE_TRIANGLE - Sine/Triangle with fold
    &MacroOscillator::RenderBuzz,           // 4: BUZZ - Comb filtered sawtooth
    
    &MacroOscillator::RenderSub,            // 5: SQUARE_SUB - Square with sub oscillator
    &MacroOscillator::RenderSub,            // 6: SAW_SUB - Saw with sub oscillator
    &MacroOscillator::RenderDualSync,       // 7: SQUARE_SYNC - Square with hard sync
    &MacroOscillator::RenderDualSync,       // 8: SAW_SYNC - Saw with hard sync
    &MacroOscillator::RenderTriple,         // 9: TRIPLE_SAW - 3 detuned saws
    &MacroOscillator::RenderTriple,         // 10: TRIPLE_SQUARE - 3 detuned squares
    &MacroOscillator::RenderTriple,         // 11: TRIPLE_TRIANGLE - 3 detuned triangles
    &MacroOscillator::RenderTriple,         // 12: TRIPLE_SINE - 3 detuned sines
    &MacroOscillator::RenderDigital,        // 13: TRIPLE_RING_MOD - Carrier × Mod1 × Mod2 (digital)
    &MacroOscillator::RenderDigital,        // 14: SAW_SWARM - 3-7 detuned saws (digital)
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
    &MacroOscillator::RenderWavetables,     // WAVETABLES
    &MacroOscillator::RenderWaveMap,        // WAVE_MAP
    &MacroOscillator::RenderWaveLine,       // WAVE_LINE
    &MacroOscillator::RenderWaveParaphonic, // WAVE_PARAPHONIC
    &MacroOscillator::RenderDigital,        // FILTERED_NOISE
    &MacroOscillator::RenderDigital,        // TWIN_PEAKS_NOISE
    &MacroOscillator::RenderDigital,        // CLOCKED_NOISE
    &MacroOscillator::RenderDigital,        // GRANULAR_CLOUD
    &MacroOscillator::RenderDigital,        // PARTICLE_NOISE
    &MacroOscillator::RenderDigital,        // DIGITAL_MODULATION
    &MacroOscillator::RenderDigital,        // QUESTION_MARK
};

MacroOscillator::MacroOscillator() : digital_oscillator_(nullptr), dsp_dispatcher_(nullptr) {
    // Keep digital_oscillator_ for backward compatibility but prefer dsp_dispatcher_
    digital_oscillator_ = new DigitalOscillator();
    dsp_dispatcher_ = new DSPDispatcher();
    Init();
}

MacroOscillator::~MacroOscillator() {
    delete digital_oscillator_;
    delete dsp_dispatcher_;
}

void MacroOscillator::Init() {
    shape_ = MacroOscillatorShape::CSAW;
    pitch_ = kPitchC4;
    parameter_[0] = 0;
    parameter_[1] = 0;
    previous_parameter_[0] = 0;
    previous_parameter_[1] = 0;
    fm_amount_ = 0;
    fm_ratio_ = kParameterCenter;
    
    lp_state_ = 0;
    
    // Initialize oscillators
    for (int i = 0; i < 3; ++i) {
        analog_oscillator_[i].Init();
    }
    
    if (digital_oscillator_) {
        digital_oscillator_->Init();
    }
    
    // Initialize wavetable state (manager is now in DSPDispatcher)
    wavetable_phase_ = 0;
    wavetable_phase_increment_ = 0;
    
    // Initialize paraphonic voices
    for (int i = 0; i < 8; ++i) {
        paraphonic_phases_[i] = 0;
        paraphonic_increments_[i] = 0;
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
    if (dsp_dispatcher_) {
        dsp_dispatcher_->Strike();
    } else if (digital_oscillator_) {
        digital_oscillator_->Strike();
    }
}

void MacroOscillator::UpdateParameters() {
    // Update parameter interpolation for smooth changes
    static int debug_count = 0;
    if (debug_count < 5) {
        printf("UpdateParameters DEBUG #%d: raw parameter_[0]=%d, parameter_[1]=%d\n", 
               debug_count, parameter_[0], parameter_[1]);
    }
    
    parameter_interpolation_.Update(0, parameter_[0]);
    parameter_interpolation_.Update(1, parameter_[1]);
    parameter_interpolation_.Process();
    
    if (debug_count < 5) {
        printf("  After interpolation: Read(0)=%d, Read(1)=%d\n", 
               parameter_interpolation_.Read(0), parameter_interpolation_.Read(1));
        debug_count++;
    }
    
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
    // CSAW: Bandlimited sawtooth with phase randomizer
    // Parameter[0] (TIMBRE): Phase randomization amount (0-32767)
    // Parameter[1] (COLOR): Low-pass filter cutoff (-32767 to 32767)
    
    // Use raw parameters directly (interpolation is broken for immediate use)
    int16_t phase_randomization = parameter_[0];  // TIMBRE
    int16_t color = parameter_[1];               // COLOR
    
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::CSAW);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_parameter(phase_randomization);
    analog_oscillator_[0].set_aux_parameter(color);
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    
    // Apply moderate gain compensation: scale by 1.0 (no gain) to prevent clipping
    // Original 13/8 (1.625x) was causing severe clipping - removed
    
    // Temporarily disable filtering to test if it's causing DC offset issues
    // TODO: Re-implement proper filter without DC accumulation
    /*
    if (color > 1024) {
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            
            // Simple low-pass filter when COLOR is positive
            lp_state_ = lp_state_ + ((sample - lp_state_) * color >> 15);
            sample = lp_state_;
            
            buffer[i] = ClipS16(sample);
        }
    }
    */
}

void MacroOscillator::RenderMorph(const uint8_t* sync, int16_t* buffer, size_t size) {
    // MORPH: Simple morphing between basic waveforms
    // FIX: Use single waveform selection to avoid frequency doubling
    
    int16_t morph = parameter_interpolation_.Read(0);  // TIMBRE
    int16_t color = parameter_interpolation_.Read(1);  // COLOR
    
    analog_oscillator_[0].set_pitch(pitch_);
    
    // Simplified morphing: directly select waveform based on morph position
    if (morph < 8192) {
        // Triangle
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
    } else if (morph < 16384) {
        // Saw
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
    } else if (morph < 24576) {
        // Square
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SQUARE);
        analog_oscillator_[0].set_parameter(16384);  // 50% pulse width
    } else {
        // Sine
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SINE);
    }
    
    // Render the selected waveform
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    
    // Apply SVF lowpass filter and violent overdrive based on COLOR
    if (color != 0) {
        // LP filter cutoff frequency based on COLOR
        int16_t cutoff_freq = std::abs(color);
        
        // Apply simple LP filter
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            lp_state_ = lp_state_ + ((sample - lp_state_) * cutoff_freq >> 15);
            sample = lp_state_;
            
            // Violent overdrive waveshaping when COLOR is high
            if (cutoff_freq > 20000) {
                // Heavy overdrive/fuzz
                sample = sample * 3;
                if (sample > 32767) sample = 32767;
                else if (sample < -32768) sample = -32768;
                // Asymmetric clipping
                if (sample > 16384) sample = 16384 + ((sample - 16384) >> 2);
                else if (sample < -16384) sample = -16384 + ((sample + 16384) >> 2);
            }
            
            buffer[i] = ClipS16(sample);
        }
    }
}

void MacroOscillator::RenderSawSquare(const uint8_t* sync, int16_t* buffer, size_t size) {
    // S/SQ: Saw/Square mix with variable waveform
    // Parameter[0] (TIMBRE): Pulse width for both oscillators (0-32767)
    // Parameter[1] (COLOR): Mix balance (interpolated)
    //   0 = pure saw, 32767 = pure square (attenuated to 148/256)
    
    int16_t pulse_width = parameter_interpolation_.Read(0);  // TIMBRE - affects both oscillators
    int16_t mix_balance = parameter_interpolation_.Read(1);  // COLOR - mix between saw and square
    
    // Configure oscillators
    // OSC1: VARIABLE_SAW with pulse width parameter
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::VARIABLE_SAW);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_parameter(pulse_width);
    
    // OSC2: SQUARE with pulse width parameter  
    analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
    analog_oscillator_[1].set_pitch(pitch_);
    analog_oscillator_[1].set_parameter(pulse_width);
    
    // Render both oscillators
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);        // Variable saw
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);   // Square
    
    // Mix based on COLOR parameter
    // 0 = pure saw, 32767 = pure square (attenuated to 148/256)
    for (size_t i = 0; i < size; ++i) {
        int32_t saw_sample = buffer[i];
        int32_t square_sample = temp_buffer_[i];
        
        // Attenuate square to 148/256 as per spec
        square_sample = (square_sample * 148) >> 8;
        
        // Interpolate between saw and attenuated square
        int32_t mixed;
        if (mix_balance <= 0) {
            mixed = saw_sample;  // Pure saw
        } else if (mix_balance >= 32767) {
            mixed = square_sample;  // Pure attenuated square
        } else {
            // Linear interpolation
            uint16_t square_gain = static_cast<uint16_t>(mix_balance * 2);  // Scale to 0-65534
            uint16_t saw_gain = 65535 - square_gain;
            mixed = (saw_sample * saw_gain + square_sample * square_gain) >> 16;
        }
        
        buffer[i] = ClipS16(mixed);
    }
}

void MacroOscillator::RenderSineTriangle(const uint8_t* sync, int16_t* buffer, size_t size) {
    // S/TR: Sine/Triangle with Fold
    // Parameter[0] (TIMBRE): Shape morph
    //   0-10922: Triangle fold amount
    //   10923-21845: Triangle → Square morph
    //   21846-32767: Sine → Triangle morph
    // Parameter[1] (COLOR): Wavefold amount
    
    int16_t shape_morph = parameter_interpolation_.Read(0);  // TIMBRE
    int16_t fold_amount = parameter_interpolation_.Read(1);  // COLOR
    
    if (shape_morph <= 10922) {
        // Stage 1: Triangle with fold amount
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[0].set_parameter(shape_morph);  // Use as fold parameter
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        
    } else if (shape_morph <= 21845) {
        // Stage 2: Triangle → Square morph
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        analog_oscillator_[1].set_parameter(16384);  // 50% pulse width for square
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Interpolate between triangle and square
        uint16_t balance = ((shape_morph - 10923) * 6) & 0xFFFF;
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
        }
        
    } else {
        // Stage 3: Sine → Triangle morph
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SINE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::TRIANGLE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Interpolate between sine and triangle
        uint16_t balance = ((shape_morph - 21846) * 6) & 0xFFFF;
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
        }
    }
    
    // Apply tri-fold waveshaping based on COLOR parameter
    if (fold_amount > 1024) {
        // Apply wavefold - tri-fold algorithm
        int32_t fold_gain = 32768 + ((fold_amount * 3) >> 1);  // Scale fold intensity
        
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            
            // Apply pre-fold gain
            sample = (sample * fold_gain) >> 15;
            
            // Tri-fold waveshaping: multiple reflection boundaries
            int fold_count = 0;
            const int max_folds = 4;  // Prevent infinite loops
            
            while ((sample > 32767 || sample < -32768) && fold_count < max_folds) {
                if (sample > 32767) {
                    sample = 65535 - sample;  // Reflect downward
                } else if (sample < -32768) {
                    sample = -65536 - sample;  // Reflect upward
                }
                fold_count++;
            }
            
            // Apply post-fold attenuation
            sample = (sample * 28672) >> 15;  // Scale down by ~87.5%
            
            buffer[i] = ClipS16(sample);
        }
    }
}

void MacroOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size) {
    // BUZZ: Comb-filtered sawtooth with harmonic emphasis
    // Parameter[0] (TIMBRE): Comb frequency/harmonics (0-32767)
    // Parameter[1] (COLOR): Filter amount/resonance (0-32767)
    
    int16_t comb_freq = parameter_interpolation_.Read(0);   // TIMBRE - harmonics control
    int16_t filter_amount = parameter_interpolation_.Read(1); // COLOR - filter resonance
    
    // Start with sawtooth oscillator
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    
    // Apply comb filtering for harmonic emphasis
    // Comb filter creates harmonic resonances at multiples of the base frequency
    
    static int16_t delay_line[256];  // Delay line for comb filter
    static int delay_index = 0;
    static bool initialized = false;
    
    if (!initialized) {
        for (int i = 0; i < 256; ++i) {
            delay_line[i] = 0;
        }
        initialized = true;
    }
    
    // Calculate delay time based on comb frequency parameter
    // Map 0-32767 to delay times of 4-128 samples
    int delay_time = 4 + ((comb_freq * 124) >> 15);
    
    // Calculate feedback amount based on filter amount
    // Map 0-32767 to feedback of 0-0.9 (to avoid instability)
    int32_t feedback = (filter_amount * 29490) >> 15;  // Scale to 0-29490 (~0.9 in 16-bit)
    
    for (size_t i = 0; i < size; ++i) {
        int32_t input_sample = buffer[i];
        
        // Read delayed sample
        int read_index = (delay_index - delay_time + 256) & 0xFF;
        int32_t delayed_sample = delay_line[read_index];
        
        // Comb filter equation: y[n] = x[n] + feedback * y[n-delay]
        int32_t output_sample = input_sample + ((delayed_sample * feedback) >> 15);
        
        // Soft clipping to prevent harsh overload
        if (output_sample > 32767) output_sample = 32767;
        else if (output_sample < -32768) output_sample = -32768;
        
        // Write to delay line
        delay_line[delay_index] = ClipS16(output_sample);
        delay_index = (delay_index + 1) & 0xFF;
        
        buffer[i] = ClipS16(output_sample);
    }
    
    // Apply additional harmonic shaping based on BUZZ characteristic
    for (size_t i = 0; i < size; ++i) {
        int32_t sample = buffer[i];
        
        // Add subtle harmonic coloration
        int32_t harmonic = sample >> 2;  // Quarter amplitude harmonic
        sample += harmonic;
        
        // Slight waveshaping for buzz character
        if (sample > 16384) {
            sample = 16384 + ((sample - 16384) >> 1);
        } else if (sample < -16384) {
            sample = -16384 + ((sample + 16384) >> 1);
        }
        
        buffer[i] = ClipS16(sample);
    }
}

void MacroOscillator::RenderSub(const uint8_t* sync, int16_t* buffer, size_t size) {
    // SUB oscillators: Main oscillator + sub oscillator (-1 octave)
    // Shape determination based on algorithm:
    //   SQUARE_SUB (5): Main = square, Sub = square
    //   SAW_SUB (6): Main = saw, Sub = saw
    // Parameter[0] (TIMBRE): Sub level (0 = none, 32767 = equal)
    // Parameter[1] (COLOR): 
    //   For square: Pulse width
    //   For saw: Detune amount
    
    int16_t sub_level = parameter_interpolation_.Read(0);    // TIMBRE - sub oscillator level
    int16_t color_param = parameter_interpolation_.Read(1);  // COLOR - pulse width or detune
    
    // Determine shapes based on algorithm
    AnalogOscillatorShape main_shape, sub_shape;
    if (shape_ == MacroOscillatorShape::SQUARE_SUB) {
        main_shape = AnalogOscillatorShape::SQUARE;
        sub_shape = AnalogOscillatorShape::SQUARE;
    } else {  // SAW_SUB
        main_shape = AnalogOscillatorShape::SAW;
        sub_shape = AnalogOscillatorShape::SAW;
    }
    
    // Configure main oscillator
    analog_oscillator_[0].set_shape(main_shape);
    analog_oscillator_[0].set_pitch(pitch_);
    
    // Configure sub oscillator (-1 octave = pitch - 12 * 128)
    analog_oscillator_[1].set_shape(sub_shape);
    analog_oscillator_[1].set_pitch(pitch_ - (12 << 7));  // -12 semitones = -1 octave
    
    // Apply COLOR parameter differently based on shape
    if (shape_ == MacroOscillatorShape::SQUARE_SUB) {
        // For square: COLOR controls pulse width for both oscillators
        analog_oscillator_[0].set_parameter(color_param);
        analog_oscillator_[1].set_parameter(color_param);
    } else {
        // For saw: COLOR controls detune amount
        // Apply slight detuning to main oscillator based on COLOR
        int16_t detune = (color_param >> 8);  // Scale detune amount
        analog_oscillator_[0].set_pitch(pitch_ + detune);
        analog_oscillator_[1].set_pitch(pitch_ - (12 << 7) - (detune >> 1));  // Sub with counter-detune
    }
    
    // Render both oscillators
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);        // Main
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);   // Sub
    
    // Mix main and sub based on TIMBRE parameter
    for (size_t i = 0; i < size; ++i) {
        int32_t main_sample = buffer[i];
        int32_t sub_sample = temp_buffer_[i];
        
        // Scale sub oscillator level: 0 = no sub, 32767 = equal level
        sub_sample = (sub_sample * sub_level) >> 15;
        
        // Mix with proper level balancing
        int32_t mixed = main_sample + sub_sample;
        
        // Apply slight compression to prevent overload when both are loud
        if (sub_level > 16384) {  // When sub is > 50%
            mixed = (mixed * 28672) >> 15;  // Scale down by ~87.5%
        }
        
        buffer[i] = ClipS16(mixed);
    }
}

void MacroOscillator::RenderDualSync(const uint8_t* sync, int16_t* buffer, size_t size) {
    // SYNC oscillators: Hard sync between master and slave
    // Shape determination based on algorithm:
    //   SQUARE_SYNC (7): Master = square
    //   SAW_SYNC (8): Master = saw
    // Parameter[0] (TIMBRE): Sync ratio (affects slave pitch)
    // Parameter[1] (COLOR): Balance between master/slave
    
    int16_t sync_ratio = parameter_interpolation_.Read(0);    // TIMBRE - sync amount/slave pitch offset
    int16_t balance = parameter_interpolation_.Read(1);       // COLOR - master/slave balance
    
    // Determine oscillator shape based on algorithm
    AnalogOscillatorShape sync_shape;
    if (shape_ == MacroOscillatorShape::SQUARE_SYNC) {
        sync_shape = AnalogOscillatorShape::SQUARE;
    } else {  // SAW_SYNC
        sync_shape = AnalogOscillatorShape::SAW;
    }
    
    // Configure master oscillator (provides sync signal)
    analog_oscillator_[0].set_shape(sync_shape);
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_sync_mode(SyncMode::MASTER);
    
    // Configure slave oscillator with pitch offset based on sync ratio
    // Map sync_ratio to pitch offset: 0 = unison, 32767 = +2 octaves
    int16_t slave_pitch_offset = (sync_ratio * 24) >> 7;  // Scale to ±24 semitones (2 octaves)
    int16_t slave_pitch = pitch_ + slave_pitch_offset;
    
    analog_oscillator_[1].set_shape(sync_shape);
    analog_oscillator_[1].set_pitch(slave_pitch);
    analog_oscillator_[1].set_sync_mode(SyncMode::SLAVE);
    
    // Render master oscillator and generate sync signal
    analog_oscillator_[0].Render(sync, buffer, sync_buffer_, size);
    
    // Render slave oscillator with sync from master
    analog_oscillator_[1].Render(sync_buffer_, temp_buffer_, nullptr, size);
    
    // Mix master and slave based on COLOR parameter
    for (size_t i = 0; i < size; ++i) {
        int32_t master_sample = buffer[i];
        int32_t slave_sample = temp_buffer_[i];
        
        // Balance calculation: 0 = pure master, 32767 = pure slave
        int32_t mixed;
        if (balance <= 0) {
            mixed = master_sample;  // Pure master
        } else if (balance >= 32767) {
            mixed = slave_sample;   // Pure slave
        } else {
            // Linear crossfade
            uint16_t slave_gain = static_cast<uint16_t>(balance * 2);  // Scale to 0-65534
            uint16_t master_gain = 65535 - slave_gain;
            mixed = (master_sample * master_gain + slave_sample * slave_gain) >> 16;
        }
        
        buffer[i] = ClipS16(mixed);
    }
    
    // Apply sync-specific processing
    // Hard sync creates characteristic "sweep" effect - enhance this
    if (sync_ratio > 8192) {  // When sync ratio is significant
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            
            // Add subtle sync-sweep coloration
            // This enhances the characteristic sync sweep sound
            if (sample > 0) {
                sample = sample + (sample >> 4);  // Slight boost to positive peaks
            } else {
                sample = sample + (sample >> 5);  // Smaller boost to negative peaks  
            }
            
            buffer[i] = ClipS16(sample);
        }
    }
}

void MacroOscillator::RenderTriple(const uint8_t* sync, int16_t* buffer, size_t size) {
    // TRIPLE oscillators: 3 detuned voices
    // Shapes: SAW (9), SQUARE (10), TRIANGLE (11), or SINE (12)
    // Parameter[0] (TIMBRE): Detune spread
    //   Maps to interval table (65 entries from -24 to +24 semitones)
    // Parameter[1] (COLOR): Voice spread pattern
    
    int16_t detune_spread = parameter_interpolation_.Read(0);  // TIMBRE - detune amount
    int16_t voice_spread = parameter_interpolation_.Read(1);   // COLOR - spread pattern
    
    // Determine oscillator shape based on algorithm
    AnalogOscillatorShape triple_shape;
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
    
    // Calculate detune interval from TIMBRE parameter
    // Map 0-32767 to semitone range -24 to +24 (using simplified interval table)
    int16_t detune_interval = ((detune_spread * 48) >> 15) - 24;  // -24 to +24 semitones
    detune_interval = detune_interval << 7;  // Convert to pitch units
    
    // Voice configuration based on COLOR parameter
    // Different spread patterns for varied stereo imaging
    int16_t voice_pitches[3];
    
    if (voice_spread < 10922) {
        // Tight cluster
        voice_pitches[0] = pitch_;                           // Center voice
        voice_pitches[1] = pitch_ + (detune_interval >> 2);  // Slight detune up
        voice_pitches[2] = pitch_ - (detune_interval >> 2);  // Slight detune down
    } else if (voice_spread < 21845) {
        // Wide spread
        voice_pitches[0] = pitch_;                           // Base pitch
        voice_pitches[1] = pitch_ + detune_interval;         // Full detune up
        voice_pitches[2] = pitch_ - detune_interval;         // Full detune down
    } else {
        // Harmonic spread
        voice_pitches[0] = pitch_;                           // Fundamental
        voice_pitches[1] = pitch_ + (detune_interval >> 1);  // Partial harmonic
        voice_pitches[2] = pitch_ + detune_interval;         // Higher harmonic
    }
    
    // Configure and render the three oscillators
    for (int osc = 0; osc < 3; ++osc) {
        analog_oscillator_[osc].set_shape(triple_shape);
        analog_oscillator_[osc].set_pitch(voice_pitches[osc]);
        
        // For square waves, add slight pulse width variation per voice
        if (triple_shape == AnalogOscillatorShape::SQUARE) {
            int16_t pulse_variation = (osc - 1) * 1638;  // ±5% variation
            analog_oscillator_[osc].set_parameter(16384 + pulse_variation);
        }
    }
    
    // Render all three voices to separate buffers initially
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);     // Voice 1 -> main buffer
    
    // Create temporary buffer for voice mixing
    static int16_t voice2_buffer[kBlockSize];
    static int16_t voice3_buffer[kBlockSize];
    
    analog_oscillator_[1].Render(sync, voice2_buffer, nullptr, size);  // Voice 2
    analog_oscillator_[2].Render(sync, voice3_buffer, nullptr, size);  // Voice 3
    
    // Mix the three voices with equal amplitude (10922 each ≈ 1/3)
    const int32_t voice_gain = 10922;  // ~1/3 amplitude per voice
    
    for (size_t i = 0; i < size; ++i) {
        int32_t voice1 = buffer[i];
        int32_t voice2 = voice2_buffer[i];
        int32_t voice3 = voice3_buffer[i];
        
        // Equal power mixing
        int32_t mixed = (voice1 * voice_gain + 
                        voice2 * voice_gain + 
                        voice3 * voice_gain) >> 15;
        
        // Apply slight saturation for warmth when all voices are present
        if (detune_spread > 16384) {  // When detune is significant
            if (mixed > 24576) {
                mixed = 24576 + ((mixed - 24576) >> 1);  // Soft limiting
            } else if (mixed < -24576) {
                mixed = -24576 + ((mixed + 24576) >> 1);
            }
        }
        
        buffer[i] = ClipS16(mixed);
    }
}

void MacroOscillator::RenderDigital(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Debug output for digital algorithm routing
    static MacroOscillatorShape last_digital_shape = MacroOscillatorShape::LAST;
    if (shape_ != last_digital_shape) {
        printf("=== RENDER DIGITAL DEBUG ===\n");
        printf("Digital algorithm shape: %d\n", static_cast<int>(shape_));
        printf("dsp_dispatcher_: %p\n", dsp_dispatcher_);
        printf("digital_oscillator_: %p\n", digital_oscillator_);
        last_digital_shape = shape_;
    }
    
    // Use interpolated parameters for smooth changes
    int16_t params[2] = { 
        parameter_interpolation_.Read(0),
        parameter_interpolation_.Read(1)
    };
    
    // Try DSP dispatcher first (newer, more complete implementation)
    if (dsp_dispatcher_) {
        printf("Using DSP dispatcher for shape %d\n", static_cast<int>(shape_));
        
        // Phase state for algorithms that need it
        static uint32_t phase = 0;
        uint32_t phase_increment = ComputePhaseIncrement(pitch_);
        
        // Dispatch to appropriate DSP algorithm
        dsp_dispatcher_->Process(shape_, pitch_, params, phase, phase_increment, sync, buffer, size);
        
        // Check if buffer was actually processed (not silence)
        bool has_signal = false;
        for (size_t i = 0; i < std::min(size_t(8), size); ++i) {
            if (buffer[i] != 0) {
                has_signal = true;
                break;
            }
        }
        printf("DSP result: %s (samples: %d,%d,%d,%d)\n", 
               has_signal ? "HAS SIGNAL" : "SILENCE", 
               size > 0 ? buffer[0] : 0,
               size > 1 ? buffer[1] : 0,
               size > 2 ? buffer[2] : 0,
               size > 3 ? buffer[3] : 0);
        
        // If DSP dispatcher returned silence, fall back to digital oscillator
        if (!has_signal && digital_oscillator_) {
            printf("DSP dispatcher returned silence, trying digital oscillator fallback\n");
            
            // Use original Braids approach: map macro shape to digital shape
            // TRIPLE_RING_MOD is the first digital algorithm (shape 13)
            int digital_shape_index = static_cast<int>(shape_) - static_cast<int>(MacroOscillatorShape::TRIPLE_RING_MOD);
            
            if (digital_shape_index >= 0) {
                // Call digital oscillator with mapped shape
                digital_oscillator_->Render(shape_, pitch_, params[0], params[1], sync, buffer, size);
                
                // Check if digital oscillator produced sound
                has_signal = false;
                for (size_t i = 0; i < std::min(size_t(8), size); ++i) {
                    if (buffer[i] != 0) {
                        has_signal = true;
                        break;
                    }
                }
                printf("Digital oscillator result: %s (samples: %d,%d,%d,%d)\n", 
                       has_signal ? "HAS SIGNAL" : "SILENCE",
                       size > 0 ? buffer[0] : 0,
                       size > 1 ? buffer[1] : 0,
                       size > 2 ? buffer[2] : 0,
                       size > 3 ? buffer[3] : 0);
            }
        }
        
    } else if (digital_oscillator_) {
        printf("Using legacy digital oscillator for shape %d\n", static_cast<int>(shape_));
        
        // Direct fallback to old implementation (like original Braids)
        digital_oscillator_->Render(shape_, pitch_, params[0], params[1], sync, buffer, size);
        
    } else {
        printf("ERROR: Both DSP dispatcher and digital oscillator are NULL! Falling back to CSaw\n");
        
        // Last resort fallback - this should never happen in normal operation
        RenderCSaw(sync, buffer, size);
    }
}

void MacroOscillator::ConfigureTriple(AnalogOscillatorShape shape) {
    for (int i = 0; i < 3; ++i) {
        analog_oscillator_[i].set_shape(shape);
    }
}

// Wavetable synthesis implementations

void MacroOscillator::RenderWavetables(const uint8_t* sync, int16_t* buffer, size_t size) {
    printf("*** MacroOscillator::RenderWavetables called! ***\n");
    
    // Update wavetable phase increment based on pitch
    wavetable_phase_increment_ = ComputePhaseIncrement(pitch_);
    
    // Use parameters to control wavetable selection and morphing
    uint16_t table_param = static_cast<uint16_t>(parameter_interpolation_.Read(0));
    uint16_t morph_param = static_cast<uint16_t>(parameter_interpolation_.Read(1));
    
    printf("  pitch_=%d, table_param=%u, morph_param=%u\n", pitch_, table_param, morph_param);
    printf("  phase_increment=0x%08X\n", wavetable_phase_increment_);
    printf("  dsp_dispatcher_=%p\n", dsp_dispatcher_);
    
    // Route through DSP dispatcher
    if (dsp_dispatcher_) {
        int16_t params[2] = { static_cast<int16_t>(table_param), static_cast<int16_t>(morph_param) };
        dsp_dispatcher_->Process(MacroOscillatorShape::WAVETABLES, pitch_, params, wavetable_phase_, wavetable_phase_increment_, sync, buffer, size);
    } else {
        printf("  ERROR: dsp_dispatcher_ is null!\n");
        // Clear buffer as fallback
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = 0;
        }
    }
}

void MacroOscillator::RenderWaveMap(const uint8_t* sync, int16_t* buffer, size_t size) {
    printf("*** MacroOscillator::RenderWaveMap called! ***\n");
    
    // Update wavetable phase increment based on pitch
    wavetable_phase_increment_ = ComputePhaseIncrement(pitch_);
    
    // Use parameters as X/Y coordinates in 2D wavetable space
    uint16_t x_param = static_cast<uint16_t>(parameter_interpolation_.Read(0));
    uint16_t y_param = static_cast<uint16_t>(parameter_interpolation_.Read(1));
    
    printf("  pitch_=%d, x_param=%u, y_param=%u\n", pitch_, x_param, y_param);
    printf("  dsp_dispatcher_=%p\n", dsp_dispatcher_);
    
    // Route through DSP dispatcher
    if (dsp_dispatcher_) {
        int16_t params[2] = { static_cast<int16_t>(x_param), static_cast<int16_t>(y_param) };
        dsp_dispatcher_->Process(MacroOscillatorShape::WAVE_MAP, pitch_, params, wavetable_phase_, wavetable_phase_increment_, sync, buffer, size);
    } else {
        printf("  ERROR: dsp_dispatcher_ is null!\n");
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = 0;
        }
    }
}

void MacroOscillator::RenderWaveLine(const uint8_t* sync, int16_t* buffer, size_t size) {
    printf("*** MacroOscillator::RenderWaveLine called! ***\n");
    
    // Update wavetable phase increment based on pitch
    wavetable_phase_increment_ = ComputePhaseIncrement(pitch_);
    
    // Parameter 1 controls sweep position, Parameter 2 adds variation
    uint16_t sweep_param = static_cast<uint16_t>(parameter_interpolation_.Read(0));
    uint16_t variation_param = static_cast<uint16_t>(parameter_interpolation_.Read(1));
    
    printf("  pitch_=%d, sweep_param=%u, variation_param=%u\n", pitch_, sweep_param, variation_param);
    printf("  dsp_dispatcher_=%p\n", dsp_dispatcher_);
    
    // Route through DSP dispatcher
    if (dsp_dispatcher_) {
        int16_t params[2] = { static_cast<int16_t>(sweep_param), static_cast<int16_t>(variation_param) };
        dsp_dispatcher_->Process(MacroOscillatorShape::WAVE_LINE, pitch_, params, wavetable_phase_, wavetable_phase_increment_, sync, buffer, size);
    } else {
        printf("  ERROR: dsp_dispatcher_ is null!\n");
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = 0;
        }
    }
}

void MacroOscillator::RenderWaveParaphonic(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Update base phase increment
    uint32_t base_increment = ComputePhaseIncrement(pitch_);
    
    // Setup paraphonic voices with slight detuning
    uint16_t spread_param = static_cast<uint16_t>(parameter_interpolation_.Read(0));
    uint16_t table_param = static_cast<uint16_t>(parameter_interpolation_.Read(1));
    
    // Calculate detuning for each voice
    const int num_voices = 4;  // Use 4 voices for paraphonic mode
    for (int i = 0; i < num_voices; ++i) {
        // Apply slight detuning based on spread parameter
        int32_t detune = ((i - num_voices/2) * spread_param) >> 8;
        paraphonic_increments_[i] = base_increment + detune;
    }
    
    // Route through DSP dispatcher
    if (dsp_dispatcher_) {
        int16_t params[2] = { static_cast<int16_t>(spread_param), static_cast<int16_t>(table_param) };
        dsp_dispatcher_->Process(shape_, pitch_, params, wavetable_phase_, wavetable_phase_increment_, sync, buffer, size);
    }
}

}  // namespace braidy