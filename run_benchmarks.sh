#!/bin/bash

# ==============================================================================
# HCP-SAT Benchmark Automation Script
# ==============================================================================
# This script automates running compare_successors.py on all .hcp graph files 
# found in the specified data directories. It logs all results to a single 
# structured report file.
# ==============================================================================

# Directories containing .hcp files to benchmark
DATA_DIRS=(
    "src/data/fhcpcs"
    "src/data/fhcppp"
    "src/data/fhcpsl"
    "src/data/tsphcp"
    "src/data/vset"
)

# Output report file name
REPORT_FILE="benchmark_report.txt"

# Reset/Create the report file with headers
echo "=== HCP-SAT Benchmark Report ===" > "$REPORT_FILE"
echo "Generated on: $(date)" >> "$REPORT_FILE"
echo "Solver backend: CADICAL" >> "$REPORT_FILE"
echo "================================================================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# Detect Python environment dynamically
PYTHON_BIN="./.venv/bin/python"
if [ ! -f "$PYTHON_BIN" ]; then
    # Fallback to system python3 or python if venv doesn't exist on the VM
    if command -v python3 &>/dev/null; then
        PYTHON_BIN="python3"
    else
        PYTHON_BIN="python"
    fi
fi

echo "Using Python executable: $PYTHON_BIN"
echo "Results will be saved to: $REPORT_FILE"
echo "Starting benchmark..."
echo ""

# Loop through each directory
for dir in "${DATA_DIRS[@]}"; do
    # Check if directory exists
    if [ -d "$dir" ]; then
        echo "--------------------------------------------------"
        echo "Processing directory: $dir"
        echo "--------------------------------------------------"
        
        echo "### Directory: $dir" >> "$REPORT_FILE"
        echo "--------------------------------------------------------------------------------" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"

        # Find and sort all .hcp files in the directory (non-recursive)
        files=$(find "$dir" -maxdepth 1 -name "*.hcp" | sort)

        if [ -z "$files" ]; then
            echo "  No .hcp files found in $dir"
            echo "  No .hcp files found." >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            continue
        fi

        # Run comparison for each file
        for file in $files; do
            filename=$(basename "$file")
            echo "  Running comparison for $filename..."
            echo "#### Graph: $filename" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            
            # Execute the comparison script and append output to report file
            $PYTHON_BIN src/compare_successors.py --graph "$file" --solver cadical >> "$REPORT_FILE"
            
            echo "" >> "$REPORT_FILE"
            echo "================================================================================" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            echo "     [OK] Results for $filename written to $REPORT_FILE."
        done
        echo "  Finished directory: $dir."
        echo ""
    else
        echo "Directory not found: $dir (Skipping)"
    fi
done

echo ""
echo "=================================================="
echo "Benchmark complete!"
echo "All results successfully saved in: $REPORT_FILE"
echo "=================================================="
