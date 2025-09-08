#include <iostream>
#include <cstdint>
#include <cmath>

// Constants from BraidyTypes.h
constexpr int16_t kPitchC4 = 60 << 7;   // MIDI note 60 (C4) in 7-bit fractional format
constexpr int16_t kOctave = 12 << 7;    // One octave in pitch units
constexpr uint16_t kHighestNote = 140 * 128;      // 17920 - highest valid note
constexpr uint16_t kPitchTableStart = 116 * 128;  // 14848 - start of LUT range
constexpr int kSampleRate = 48000;

// Truncated oscillator increments table - just first few entries for C4 area
constexpr uint32_t LUT_OSCILLATOR_INCREMENTS[10] = {
    594573364, 598881888, 603221633, 607592826,
    611995694, 616430467, 620897376, 625396654,
    629928536, 634493258
};

uint32_t ComputePhaseIncrement(int16_t midi_pitch) {
    std::cout << "Input midi_pitch: " << midi_pitch << " (0x" << std::hex << midi_pitch << std::dec << ")" << std::endl;
    
    if (midi_pitch >= kHighestNote) {
        midi_pitch = kHighestNote - 1;
        std::cout << "Clamped to: " << midi_pitch << std::endl;
    }
    
    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;
    std::cout << "ref_pitch after subtracting start: " << ref_pitch << std::endl;
    
    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
        std::cout << "After octave shift " << num_shifts << ": ref_pitch=" << ref_pitch << std::endl;
    }
    
    std::cout << "Final ref_pitch: " << ref_pitch << ", index: " << (ref_pitch >> 4) << std::endl;
    
    if ((ref_pitch >> 4) >= 10) {
        std::cout << "ERROR: Index out of range!" << std::endl;
        return 0;
    }
    
    uint32_t a = LUT_OSCILLATOR_INCREMENTS[ref_pitch >> 4];
    uint32_t b = LUT_OSCILLATOR_INCREMENTS[(ref_pitch >> 4) + 1];
    std::cout << "LUT values: a=" << a << ", b=" << b << std::endl;
    
    uint32_t phase_increment = a + 
        (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
    std::cout << "Before shifts: " << phase_increment << " (0x" << std::hex << phase_increment << std::dec << ")" << std::endl;
    
    phase_increment >>= num_shifts;
    std::cout << "After " << num_shifts << " right shifts: " << phase_increment << " (0x" << std::hex << phase_increment << std::dec << ")" << std::endl;
    
    // Convert to frequency for verification
    double frequency = (static_cast<double>(phase_increment) * kSampleRate) / 4294967296.0;
    std::cout << "Calculated frequency: " << frequency << " Hz" << std::endl;
    
    return phase_increment;
}

int main() {
    std::cout << "=== Phase Increment Debug ===" << std::endl;
    std::cout << "kPitchC4 = " << kPitchC4 << " (" << (kPitchC4 >> 7) << " semitones)" << std::endl;
    std::cout << "kPitchTableStart = " << kPitchTableStart << std::endl;
    std::cout << "kOctave = " << kOctave << std::endl;
    std::cout << "Sample rate = " << kSampleRate << " Hz" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Expected C4 frequency: 261.63 Hz" << std::endl;
    std::cout << std::endl;
    
    // Test C4
    uint32_t c4_inc = ComputePhaseIncrement(kPitchC4);
    
    std::cout << std::endl;
    std::cout << "=== Testing a few notes ===" << std::endl;
    
    // Test a few different notes
    int test_notes[] = {48, 60, 72, 84}; // C3, C4, C5, C6
    for (int note : test_notes) {
        std::cout << "\nMIDI note " << note << ": " << std::endl;
        int16_t pitch = note << 7;
        uint32_t inc = ComputePhaseIncrement(pitch);
        double freq = (static_cast<double>(inc) * kSampleRate) / 4294967296.0;
        double expected = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        std::cout << "Calculated: " << freq << " Hz, Expected: " << expected << " Hz" << std::endl;
    }
    
    return 0;
}