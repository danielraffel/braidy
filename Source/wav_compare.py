#!/usr/bin/env python3
"""
WAV Comparison Tool for Braidy Parity Testing
Analyzes differences between reference and candidate WAV files
"""

import numpy as np
import argparse
import struct
import sys
import os
from pathlib import Path

class WavReader:
    """Simple WAV file reader for 16-bit mono files"""
    
    def __init__(self, filename):
        self.filename = filename
        self.sample_rate = 0
        self.data = None
        self._read_wav()
    
    def _read_wav(self):
        with open(self.filename, 'rb') as f:
            # Read WAV header
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
                    
                    if audio_format != 1:
                        raise ValueError("Only PCM format supported")
                    if channels != 1:
                        raise ValueError("Only mono files supported")
                    if bits_per_sample != 16:
                        raise ValueError("Only 16-bit samples supported")
                    
                    self.sample_rate = sample_rate
                    break
                else:
                    f.seek(chunk_size, 1)  # Skip chunk
            
            # Find data chunk
            while True:
                chunk_id = f.read(4)
                if not chunk_id:
                    raise ValueError("data chunk not found")
                
                chunk_size = struct.unpack('<I', f.read(4))[0]
                
                if chunk_id == b'data':
                    sample_count = chunk_size // 2
                    raw_data = f.read(chunk_size)
                    self.data = np.array(struct.unpack(f'<{sample_count}h', raw_data), dtype=np.int16)
                    break
                else:
                    f.seek(chunk_size, 1)  # Skip chunk

