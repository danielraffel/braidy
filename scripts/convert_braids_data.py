#!/usr/bin/env python3
"""
Convert Braids wavetable binary data to C++ header file
"""

import os
import sys
import struct

def read_binary_file(filepath):
    """Read binary file and return as byte array"""
    with open(filepath, 'rb') as f:
        return f.read()

def convert_to_cpp_array(data, name, type_name='uint8_t'):
    """Convert byte data to C++ array initialization"""
    lines = []
    lines.append(f"// Braids wavetable data - {len(data)} bytes")
    lines.append(f"const {type_name} {name}[{len(data)}] = {{")
    
    # Write data in rows of 16 values
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        values = ', '.join(f"0x{b:02X}" for b in chunk)
        if i + 16 < len(data):
            values += ','
        lines.append(f"    {values}")
    
    lines.append("};")
    return '\n'.join(lines)

def convert_to_int16_array(data, name):
    """Convert byte data to int16_t array (for wavetables)"""
    lines = []
    # Convert bytes to int16 values (little-endian)
    int16_values = []
    for i in range(0, len(data), 2):
        if i + 1 < len(data):
            value = struct.unpack('<h', data[i:i+2])[0]
            int16_values.append(value)
    
    lines.append(f"// Braids wavetable data - {len(int16_values)} int16_t values")
    lines.append(f"const int16_t {name}[{len(int16_values)}] = {{")
    
    # Write data in rows of 8 values
    for i in range(0, len(int16_values), 8):
        chunk = int16_values[i:i+8]
        values = ', '.join(f"{v:6d}" for v in chunk)
        if i + 8 < len(int16_values):
            values += ','
        lines.append(f"    {values}")
    
    lines.append("};")
    return '\n'.join(lines)

def main():
    braids_data_dir = "eurorack/braids/data"
    output_file = "Source/BraidyCore/BraidsWavetableData.h"
    
    # Read the wavetable files
    waves_data = read_binary_file(os.path.join(braids_data_dir, "waves.bin"))
    map_data = read_binary_file(os.path.join(braids_data_dir, "map.bin"))
    
    print(f"Read {len(waves_data)} bytes from waves.bin")
    print(f"Read {len(map_data)} bytes from map.bin")
    
    # Generate header file
    with open(output_file, 'w') as f:
        f.write("""#pragma once

// Braids Wavetable Data
// Ported from Mutable Instruments Braids
// Original code by Emilie Gillet (emilie.o.gillet@gmail.com)
// Used under MIT License

#include <cstdint>

namespace braidy {

""")
        
        # Convert waves.bin to int16_t array (wavetables are 16-bit signed)
        f.write(convert_to_int16_array(waves_data, "braids_wavetable_data"))
        f.write("\n\n")
        
        # Convert map.bin to uint8_t array (map is 8-bit)
        f.write(convert_to_cpp_array(map_data, "braids_wavetable_map"))
        f.write("\n\n")
        
        # Add constants
        f.write("// Wavetable constants\n")
        f.write(f"const size_t kBraidsWavetableSize = {len(waves_data) // 2};\n")
        f.write(f"const size_t kBraidsWavetableMapSize = {len(map_data)};\n")
        f.write("const size_t kBraidsWavetableBankSize = 64;  // 64 wavetables\n")
        f.write("const size_t kBraidsWavetableSampleSize = 129; // 129 samples per wavetable\n")
        
        f.write("\n} // namespace braidy\n")
    
    print(f"Generated {output_file}")
    
    # Also generate bandlimited waveform tables
    generate_bandlimited_tables()

def generate_bandlimited_tables():
    """Generate bandlimited comb waveforms like Braids"""
    import numpy as np
    
    output_file = "Source/BraidyCore/BraidsBandlimitedTables.h"
    WAVETABLE_SIZE = 256
    
    with open(output_file, 'w') as f:
        f.write("""#pragma once

// Braids Bandlimited Waveform Tables
// Generated from Braids synthesis algorithms

#include <cstdint>

namespace braidy {

""")
        
        # Generate bandlimited comb tables for different frequency zones
        for zone in range(15):
            f0 = 440.0 * 2.0 ** ((18 + 8 * zone - 69) / 12.0)
            if zone == 14:
                f0 = 48000 / 2.0 - 1
            else:
                f0 = min(f0, 48000 / 2.0)
            
            period = 48000 / f0
            m = 2 * np.floor(period / 2) + 1.0
            i = np.arange(-WAVETABLE_SIZE / 2, WAVETABLE_SIZE / 2) / float(WAVETABLE_SIZE)
            pulse = np.sin(np.pi * i * m) / (m * np.sin(np.pi * i) + 1e-9)
            pulse[WAVETABLE_SIZE // 2] = 1.0
            
            # Rotate for quadrature
            quadrature = np.fmod(np.arange(WAVETABLE_SIZE + 1) + WAVETABLE_SIZE / 4, WAVETABLE_SIZE).astype(int)
            pulse = pulse[quadrature[:-1]]
            
            # Scale to int16
            pulse = pulse - pulse.mean()
            mx = np.abs(pulse).max()
            pulse = (pulse / mx * 32000).astype(np.int16)
            
            f.write(f"const int16_t bandlimited_comb_{zone}[257] = {{\n")
            for j in range(0, len(pulse), 8):
                chunk = pulse[j:j+8]
                values = ', '.join(f"{v:6d}" for v in chunk)
                if j + 8 < len(pulse):
                    values += ','
                f.write(f"    {values}\n")
            f.write(f"    {pulse[-1]:6d}\n")  # Last sample repeated for interpolation
            f.write("};\n\n")
        
        f.write("} // namespace braidy\n")
    
    print(f"Generated {output_file}")

if __name__ == "__main__":
    main()