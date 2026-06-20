import sys
import os
import time
import random
import statistics
import argparse
import json
import shutil

# Add src to Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))

from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.crt import CRT
from libs.models.successor.pruning_crt import PruningCRT
from compare_successors import verify_hamiltonian_cycle

def run_single_benchmark(graph_path, model_class, seed, timeout=300):
    random.seed(seed)
    
    enc = NSCEncoding()
    
    # Dynamically override global TIME_BUDGET
    import solver as solver_mod
    original_timeout = solver_mod.TIME_BUDGET
    solver_mod.TIME_BUDGET = timeout
    
    from solver import IncHcpSolver, HcpSolver
    if model_class.__name__ in ["IncCRT", "IncOCRT"]:
        solver = IncHcpSolver(model_class(enc), enc)
    else:
        solver = HcpSolver(model_class(enc), enc)
        
    solver.graph.load_graph_from_file(graph_path)
    
    # Run the native C++ solver
    solve_res = solver.solve_cadical(seed=seed)
    
    # Restore global timeout
    solver_mod.TIME_BUDGET = original_timeout

    t_solve = solve_res["time"]
    status = solve_res["status"]
    model = solve_res["model"]
    nof_vars = solve_res["nofVariables"]
    nof_clauses = solve_res["nofClauses"]
    
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
        
    # Setup seed directory structure: benchmark/experiment/sub-cycle/seed_{seed}/
    graph_name = os.path.basename(graph_path)
    model_name = model_class.__name__
    seed_dir = f"benchmark/experiment/sub-cycle/seed_{seed}"
    os.makedirs(seed_dir, exist_ok=True)
    
    dest_output = os.path.join(seed_dir, f"{graph_name}_{model_name}_output.txt")
    dest_report = os.path.join(seed_dir, f"{graph_name}_{model_name}_report.txt")
    
    # Write solver output file
    with open(dest_output, "w") as f_out:
        if status == "SAT" and model is not None:
            f_out.write(f"1\n{t_solve}\n" + " ".join(map(str, model)) + "\n")
        elif status == "UNSAT":
            f_out.write(f"0\n{t_solve}\n")
        else:
            f_out.write(f"-1\n{t_solve}\n")
            
    # Write mockup runlim report file
    with open(dest_report, "w") as f_rep:
        f_rep.write(f"[runlim] time: {t_solve:.6f} seconds\n")
        f_rep.write(f"[runlim] status: {status}\n")
        
    # Write to seed-specific log-file.txt
    log_file_path = os.path.join(seed_dir, "log-file.txt")
    with open(log_file_path, "a") as f_log:
        run_name = f"{graph_name}_{model_name}"
        f_log.write(f"{run_name.ljust(30)} {str(nof_vars).ljust(10)} {str(nof_clauses).ljust(10)} {status.ljust(10)}  {str(t_solve).ljust(10)} {verification.ljust(10)}\n")
        
    return t_solve, status

def main():
    parser = argparse.ArgumentParser(description="Benchmark CRT vs PruningCRT on HCP graphs.")
    parser.add_argument("graphs", nargs="*", help="Paths to HCP graph files. If omitted, default graphs will be used.")
    parser.add_argument("--timeout", type=int, default=300, help="Timeout limit for CaDiCaL in seconds (default: 300).")
    parser.add_argument("--seeds", type=int, default=10, help="Number of seeds to run (default: 10).")
    args = parser.parse_args()
    
    # Default graphs if none specified
    graphs = args.graphs
    if not graphs:
        graphs = ["src/data/fhcppp/graph48.hcp"]
        
    print(f"=== HCP-SAT Benchmark: CRT vs PruningCRT ===")
    print(f"Graphs to test: {graphs}")
    print(f"Seeds count: {args.seeds}")
    print(f"Timeout per run: {args.timeout}s\n")
    
    raw_results = {}
    seeds = list(range(args.seeds))
    
    # Ensure destination base directory exists
    base_exp_dir = "benchmark/experiment/sub-cycle"
    os.makedirs(base_exp_dir, exist_ok=True)
    
    # Write header for the new batch to each seed log-file.txt
    for seed in seeds:
        seed_dir = os.path.join(base_exp_dir, f"seed_{seed}")
        os.makedirs(seed_dir, exist_ok=True)
        log_file_path = os.path.join(seed_dir, "log-file.txt")
        with open(log_file_path, "a") as f_log:
            f_log.write(f"\n# --- New Benchmark Batch: {time.strftime('%Y-%m-%d %H:%M:%S')} ---\n")
            
    for graph in graphs:
        graph_name = os.path.basename(graph)
        print(f"--- Running benchmark for: {graph_name} ---")
        raw_results[graph_name] = {
            "CRT": [],
            "PruningCRT": []
        }
        
        for name, ModelClass in [("CRT", CRT), ("PruningCRT", PruningCRT)]:
            print(f"  Successor: {name}")
            for seed in seeds:
                print(f"    Seed {seed:2d}/{args.seeds-1:2d} ... ", end="", flush=True)
                t_solve, status = run_single_benchmark(graph, ModelClass, seed, args.timeout)
                print(f"status={status}, time={t_solve:.4f}s")
                raw_results[graph_name][name].append({
                    "seed": seed,
                    "time": t_solve,
                    "status": status
                })
                
    # Save statistics to summary.txt under benchmark/experiment/sub-cycle/
    summary_path = os.path.join(base_exp_dir, "summary.txt")
    with open(summary_path, "w") as f_sum:
        f_sum.write("=== HCP-SAT Benchmark Summary Report ===\n")
        f_sum.write(f"Generated on: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_sum.write(f"Timeout: {args.timeout}s | Seeds: {args.seeds}\n")
        f_sum.write("=================================================================================\n\n")
        
        for graph_name, models in raw_results.items():
            f_sum.write(f"Graph: {graph_name}\n")
            f_sum.write("-" * 80 + "\n")
            f_sum.write(f"{'Successor':15} | {'Min (s)':10} | {'Max (s)':10} | {'Mean (s)':10} | {'Median (s)':10} | {'Timeouts':8}\n")
            f_sum.write("-" * 80 + "\n")
            
            for name, runs in models.items():
                times = [run["time"] for run in runs]
                timeouts = sum(1 for run in runs if run["status"] == "TIMEOUT")
                
                min_time = min(times)
                max_time = max(times)
                mean_time = statistics.mean(times)
                median_time = statistics.median(times)
                
                f_sum.write(f"{name:15} | {min_time:10.4f} | {max_time:10.4f} | {mean_time:10.4f} | {median_time:10.4f} | {timeouts:d}/{args.seeds}\n")
            f_sum.write("-" * 80 + "\n\n")
            
    # Save global raw results JSON
    raw_output_path = os.path.join(base_exp_dir, "raw_results.json")
    with open(raw_output_path, "w") as f:
        json.dump(raw_results, f, indent=4)
        
    # Print the summary to terminal
    with open(summary_path, "r") as f:
        print("\n" + f.read())
        
    print(f"[OK] Summary report saved to {summary_path}")
    print(f"[OK] Raw results saved to {raw_output_path}")

if __name__ == "__main__":
    main()
