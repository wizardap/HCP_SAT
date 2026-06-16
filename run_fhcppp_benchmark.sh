#!/bin/bash
# ==============================================================================
# Automated Benchmark Automation Script for fhcppp
# ==============================================================================
# This script runs the baseline and different preprocessing phases for both
# CRT and VCCRT on the fhcppp dataset. It moves the resulting log-file.txt
# to its corresponding directory.
# ==============================================================================

set -e

DATA_DIR="src/data/fhcppp"
SOLVER="cadical"

# Detect Python executable dynamically
PYTHON_BIN="./.venv/bin/python"
if [ ! -f "$PYTHON_BIN" ]; then
    if command -v python3 &>/dev/null; then
        PYTHON_BIN="python3"
    else
        PYTHON_BIN="python"
    fi
fi

# Helper function to move the default log-file.txt to the target folder
move_log_to() {
    TARGET_DIR=$1
    mkdir -p "$TARGET_DIR"
    if [ -f "log-file.txt" ]; then
        mv log-file.txt "$TARGET_DIR/log-file.txt"
        echo "[OK] Moved log-file.txt to $TARGET_DIR/log-file.txt"
    else
        echo "[Warning] log-file.txt was not generated!"
    fi
}

# Clean any existing log-file.txt at the root
rm -f log-file.txt

echo "================================================================================"
echo "HCP-SAT Automated Benchmark for fhcppp"
echo "Data Directory: $DATA_DIR"
echo "Solver: $SOLVER"
echo "Using Python: $PYTHON_BIN"
echo "================================================================================"

# 1. CRT (Baseline)
echo ""
echo ">>> [1/6] Running CRT Baseline..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_baseline"

# 2. CRT (P1: Cascade only)
echo ""
echo ">>> [2/6] Running CRT P1 (Cascade)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --no-probing --no-contraction --no-2cut --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1"

# 3. CRT (P1+P2: Contraction)
echo ""
echo ">>> [3/6] Running CRT P1+P2 (Contraction)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --no-probing --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1_p2"

# 4. CRT (P1+P2+P3: Probing)
echo ""
echo ">>> [4/6] Running CRT P1+P2+P3 (Probing)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1_p2_p3"

# 5. VCCRT (Baseline)
echo ""
echo ">>> [5/6] Running VCCRT Baseline..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor vccrt --solver $SOLVER
move_log_to "benchmark/fhcppp/vccrt_baseline"

# 6. VCCRT (Preprocessed)
echo ""
echo ">>> [6/6] Running VCCRT Preprocessed..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor vccrt_preprocessed --solver $SOLVER
move_log_to "benchmark/fhcppp/vccrt_preprocessed"

echo ""
echo "================================================================================"
echo "All benchmarks finished! Results stored under benchmark/fhcppp/"
echo "================================================================================"
