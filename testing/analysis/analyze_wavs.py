#!/usr/bin/env python3
"""
WAV File Analysis Tool for Braidy Testing
Analyzes WAV files from both JUCE and Braids implementations
"""

import sys
import struct
import glob
import os

def read_wav_file(filename):
    """Read a WAV file and return sample data"""
    try:
        with open(filename, 'rb') as f:
            # Read RIFF header
            riff = f.read(4)
            if riff != b'RIFF':
                return None, None, "Not a WAV file"
            
            file_size = struct.unpack('<I', f.read(4))[0]
            wave = f.read(4)
            if wave != b'WAVE':
                return None, None, "Not a WAV file"
            
            # Read fmt chunk
            fmt = f.read(4)
            if fmt != b'fmt ':
                return None, None, "fmt chunk not found"
            
            fmt_size = struct.unpack('<I', f.read(4))[0]
            audio_format = struct.unpack('<H', f.read(2))[0]
            num_channels = struct.unpack('<H', f.read(2))[0]
            sample_rate = struct.unpack('<I', f.read(4))[0]
            byte_rate = struct.unpack('<I', f.read(4))[0]
            block_align = struct.unpack('<H', f.read(2))[0]
            bits_per_sample = struct.unpack('<H', f.read(2))[0]
            
            # Skip any extra format bytes
            if fmt_size > 16:
                f.read(fmt_size - 16)
            
            # Find data chunk
            while True:
                chunk = f.read(4)
                if len(chunk) < 4:
                    return None, None, "data chunk not found"
                
                chunk_size = struct.unpack('<I', f.read(4))[0]
                
                if chunk == b'data':
                    # Read sample data
                    num_samples = chunk_size // 2  # 16-bit samples
                    samples = []
                    for _ in range(num_samples):
                        sample_bytes = f.read(2)
                        if len(sample_bytes) < 2:
                            break
                        sample = struct.unpack('<h', sample_bytes)[0]
                        samples.append(sample)
                    return samples, sample_rate, None
                else:
                    # Skip this chunk
                    f.seek(chunk_size, 1)
                    
    except Exception as e:
        return None, None, str(e)

def analyze_wav(filename):
    """Analyze a WAV file and return metrics"""
    samples, sample_rate, error = read_wav_file(filename)
    
    if error:
        return {'error': error}
    
    if not samples:
        return {'error': 'No samples found'}
    
    # Basic statistics
    num_samples = len(samples)
    duration = num_samples / sample_rate if sample_rate else 0
    
    # Find min/max
    min_val = min(samples) if samples else 0
    max_val = max(samples) if samples else 0
    
    # DC offset
    dc_offset = sum(samples) / len(samples) if samples else 0
    
    # RMS
    rms = 0
    if samples:
        sum_squares = sum(s * s for s in samples)
        rms = (sum_squares / len(samples)) ** 0.5
    
    # Peak level
    peak = max(abs(min_val), abs(max_val))
    
    # Zero crossings
    zero_crossings = 0
    for i in range(1, len(samples)):
        if (samples[i-1] < 0 and samples[i] >= 0) or (samples[i-1] >= 0 and samples[i] < 0):
            zero_crossings += 1
    
    # Frequency estimate
    freq_estimate = (zero_crossings / 2) / duration if duration > 0 else 0
    
    # Clipping detection
    clipping_count = sum(1 for s in samples if abs(s) >= 32767)
    clipping_percent = (clipping_count * 100.0 / num_samples) if num_samples else 0
    
    # Signal detection
    has_signal = any(abs(s) > 100 for s in samples)
    
    return {
        'num_samples': num_samples,
        'duration': duration,
        'dc_offset': dc_offset,
        'rms': rms,
        'peak': peak,
        'freq_estimate': freq_estimate,
        'clipping_percent': clipping_percent,
        'has_signal': has_signal,
        'min_val': min_val,
        'max_val': max_val
    }

