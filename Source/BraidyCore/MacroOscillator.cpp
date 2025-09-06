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

void MacroOscillator::Render(const uint8_t* sync_buffer, int16_t* buffer, size_t size) {
    if (size > kBlockSize) {
        size = kBlockSize;  // Ensure we don't overflow our temp buffers
    }
    
    UpdateParameters();
    
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
    
    // Dispatch to appropriate rendering function
    int shape_index = static_cast<int>(shape_);
    if (shape_index >= 0 && shape_index < static_cast<int>(MacroOscillatorShape::LAST)) {
        (this->*fn_table_[shape_index])(sync_buffer, buffer, size);
    } else {
        // Fallback to CSAW
        RenderCSaw(sync_buffer, buffer, size);
    }
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
    // Morph between different waveforms based on parameter
    int16_t morph = parameter_interpolation_.Read(0);
    
    if (morph < 10922) {  // First third: Saw to Square
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        // Crossfade between saw and square
        uint16_t fade = static_cast<uint16_t>((morph * 6) & 0xFFFF);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
        
    } else if (morph < 21845) {  // Second third: Square to Triangle
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::SQUARE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::TRIANGLE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        uint16_t fade = static_cast<uint16_t>(((morph - 10922) * 6) & 0xFFFF);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
        
    } else {  // Final third: Triangle to Sine
        analog_oscillator_[0].set_shape(AnalogOscillatorShape::TRIANGLE);
        analog_oscillator_[1].set_shape(AnalogOscillatorShape::SINE);
        
        analog_oscillator_[0].set_pitch(pitch_);
        analog_oscillator_[1].set_pitch(pitch_);
        
        analog_oscillator_[0].Render(sync, buffer, nullptr, size);
        analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
        
        uint16_t fade = static_cast<uint16_t>(((morph - 21845) * 6) & 0xFFFF);
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = Crossfade(buffer[i], temp_buffer_[i], fade);
        }
    }
}

void MacroOscillator::RenderSawSquare(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Morph between saw and square based on first parameter
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::SAW);
    analog_oscillator_[1].set_shape(AnalogOscillatorShape::SQUARE);
    
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[1].set_pitch(pitch_);
    analog_oscillator_[1].set_parameter(parameter_interpolation_.Read(1));  // Pulse width
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
    
    // Crossfade based on first parameter
    uint16_t balance = static_cast<uint16_t>(parameter_interpolation_.Read(0) * 2);
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
    }
}

void MacroOscillator::RenderSineTriangle(const uint8_t* sync, int16_t* buffer, size_t size) {
    // Morph between sine and triangle
    analog_oscillator_[0].set_shape(AnalogOscillatorShape::SINE);
    analog_oscillator_[1].set_shape(AnalogOscillatorShape::TRIANGLE);
    
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[1].set_pitch(pitch_);
    
    analog_oscillator_[0].set_parameter(parameter_interpolation_.Read(1));  // Harmonic content
    
    analog_oscillator_[0].Render(sync, buffer, nullptr, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, nullptr, size);
    
    // Crossfade
    uint16_t balance = static_cast<uint16_t>(parameter_interpolation_.Read(0) * 2);
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = Crossfade(buffer[i], temp_buffer_[i], balance);
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