// Formant Tables for Vowel Synthesis
// Based on acoustic phonetics research and Braids original implementation

#pragma once

#include <cstdint>

namespace braidy {

// Formant frequencies in Hz for different vowels
// F1, F2, F3, F4, F5 for each vowel
struct FormantData {
    uint16_t f1;  // First formant frequency
    uint16_t f2;  // Second formant frequency
    uint16_t f3;  // Third formant frequency
    uint16_t f4;  // Fourth formant frequency
    uint16_t f5;  // Fifth formant frequency
    uint8_t amp1; // Amplitude for F1 (0-255)
    uint8_t amp2; // Amplitude for F2
    uint8_t amp3; // Amplitude for F3
    uint8_t amp4; // Amplitude for F4
    uint8_t amp5; // Amplitude for F5
};

// Vowel formants based on male vocal tract
const FormantData kMaleVowelFormants[] = {
    // A as in "father" [ɑ]
    { 700, 1100, 2500, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // E as in "bed" [ɛ]
    { 530, 1850, 2500, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // I as in "beat" [i]
    { 270, 2300, 3000, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // O as in "boat" [o]
    { 570, 840, 2400, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // U as in "boot" [u]
    { 300, 870, 2250, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // AE as in "cat" [æ]
    { 660, 1720, 2400, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // AH as in "but" [ʌ]
    { 520, 1190, 2390, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // AW as in "bought" [ɔ]
    { 590, 920, 2710, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // EH as in "bet" [e]
    { 400, 2000, 2550, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // ER as in "bird" [ɝ]
    { 490, 1350, 1700, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // IH as in "bit" [ɪ]
    { 400, 1900, 2550, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // IY as in "beet" [i]
    { 270, 2290, 3010, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // OW as in "boat" [oʊ]
    { 570, 840, 2410, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // OY as in "boy" [ɔɪ]
    { 590, 920, 2710, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // UH as in "book" [ʊ]
    { 440, 1020, 2240, 3500, 4500, 255, 200, 150, 100, 50 },
    
    // UW as in "boot" [u]
    { 300, 870, 2240, 3500, 4500, 255, 200, 150, 100, 50 }
};

// Female vowel formants (higher frequencies)
const FormantData kFemaleVowelFormants[] = {
    // A [ɑ]
    { 850, 1200, 2800, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // E [ɛ]
    { 610, 2330, 2990, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // I [i]
    { 310, 2790, 3310, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // O [o]
    { 590, 920, 2710, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // U [u]
    { 370, 950, 2670, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // AE [æ]
    { 860, 2050, 2850, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // AH [ʌ]
    { 590, 1220, 2810, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // AW [ɔ]
    { 710, 1100, 2540, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // EH [e]
    { 470, 2520, 3010, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // ER [ɝ]
    { 560, 1480, 1800, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // IH [ɪ]
    { 430, 2480, 3070, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // IY [i]
    { 310, 2790, 3310, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // OW [oʊ]
    { 590, 920, 2710, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // OY [ɔɪ]
    { 710, 1100, 2540, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // UH [ʊ]
    { 470, 1160, 2680, 3800, 4800, 255, 200, 150, 100, 50 },
    
    // UW [u]
    { 370, 950, 2670, 3800, 4800, 255, 200, 150, 100, 50 }
};

// Consonant articulation parameters
struct ConsonantData {
    uint16_t noise_freq;    // Noise center frequency
    uint16_t noise_bw;      // Noise bandwidth
    uint8_t noise_amp;      // Noise amplitude
    uint8_t voiced_amp;     // Voiced component amplitude
    uint16_t burst_duration; // Duration of burst in samples
};

// Consonant articulation data
const ConsonantData kConsonantData[] = {
    // B - voiced bilabial plosive
    { 200, 100, 50, 200, 100 },
    
    // D - voiced alveolar plosive
    { 4000, 500, 80, 180, 80 },
    
    // F - voiceless labiodental fricative
    { 8000, 3000, 200, 0, 0 },
    
    // G - voiced velar plosive
    { 2000, 300, 60, 190, 90 },
    
    // H - voiceless glottal fricative
    { 1500, 5000, 150, 50, 0 },
    
    // K - voiceless velar plosive
    { 2500, 400, 180, 0, 120 },
    
    // L - voiced alveolar lateral
    { 300, 200, 20, 230, 0 },
    
    // M - voiced bilabial nasal
    { 250, 150, 30, 220, 0 },
    
    // N - voiced alveolar nasal
    { 300, 200, 30, 220, 0 },
    
    // P - voiceless bilabial plosive
    { 500, 200, 160, 0, 100 },
    
    // R - voiced alveolar approximant
    { 1300, 500, 40, 210, 0 },
    
    // S - voiceless alveolar fricative
    { 6500, 2000, 220, 0, 0 },
    
    // T - voiceless alveolar plosive
    { 4500, 600, 190, 0, 80 },
    
    // V - voiced labiodental fricative
    { 8000, 3000, 100, 150, 0 },
    
    // W - voiced labial-velar approximant
    { 300, 200, 20, 230, 0 },
    
    // Z - voiced alveolar fricative
    { 6500, 2000, 120, 150, 0 }
};

// Formant transition table for smooth morphing
class FormantTable {
public:
    // Get interpolated formant data between two vowels
    static FormantData Interpolate(const FormantData& a, const FormantData& b, float mix) {
        FormantData result;
        
        // Linear interpolation of frequencies
        result.f1 = static_cast<uint16_t>(a.f1 + (b.f1 - a.f1) * mix);
        result.f2 = static_cast<uint16_t>(a.f2 + (b.f2 - a.f2) * mix);
        result.f3 = static_cast<uint16_t>(a.f3 + (b.f3 - a.f3) * mix);
        result.f4 = static_cast<uint16_t>(a.f4 + (b.f4 - a.f4) * mix);
        result.f5 = static_cast<uint16_t>(a.f5 + (b.f5 - a.f5) * mix);
        
        // Linear interpolation of amplitudes
        result.amp1 = static_cast<uint8_t>(a.amp1 + (b.amp1 - a.amp1) * mix);
        result.amp2 = static_cast<uint8_t>(a.amp2 + (b.amp2 - a.amp2) * mix);
        result.amp3 = static_cast<uint8_t>(a.amp3 + (b.amp3 - a.amp3) * mix);
        result.amp4 = static_cast<uint8_t>(a.amp4 + (b.amp4 - a.amp4) * mix);
        result.amp5 = static_cast<uint8_t>(a.amp5 + (b.amp5 - a.amp5) * mix);
        
        return result;
    }
    
    // Get formant data for a specific vowel index with morphing
    static FormantData GetVowel(uint16_t index, bool female = false) {
        const FormantData* table = female ? kFemaleVowelFormants : kMaleVowelFormants;
        const int table_size = 16;
        
        // Calculate integer and fractional parts
        int vowel_int = (index >> 12) % table_size;  // Top 4 bits
        float vowel_frac = (index & 0xFFF) / 4096.0f;  // Bottom 12 bits
        
        // Get adjacent vowels
        int next_vowel = (vowel_int + 1) % table_size;
        
        // Interpolate between adjacent vowels
        return Interpolate(table[vowel_int], table[next_vowel], vowel_frac);
    }
    
    // Apply consonant articulation to formant data
    static void ApplyConsonant(FormantData& vowel, const ConsonantData& consonant, float strength) {
        // Modify formants based on consonant articulation
        vowel.f1 = static_cast<uint16_t>(vowel.f1 * (1.0f - strength * 0.3f));
        vowel.f2 = static_cast<uint16_t>(vowel.f2 * (1.0f + strength * 0.2f));
        
        // Reduce voiced amplitude for voiceless consonants
        if (consonant.voiced_amp == 0) {
            vowel.amp1 = static_cast<uint8_t>(vowel.amp1 * (1.0f - strength));
            vowel.amp2 = static_cast<uint8_t>(vowel.amp2 * (1.0f - strength));
        }
    }
};

} // namespace braidy