#!/usr/bin/env python3
"""
Simple WAV Analysis Tool (No numpy required)
Basic diagnostics for generated WAV files
"""

import struct
import sys
import os
import math

class SimpleWavAnalyzer:
    def __init__(self, filename):
        self.filename = filename
        self.sample_rate = 0
        self.data = []
        self._read_wav()
    
    def _read_wav(self):
        """Read 16-bit mono WAV file"""
        with open(self.filename, 'rb') as f:
            # Check RIFF header
            if f.read(4) != b'RIFF':
                raise ValueError(f"Not a valid WAV file: {self.filename}")
            
            file_size = struct.unpack('<I', f.read(4))[0]
            
            if f.read(4) != b'WAVE':
                raise ValueError(f"Not a valid WAV file: {self.filename}")
            
            # Find fmt chunk
            while True:
                chunk_id = f.read(4)
                if not chunk_id:
                    raise ValueError("fmt chunk not found")
                
                chunk_size = struct.unpack('<I', f.read(4))[0]
                
                if chunk_id == b'fmt ':
                    fmt_data = f.read(chunk_size)
                    audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample = \
                        struct.unpack('<HHIIHH', fmt_data[:16])
                    
                    if audio_format != 1 or channels != 1 or bits_per_sample != 16:
                        raise ValueError("Only 16-bit mono PCM supported")
                    
                    self.sample_rate = sample_rate
                    break
                else:
                    f.seek(chunk_size, 1)
            
            # Find data chunk
            while True:
                chunk_id = f.read(4)
                if not chunk_id:
                    raise ValueError("data chunk not found")
                
                chunk_size = struct.unpack('<I', f.read(4))[0]
                
                if chunk_id == b'data':
                    sample_count = chunk_size // 2
                    raw_data = f.read(chunk_size)
                    self.data = list(struct.unpack(f'<{sample_count}h', raw_data))
                    break
                else:
                    f.seek(chunk_size, 1)
    
    def analyze(self):
        """Perform basic signal analysis"""
        print(f"\n=== WAV FILE ANALYSIS ===")
        print(f"File: {self.filename}")
        print(f"Samples: {len(self.data)}")
        print(f"Sample Rate: {self.sample_rate} Hz")
        print(f"Duration: {len(self.data) / self.sample_rate:.3f} seconds")
        
        if not self.data:
            print("❌ NO DATA: File contains no audio samples")
            return "NO_DATA"
        
        # Basic statistics
        min_val = min(self.data)
        max_val = max(self.data)
        sum_val = sum(self.data)
        dc_offset = sum_val / len(self.data)
        
        print(f"\n=== SIGNAL CHARACTERISTICS ===")
        print(f"Range: {min_val} to {max_val}")
        print(f"DC Offset: {dc_offset:.2f}")
        
        # RMS calculation
        sum_squares = sum(x * x for x in self.data)
        rms = math.sqrt(sum_squares / len(self.data))
        print(f"RMS Level: {rms:.2f}")
        
        if rms > 0:
            rms_db = 20 * math.log10(rms / 32768)
            print(f"RMS Level: {rms_db:.1f} dBFS")
        
        # Peak level
        peak = max(abs(min_val), abs(max_val))
        print(f"Peak Level: {peak}")
        
        if peak > 0:
            peak_db = 20 * math.log10(peak / 32768)
            print(f"Peak Level: {peak_db:.1f} dBFS")
        
        # Variance calculation (spread of values)
        mean = dc_offset
        variance = sum((x - mean) * (x - mean) for x in self.data) / len(self.data)
        print(f"Variance: {variance:.2f}")
        
        # Zero crossing rate (rough frequency estimate)
        zero_crossings = 0
        for i in range(1, len(self.data)):
            if (self.data[i-1] < 0 and self.data[i] >= 0) or (self.data[i-1] >= 0 and self.data[i] < 0):
                zero_crossings += 1
        
        if len(self.data) > 1:
            zcr_hz = (zero_crossings / 2) / (len(self.data) / self.sample_rate)
            print(f"Zero Crossing Rate: {zcr_hz:.1f} Hz")
        
        # Clipping detection
        clipped_samples = sum(1 for x in self.data if abs(x) > 32700)
        clip_percent = (clipped_samples / len(self.data)) * 100
        print(f"Clipping: {clipped_samples} samples ({clip_percent:.3f}%)")
        
        # Show first few samples for pattern detection
        print(f"\n=== FIRST 20 SAMPLES ===")
        first_samples = self.data[:20] if len(self.data) >= 20 else self.data
        for i, sample in enumerate(first_samples):
            print(f"  [{i:2d}]: {sample:6d}")
        
        if len(self.data) > 20:
            print("  ... (more samples)")
        
        # Diagnostic checks
        print(f"\n=== DIAGNOSTIC ===")
        
        # Silence check
        if rms < 1.0:
            print("❌ SILENT: RMS level too low, essentially silence")
            return "SILENT"
        
        # DC/stuck check
        if variance < 1.0:
            print("❌ STUCK: Very low variance, signal appears stuck")
            return "STUCK"
        
        # All same value check
        if len(set(self.data[:1000])) == 1:  # Check first 1000 samples
            print("❌ CONSTANT: All samples have the same value")
            return "CONSTANT"
        
        # Reasonable signal check
        if rms > 1000 and variance > 1000:
            print("✅ REASONABLE: Signal contains audio content")
            
            # Check if frequency is reasonable for MIDI 60 (261.63 Hz)
            if len(self.data) > 1:
                if 200 < zcr_hz < 400:
                    print(f"✅ FREQUENCY: Zero crossing rate ({zcr_hz:.1f} Hz) reasonable for MIDI 60")
                else:
                    print(f"⚠️  FREQUENCY: Zero crossing rate ({zcr_hz:.1f} Hz) unusual for MIDI 60 (expected ~262 Hz)")
            
            return "REASONABLE"
        
        elif rms > 100 and variance > 100:
            print("⚠️  WEAK: Signal present but may have issues")
            return "WEAK"
        
        else:
            print("❌ PROBLEMATIC: Signal has serious issues")
            return "PROBLEMATIC"

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 simple_wav_analyze.py <wav_file>")
        return 1
    
    wav_file = sys.argv[1]
    
    if not os.path.exists(wav_file):
        print(f"Error: File not found: {wav_file}")
        return 1
    
    try:
        analyzer = SimpleWavAnalyzer(wav_file)
        result = analyzer.analyze()
        
        # Return success if signal is reasonable
        return 0 if result == "REASONABLE" else 1
        
    except Exception as e:
        print(f"Error analyzing file: {e}")
        return 1

if __name__ == '__main__':
    sys.exit(main())