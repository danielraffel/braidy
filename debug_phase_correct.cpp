#include <iostream>
#include <cstdint>
#include <cmath>

// Constants from BraidyTypes.h (corrected)
constexpr int16_t kPitchC4 = 60 << 7;   // MIDI note 60 (C4) in 7-bit fractional format
constexpr int16_t kOctave = 12 << 7;    // One octave in pitch units
constexpr uint16_t kHighestNote = 128 * 128;      // 16384 - highest valid note  
constexpr uint16_t kPitchTableStart = 128 * 128;  // 16384 - start of LUT range
constexpr int kSampleRate = 48000;

// Full oscillator increments table 
constexpr uint32_t LUT_OSCILLATOR_INCREMENTS[97] = {
  594573364, 598881888, 603221633, 607592826,
  611995694, 616430467, 620897376, 625396654,
  629928536, 634493258, 639091058, 643722175,
  648386851, 653085330, 657817855, 662584675,
  667386036, 672222191, 677093390, 681999888,
  686941940, 691919804, 696933740, 701984010,
  707070875, 712194602, 717355458, 722553711,
  727789633, 733063497, 738375577, 743726151,
  749115497, 754543897, 760011633, 765518991,
  771066257, 776653721, 782281674, 787950409,
  793660223, 799411412, 805204277, 811039119,
  816916243, 822835954, 828798563, 834804379,
  840853716, 846946888, 853084215, 859266014,
  865492610, 871764326, 878081490, 884444431,
  890853479, 897308971, 903811242, 910360631,
  916957479, 923602131, 930294933, 937036233,
  943826384, 950665739, 957554655, 964493491,
  971482608, 978522372, 985613148, 992755307,
  999949221, 1007195266, 1014493818, 1021845258,
  1029249970, 1036708340, 1044220756, 1051787610,
  1059409296, 1067086213, 1074818759, 1082607339,
  1090452358, 1098354226, 1106313353, 1114330156,
  1122405051, 1130538461, 1138730809, 1146982522,
  1155294030, 1163665767, 1172098168, 1180591675,
  1189146729,
};

uint32_t ComputePhaseIncrement(int16_t midi_pitch) {
    std::cout << "Input midi_pitch: " << midi_pitch << " (MIDI note " << (midi_pitch >> 7) << ")" << std::endl;
    
    if (midi_pitch >= kHighestNote) {
        midi_pitch = kHighestNote - 1;
        std::cout << "Clamped to: " << midi_pitch << std::endl;
    }
    
    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;
    std::cout << "ref_pitch after subtracting start (" << kPitchTableStart << "): " << ref_pitch << std::endl;
    
    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
        std::cout << "After octave shift " << num_shifts << ": ref_pitch=" << ref_pitch << std::endl;
    }
    
    int index = ref_pitch >> 4;
    std::cout << "Final ref_pitch: " << ref_pitch << ", index: " << index << std::endl;
    
    if (index >= 96) {
        std::cout << "ERROR: Index " << index << " out of range (max 95)!" << std::endl;
        return 0;
    }
    
    uint32_t a = LUT_OSCILLATOR_INCREMENTS[index];
    uint32_t b = LUT_OSCILLATOR_INCREMENTS[index + 1];
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
    std::cout << "=== Phase Increment Debug (Corrected Constants) ===" << std::endl;
    std::cout << "kPitchC4 = " << kPitchC4 << " (MIDI note " << (kPitchC4 >> 7) << ")" << std::endl;
    std::cout << "kPitchTableStart = " << kPitchTableStart << " (MIDI note " << (kPitchTableStart >> 7) << ")" << std::endl;
    std::cout << "kOctave = " << kOctave << " (12 semitones)" << std::endl;
    std::cout << "Sample rate = " << kSampleRate << " Hz" << std::endl;
    std::cout << std::endl;
    
    // Test C4 first
    std::cout << "=== Testing MIDI note 60 (C4) - Expected 261.63 Hz ===" << std::endl;
    uint32_t c4_inc = ComputePhaseIncrement(kPitchC4);
    
    std::cout << std::endl;
    std::cout << "=== Testing other notes ===" << std::endl;
    
    // Test a few different notes
    int test_notes[] = {48, 60, 72, 84}; // C3, C4, C5, C6
    for (int note : test_notes) {
        std::cout << "\n--- MIDI note " << note << " ---" << std::endl;
        int16_t pitch = note << 7;
        uint32_t inc = ComputePhaseIncrement(pitch);
        if (inc > 0) {
            double freq = (static_cast<double>(inc) * kSampleRate) / 4294967296.0;
            double expected = 440.0 * std::pow(2.0, (note - 69) / 12.0);
            double error = std::abs(freq - expected) / expected * 100.0;
            std::cout << "Result: " << freq << " Hz, Expected: " << expected << " Hz (error: " << error << "%)" << std::endl;
        }
    }
    
    return 0;
}