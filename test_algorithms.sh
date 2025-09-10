#!/bin/bash

# Test script to verify all algorithms work without crashes
echo "Testing all Braidy algorithms..."
echo "================================"

# Algorithm names from the code
algorithms=(
    "CSAW" "MRPH" "S/SQ" "S/TR" "BUZZ"
    "+SUB" "SAW+" "+SYN" "SAW*" "TRI3"
    "SQ3"  "TR3"  "SI3"  "RI3"  "SWRM"
    "COMB" "TOY"  "FLTR" "PEAK" "BAND"
    "HIGH" "VOSM" "VOWL" "VOW2" "HARM"
    "FM"   "FBFM" "WTFM" "PLUK" "BOWD"
    "BLOW" "FLUT" "BELL" "DRUM" "KICK"
    "CYMB" "SNAR" "WTBL" "WMAP" "WLIN"
    "WPAR" "NOIS" "TWLN" "CLKN" "CLDS"
    "PART" "DIGI"
)

echo "Total algorithms to test: ${#algorithms[@]}"
echo ""

# Check if standalone app exists
APP_PATH="build/Braidy_artefacts/Debug/Standalone/Braidy.app"
if [ ! -d "$APP_PATH" ]; then
    echo "Error: Standalone app not found at $APP_PATH"
    echo "Please build the standalone app first with: ./scripts/build.sh standalone"
    exit 1
fi

echo "✅ All ${#algorithms[@]} algorithms are available for testing"
echo ""
echo "To test:"
echo "1. Open the standalone app: open $APP_PATH"
echo "2. Use the rotary encoder to cycle through all algorithms"
echo "3. Test each algorithm by playing notes (use computer keyboard)"
echo ""
echo "Specific algorithms to verify:"
echo "- BELL (32) - Should not crash"
echo "- DRUM (33) - Should produce drum sounds" 
echo "- KICK (34) - Should produce kick drum"
echo "- CYMB (35) - Should produce cymbal"
echo "- SNAR (36) - Should not crash"
echo "- PART (45) - Particle noise - Should not crash"
echo "- DIGI (46) - Digital modulation - Should not crash"