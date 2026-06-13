#!/bin/bash

# ==============================================================================
# HCP-SAT Environment Setup & Benchmark Execution Flow
# ==============================================================================
# This script automates the complete workflow on a new virtual machine:
# 1. Creates a Python virtual environment (.venv)
# 2. Installs required packages from requirements.txt
# 3. Grants execution permission to the benchmark script
# 4. Executes the full benchmark suite
# ==============================================================================

# Stop the script if any command fails
set -e

echo "=================================================="
echo "Starting HCP-SAT Setup and Benchmark Flow"
echo "=================================================="
echo ""

# Step 1: Create Virtual Environment
echo "=== Step 1: Creating Virtual Environment (.venv) ==="
if [ -d ".venv" ]; then
    echo "  Virtual environment (.venv) already exists. Skipping creation."
else
    # Check if python3-venv is installed
    if ! python3 -m venv .venv 2>/dev/null; then
        echo "  [ERROR] python3-venv is not installed. Please install it first."
        echo "  On Ubuntu/Debian run: sudo apt install python3-venv"
        exit 1
    fi
    echo "  Virtual environment (.venv) created successfully."
fi
echo ""

# Step 2: Install dependencies
echo "=== Step 2: Installing dependencies from requirements.txt ==="
echo "  Upgrading pip..."
./.venv/bin/pip install --upgrade pip

echo "  Installing required packages..."
./.venv/bin/pip install -r requirements.txt
echo "  Dependencies installed successfully."
echo ""

# Step 3: Set script permissions
echo "=== Step 3: Setting execution permissions for scripts ==="
chmod +x run_benchmarks.sh
echo "  Permissions set successfully."
echo ""

# Step 4: Execute Benchmarks
echo "=== Step 4: Executing Benchmark Suite ==="
./run_benchmarks.sh
echo ""

echo "=================================================="
echo "HCP-SAT Setup & Benchmark Flow Completed!"
echo "=================================================="
