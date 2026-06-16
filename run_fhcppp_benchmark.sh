#!/bin/bash
# ==============================================================================
# Automated Benchmark Automation Script for fhcppp
# ==============================================================================
# This script runs the different preprocessing phases for both CRT and VCCRT
# on the fhcppp dataset. It moves the resulting log-file.txt to its corresponding
# directory. Note: CRT (Baseline) is skipped as requested.
# ==============================================================================

set -e

DATA_DIR="src/data/fhcppp"
SOLVER="cadical"
ENCODER="pblib"

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
echo "Encoder: $ENCODER"
echo "Solver: $SOLVER"
echo "Using Python: $PYTHON_BIN"
echo "================================================================================"

# 1. CRT (P1: Cascade only)
echo ""
echo ">>> [1/5] Running CRT P1 (Cascade)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --no-probing --no-contraction --no-2cut --encoder $ENCODER --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1"

# 2. CRT (P1+P2: Contraction)
echo ""
echo ">>> [2/5] Running CRT P1+P2 (Contraction)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --no-probing --encoder $ENCODER --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1_p2"

# 3. CRT (P1+P2+P3: Probing)
echo ""
echo ">>> [3/5] Running CRT P1+P2+P3 (Probing)..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor crt_preprocessed --encoder $ENCODER --solver $SOLVER
move_log_to "benchmark/fhcppp/crt_p1_p2_p3"

# 4. VCCRT (Baseline)
echo ""
echo ">>> [4/5] Running VCCRT Baseline..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor vccrt --encoder $ENCODER --solver $SOLVER
move_log_to "benchmark/fhcppp/vccrt_baseline"

# 5. VCCRT (Preprocessed)
echo ""
echo ">>> [5/5] Running VCCRT Preprocessed..."
$PYTHON_BIN src/test_hcp.py --data $DATA_DIR --successor vccrt_preprocessed --encoder $ENCODER --solver $SOLVER
move_log_to "benchmark/fhcppp/vccrt_preprocessed"

echo ""
echo "================================================================================"
echo "All benchmarks finished! Results stored under benchmark/fhcppp/"
echo "================================================================================"
