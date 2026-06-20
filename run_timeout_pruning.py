import sys
import os
import time
import random
import argparse
import shutil

# Add src to Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))

from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.pruning_crt import PruningCRT
from compare_successors import verify_hamiltonian_cycle

def run_single_timeout_benchmark(graph_path, seed, timeout=600):
    random.seed(seed)
    
    enc = NSCEncoding()
    solver = HcpSolver(PruningCRT(enc), enc)
    solver.graph.load_graph_from_file(graph_path)
    
    # Build clauses
    solver.successor.build_clauses(solver.context, solver.graph)
    cnf = solver.context.cnf.clauses
    
    # Shuffle clauses using the seed to ensure robustness
    random.shuffle(cnf)
    
    nof_vars = solver.context.total_vars()
    nof_clauses = len(cnf)
    
    input_file = 'src/utils/all_cadical/input.txt'
    output_file = 'src/utils/all_cadical/output.txt'
    report_file = 'src/utils/all_cadical/report.txt'
    
    # Clean up old files
    for f in [input_file, output_file, report_file]:
        if os.path.exists(f):
            os.remove(f)
            
    os.makedirs(os.path.dirname(input_file), exist_ok=True)
    
    with open(input_file, 'w') as writer:
        for clause in cnf:
            writer.write(" ".join(map(str, clause)) + "\n")
            
    # Setup seed directory structure
    graph_name = os.path.basename(graph_path)
    seed_dir = f"benchmark/experiment/sub-cycle/seed_{seed}"
    os.makedirs(seed_dir, exist_ok=True)
    
    dest_output = os.path.join(seed_dir, f"{graph_name}_PruningCRT_output.txt")
    dest_report = os.path.join(seed_dir, f"{graph_name}_PruningCRT_report.txt")
    dest_cadical_log = os.path.join(seed_dir, f"{graph_name}_PruningCRT_cadical.log")
    
    # Run cadical under runlim, capturing the verbose cadical output to dest_cadical_log
    t0 = time.time()
    # Adding +2s margin to runlim limit
    bashCommand = f"./src/utils/all_cadical/runlim -r {timeout + 2} -o {report_file} {sys.executable} src/utils/all_cadical/cadical.py > {dest_cadical_log} 2>&1"
    os.system(bashCommand)
    
    t_solve = time.time() - t0
    status = "UNKNOWN"
    model = None
    
    try:
        with open(output_file, 'r') as f:
            lines = f.readlines()
        if len(lines) > 1 and lines[0].strip() != "-1":
            t_solve = float(lines[1].strip())
            status = "SAT" if lines[0].strip() == "1" else "UNSAT"
            model = list(map(int, lines[2].split()))
        else:
            status = "TIMEOUT"
            t_solve = float(timeout)
    except Exception:
        status = "TIMEOUT"
        t_solve = float(timeout)
        
    # Verify the cycle if SAT
    verification = "None"
    if status == "SAT" and model is not None:
        model_set = set(model)
        N = solver.graph.v
        HCP = []
        for i in range(1, N + 1):
            for j in range(1, N + 1):
                if solver.successor.getH(i, j) in model_set:
                    HCP.append((i, j))
        success, msg = verify_hamiltonian_cycle(HCP, solver.graph.graph, N)
        verification = "valid" if success else "None"
    
    if os.path.exists(output_file):
        shutil.copy(output_file, dest_output)
    else:
        with open(dest_output, "w") as f_out:
            f_out.write(f"-1\n{t_solve}\n")
            
    if os.path.exists(report_file):
        shutil.copy(report_file, dest_report)
        
    # Write to seed-specific log-file.txt
    log_file_path = os.path.join(seed_dir, "log-file.txt")
    with open(log_file_path, "a") as f_log:
        run_name = f"{graph_name}_PruningCRT"
        f_log.write(f"{run_name.ljust(30)} {str(nof_vars).ljust(10)} {str(nof_clauses).ljust(10)} {status.ljust(10)}  {str(t_solve).ljust(10)} {verification.ljust(10)}\n")
        
    # Clean up temporary run files
    for f in [input_file, output_file, report_file]:
        if os.path.exists(f):
            os.remove(f)
            
    return t_solve, status

def main():
    parser = argparse.ArgumentParser(description="Run PruningCRT on timeout HCP graphs.")
    parser.add_argument("--timeout", type=int, default=600, help="Timeout limit for CaDiCaL in seconds (default: 600).")
    parser.add_argument("--seeds", type=int, default=1, help="Number of seeds to run (default: 1).")
    args = parser.parse_args()
    
    timeout_graphs = [
        "src/data/fhcppp/graph424.hcp"
    ]
    
    # Check that paths exist
    graphs = []
    for g in timeout_graphs:
        if os.path.exists(g):
            graphs.append(g)
        else:
            print(f"Warning: {g} not found. Skipping.")
            
    if not graphs:
        print("No valid timeout graphs found to process.")
        return
        
    print(f"=== Running Timeout PruningCRT Benchmark ===")
    print(f"Graphs to test: {[os.path.basename(g) for g in graphs]}")
    print(f"Seeds count: {args.seeds}")
    print(f"Timeout per run: {args.timeout}s\n")
    
    seeds = list(range(args.seeds))
    
    for graph in graphs:
        graph_name = os.path.basename(graph)
        print(f"--- Running benchmark for: {graph_name} ---")
        for seed in seeds:
            print(f"  Seed {seed:2d}/{args.seeds-1:2d} ... ", end="", flush=True)
            t_solve, status = run_single_timeout_benchmark(graph, seed, args.timeout)
            print(f"status={status}, time={t_solve:.4f}s")
            
    print("\n[OK] Benchmark workflow run complete!")

if __name__ == "__main__":
    main()