def analyze_directory(directory, label):
    """Analyze all WAV files in a directory"""
    wav_files = sorted(glob.glob(f'{directory}/*.wav'))
    
    if not wav_files:
        print(f"No WAV files found in {directory}/")
        return []
    
    print(f"\n{'=' * 60}")
    print(f"{label} ANALYSIS ({len(wav_files)} files)")
    print(f"{'=' * 60}")
    
    results = []
    working_well = []
    has_issues = []
    silent = []
    
    for wav_file in wav_files:
        basename = os.path.basename(wav_file)
        # Extract algorithm name from filename
        parts = basename.replace('.wav', '').split('_', 2)
        if len(parts) >= 3:
            algo_num = parts[1]
            algo_name = parts[2]
        else:
            algo_num = "??"
            algo_name = basename
        
        result = analyze_wav(wav_file)
        result['name'] = algo_name
        result['num'] = algo_num
        results.append(result)
        
        if 'error' in result:
            print(f"[{algo_num}] {algo_name:20} ERROR: {result['error']}")
            has_issues.append(algo_name)
        elif not result['has_signal']:
            print(f"[{algo_num}] {algo_name:20} ❌ SILENT")
            silent.append(algo_name)
        else:
            freq = result['freq_estimate']
            dc = abs(result['dc_offset'])
            clip = result['clipping_percent']
            
            # Determine status
            status = "✅"
            issues = []
            
            # Expected ~261 Hz for Middle C
            if freq < 230 or freq > 290:
                issues.append(f"freq={freq:.0f}Hz")
            if dc > 1000:
                issues.append(f"DC={dc:.0f}")
            if clip > 5:
                issues.append(f"clip={clip:.1f}%")
            if result['peak'] < 5000:
                issues.append("weak")
            
            if issues:
                status = "⚠️"
                has_issues.append(algo_name)
                issue_str = " [" + ", ".join(issues) + "]"
            else:
                working_well.append(algo_name)
                issue_str = ""
            
            print(f"[{algo_num}] {algo_name:20} {status} {freq:4.0f}Hz, peak={result['peak']:5d}, DC={dc:4.0f}{issue_str}")
    
    # Summary
    print(f"\nSummary for {label}:")
    print(f"  Working well: {len(working_well)}")
    print(f"  Has issues: {len(has_issues)}")
    print(f"  Silent: {len(silent)}")
    
    return results

def main():
    print("=" * 80)
    print("BRAIDY WAV ANALYSIS REPORT")
    print("=" * 80)
    
    # Analyze JUCE implementation
    juce_results = analyze_directory('testing/juce_wavs', 'JUCE/BRAIDY')
    
    # Analyze Braids reference (if available)
    braids_results = analyze_directory('testing/braids_wavs', 'BRAIDS REFERENCE')
    
    # If both available, compare
    if juce_results and braids_results:
        print("\n" + "=" * 80)
        print("COMPARISON")
        print("=" * 80)
        
        # Create lookup by algorithm name
        braids_dict = {r['name']: r for r in braids_results if 'error' not in r}
        
        matches = 0
        mismatches = 0
        
        for juce in juce_results:
            if 'error' in juce:
                continue
            
            name = juce['name']
            if name in braids_dict:
                braids = braids_dict[name]
                
                # Compare key metrics
                freq_diff = abs(juce['freq_estimate'] - braids['freq_estimate'])
                dc_diff = abs(juce['dc_offset'] - braids['dc_offset'])
                
                if freq_diff < 5 and dc_diff < 500:
                    matches += 1
                    status = "✅ MATCH"
                else:
                    mismatches += 1
                    status = "❌ MISMATCH"
                    print(f"{name:20} {status} - Freq diff: {freq_diff:.1f}Hz, DC diff: {dc_diff:.0f}")
        
        print(f"\nTotal matches: {matches}")
        print(f"Total mismatches: {mismatches}")

if __name__ == "__main__":
    main()