class WavComparer:
    def __init__(self, reference_file=None, candidate_file=None):
        self.reference_file = reference_file
        self.candidate_file = candidate_file
        self.ref_data = None
        self.cand_data = None
        self.ref_rate = None
        self.cand_rate = None
    
    def load_files(self, reference_file, candidate_file):
        """Load reference and candidate files"""
        print(f"Loading reference: {reference_file}")
        if not os.path.exists(reference_file):
            raise FileNotFoundError(f"Reference file not found: {reference_file}")
        
        print(f"Loading candidate: {candidate_file}")
        if not os.path.exists(candidate_file):
            raise FileNotFoundError(f"Candidate file not found: {candidate_file}")
        
        ref_wav = WavReader(reference_file)
        cand_wav = WavReader(candidate_file)
        
        self.ref_data = ref_wav.data.astype(np.float64)
        self.cand_data = cand_wav.data.astype(np.float64)
        self.ref_rate = ref_wav.sample_rate
        self.cand_rate = cand_wav.sample_rate
        
        print(f"Reference: {len(self.ref_data)} samples at {self.ref_rate}Hz")
        print(f"Candidate: {len(self.cand_data)} samples at {self.cand_rate}Hz")
    
    def analyze_candidate_only(self, candidate_file):
        """Analyze a single candidate file (no reference)"""
        print(f"\n=== CANDIDATE-ONLY ANALYSIS ===")
        
        cand_wav = WavReader(candidate_file)
        self.cand_data = cand_wav.data.astype(np.float64)
        self.cand_rate = cand_wav.sample_rate
        
        print(f"File: {candidate_file}")
        print(f"Samples: {len(self.cand_data)}")
        print(f"Sample Rate: {self.cand_rate} Hz")
        print(f"Duration: {len(self.cand_data) / self.cand_rate:.2f} seconds")
        
        # Basic signal analysis
        rms = np.sqrt(np.mean(self.cand_data ** 2))
        peak = np.max(np.abs(self.cand_data))
        dc_offset = np.mean(self.cand_data)
        
        print(f"\n=== SIGNAL CHARACTERISTICS ===")
        print(f"RMS Level: {rms:.2f} ({20*np.log10(rms/32768):.1f} dBFS)")
        print(f"Peak Level: {peak:.2f} ({20*np.log10(peak/32768):.1f} dBFS)")
        print(f"DC Offset: {dc_offset:.2f}")
        
        # Check for common problems
        print(f"\n=== DIAGNOSTIC ===")
        
        # Silence detection
        if rms < 1.0:
            print("❌ SILENT: RMS level too low, probably silence")
            return "SILENT"
        
        # DC/stuck detection
        variance = np.var(self.cand_data)
        if variance < 1.0:
            print("❌ STUCK: Very low variance, signal appears stuck")
            return "STUCK"
        
        # Clipping detection
        clip_samples = np.sum(np.abs(self.cand_data) > 32700)
        clip_percent = (clip_samples / len(self.cand_data)) * 100
        print(f"Clipping: {clip_samples} samples ({clip_percent:.2f}%)")
        if clip_percent > 1.0:
            print("⚠️  HIGH CLIPPING detected")
        
        # Zero crossing rate (proxy for frequency content)
        zero_crossings = np.sum(np.diff(np.signbit(self.cand_data)))
        zcr = zero_crossings / len(self.cand_data) * self.cand_rate / 2
        print(f"Zero Crossing Rate: {zcr:.1f} Hz (approx fundamental frequency)")
        
        # Frequency analysis
        if len(self.cand_data) >= 1024:
            # Simple frequency analysis
            fft = np.fft.fft(self.cand_data[:1024])
            freqs = np.fft.fftfreq(1024, 1/self.cand_rate)
            magnitude = np.abs(fft)
            
            # Find peak frequency
            peak_idx = np.argmax(magnitude[1:len(magnitude)//2]) + 1  # Skip DC
            peak_freq = abs(freqs[peak_idx])
            print(f"Dominant Frequency: {peak_freq:.1f} Hz")
            
            # Expected frequency for MIDI 60 is 261.63 Hz
            if abs(peak_freq - 261.63) < 10:
                print("✅ Frequency close to expected 261.63 Hz (MIDI 60)")
            else:
                print(f"⚠️  Frequency differs from expected 261.63 Hz")
        
        # Overall verdict for single file
        if rms > 1000 and variance > 1000 and zcr > 100:
            print("\n✅ REASONABLE: Signal appears to contain audio content")
            return "REASONABLE"
        elif rms > 100 and variance > 100:
            print("\n⚠️  WEAK: Signal present but may have issues")
            return "WEAK"  
        else:
            print("\n❌ PROBLEMATIC: Signal has serious issues")
            return "PROBLEMATIC"
    
    def align_signals(self):
        """Align signals using cross-correlation"""
        if self.ref_data is None or self.cand_data is None:
            raise ValueError("Files not loaded")
        
        if self.ref_rate != self.cand_rate:
            raise ValueError(f"Sample rate mismatch: {self.ref_rate} vs {self.cand_rate}")
        
        # Cross-correlation for alignment
        correlation = np.correlate(self.ref_data, self.cand_data, mode='full')
        offset = np.argmax(correlation) - (len(self.cand_data) - 1)
        
        # Apply offset
        if offset > 0:
            aligned_ref = self.ref_data[offset:]
            aligned_cand = self.cand_data
        elif offset < 0:
            aligned_ref = self.ref_data
            aligned_cand = self.cand_data[-offset:]
        else:
            aligned_ref = self.ref_data
            aligned_cand = self.cand_data
        
        # Trim to same length
        min_len = min(len(aligned_ref), len(aligned_cand))
        return aligned_ref[:min_len], aligned_cand[:min_len], offset
    
    def compute_metrics(self, ref, cand):
        """Compute comparison metrics"""
        # Basic metrics
        rms_error = np.sqrt(np.mean((ref - cand) ** 2))
        peak_error = np.max(np.abs(ref - cand))
        
        # SNR calculation
        signal_power = np.mean(ref ** 2)
        noise_power = np.mean((ref - cand) ** 2)
        if noise_power > 0:
            snr_db = 10 * np.log10(signal_power / noise_power)
        else:
            snr_db = float('inf')
        
        # Correlation
        if len(ref) > 1:
            correlation = np.corrcoef(ref, cand)[0, 1] if np.std(ref) > 0 and np.std(cand) > 0 else 0
        else:
            correlation = 0
        
        return {
            'rms_error': rms_error,
            'peak_error': peak_error,
            'snr_db': snr_db,
            'correlation': correlation
        }
    
    def analyze(self):
        """Full comparison analysis"""
        if self.ref_data is None or self.cand_data is None:
            raise ValueError("Files not loaded")
        
        print(f"\n=== WAV COMPARISON ANALYSIS ===")
        
        # Align signals
        ref_aligned, cand_aligned, offset = self.align_signals()
        print(f"Alignment offset: {offset} samples ({offset/self.ref_rate*1000:.1f} ms)")
        
        # Compute metrics
        metrics = self.compute_metrics(ref_aligned, cand_aligned)
        
        print(f"\n=== METRICS ===")
        print(f"RMS Error: {metrics['rms_error']:.2f} LSB")
        print(f"Peak Error: {metrics['peak_error']:.2f} LSB") 
        print(f"SNR: {metrics['snr_db']:.2f} dB")
        print(f"Correlation: {metrics['correlation']:.6f}")
        
        # Verdict
        print(f"\n=== VERDICT ===")
        peak_lsb = metrics['peak_error']
        snr = metrics['snr_db']
        corr = metrics['correlation']
        
        if peak_lsb < 2.0 and snr > 80:
            print("✅ EXCELLENT: Bit-accurate match")
            verdict = "EXCELLENT"
        elif peak_lsb < 10.0 and snr > 60:
            print("✅ GOOD: Perceptually transparent")
            verdict = "GOOD"
        elif peak_lsb < 100.0 and snr > 40:
            print("⚠️  ACCEPTABLE: Audible differences but usable")
            verdict = "ACCEPTABLE"
        elif corr > 0.8:
            print("⚠️  POOR: Same waveform but wrong amplitude/offset")
            verdict = "POOR"
        else:
            print("❌ BROKEN: Completely different signals")
            verdict = "BROKEN"
        
        return verdict, metrics

def main():
    parser = argparse.ArgumentParser(description='Compare WAV files for audio parity testing')
    parser.add_argument('candidate', help='Candidate WAV file to analyze')
    parser.add_argument('reference', nargs='?', help='Reference WAV file (optional)')
    parser.add_argument('--save-aligned', action='store_true', help='Save aligned versions')
    
    args = parser.parse_args()
    
    comparer = WavComparer()
    
    if args.reference:
        # Full comparison mode
        try:
            comparer.load_files(args.reference, args.candidate)
            verdict, metrics = comparer.analyze()
            
            if args.save_aligned:
                ref_aligned, cand_aligned, _ = comparer.align_signals()
                # Would save aligned files here
                print("Note: --save-aligned not yet implemented")
            
            return 0 if verdict in ['EXCELLENT', 'GOOD'] else 1
            
        except Exception as e:
            print(f"Error: {e}")
            return 1
    else:
        # Single file analysis mode
        try:
            verdict = comparer.analyze_candidate_only(args.candidate)
            return 0 if verdict in ['REASONABLE'] else 1
            
        except Exception as e:
            print(f"Error: {e}")
            return 1

if __name__ == '__main__':
    sys.exit(main())