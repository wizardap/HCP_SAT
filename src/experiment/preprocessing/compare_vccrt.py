import sys
import os
import time
import argparse

# Ensure src is in python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../..")))

from libs.utils.graph import Graph
from libs.encodings.cardinality_one.scl import NSCEncoding
from libs.encodings.cardinality_one.pblib import PbLibEncoder
from solver import HcpSolver, IncHcpSolver

from libs.models.successor.crt import CRT
from libs.models.successor.vccrt import VCCRT
from experiment.preprocessing.crt_preprocessed import CRTPreprocessed


def verify_hamiltonian_cycle(HCP, graph, N):
    if len(HCP) != N:
        return False, f"Incorrect number of edges: got {len(HCP)}, expected {N}"

    out_degree = {}
    in_degree = {}
    adj = {}
    for u, v in HCP:
        out_degree[u] = out_degree.get(u, 0) + 1
        in_degree[v] = in_degree.get(v, 0) + 1
        adj[u] = v

    for i in range(1, N + 1):
        if out_degree.get(i, 0) != 1:
            return False, f"Vertex {i} has out-degree {out_degree.get(i, 0)} (expected 1)"
        if in_degree.get(i, 0) != 1:
            return False, f"Vertex {i} has in-degree {in_degree.get(i, 0)} (expected 1)"

    visited = set()
    curr = 1
    for _ in range(N):
        if curr in visited:
            return False, f"Sub-cycle detected (loop at vertex {curr})"
        visited.add(curr)
        if curr not in adj:
            return False, f"No outgoing edge from vertex {curr}"
        curr = adj[curr]

    if curr != 1:
        return False, f"Cycle did not return to start vertex (ended at {curr})"
    if len(visited) != N:
        return False, f"Only visited {len(visited)} vertices (expected {N})"

    # Check original graph edges
    for u, v in HCP:
        if v not in graph[u]:
            return False, f"Edge ({u}, {v}) does not exist in the original graph"

    return True, "Valid HC!"


def get_files_from_folder(folder_path):
    import re
    def extract_numbers(filename):
        numbers = re.findall(r'\d+', filename)
        return list(map(int, numbers))

    return sorted(
        [entry.path.replace("\\", "/") for entry in os.scandir(folder_path) if entry.is_file()],
        key=lambda x: extract_numbers(os.path.basename(x))
    )


def run_comparison(graph_path, solver_name):
    configs = [
        ("CRT (Baseline)", lambda enc: CRT(enc)),
        ("CRTPreprocessed", lambda enc: CRTPreprocessed(enc)),
        ("VCCRT (Baseline)", lambda enc: VCCRT(enc)),
        ("VCCRTPreprocessed", lambda enc: CRTPreprocessed(enc, crt_class=VCCRT)),
    ]

    results = []

    # Choose encoder
    try:
        encoder = PbLibEncoder()
    except Exception:
        encoder = NSCEncoding()

    for name, init_successor in configs:
        successor = init_successor(encoder)
        if getattr(successor, 'is_incremental', False):
            hcpSolver = IncHcpSolver(successor, encoder)
        else:
            hcpSolver = HcpSolver(successor, encoder)
            
        hcpSolver.graph.load_graph_from_file(graph_path)

        # Build clauses
        t0 = time.time()
        hcpSolver.successor.build_clauses(hcpSolver.context, hcpSolver.graph)
        build_time = time.time() - t0

        # Solve
        t0 = time.time()
        if solver_name == "cadical":
            solve_res = hcpSolver.solve_cadical()
        else:
            solve_res = hcpSolver.solve()
        solve_time = time.time() - t0

        status = solve_res["status"]
        solve_reported_time = solve_res["time"]
        model = solve_res["model"]
        nof_vars = solve_res["nofVariables"]
        nof_clauses = solve_res["nofClauses"]

        # Validate
        verification_status = "N/A"
        if status == "SAT" and model is not None:
            model_set = set(model)
            N = hcpSolver.graph.v
            HCP = []
            for i in range(1, N + 1):
                for j in range(1, N + 1):
                    if successor.getH(i, j) in model_set:
                        HCP.append((i, j))
            success, msg = verify_hamiltonian_cycle(HCP, hcpSolver.graph.graph, N)
            if not success:
                print(f"[Verification Error] {name}: {msg}")
            verification_status = "SUCCESS" if success else "FAILED"
        elif status == "UNSAT":
            verification_status = "UNSAT"
        else:
            verification_status = "TIMEOUT"

        results.append({
            "name": name,
            "vars": nof_vars,
            "clauses": nof_clauses,
            "build_time": build_time,
            "solve_time": solve_reported_time,
            "verification": verification_status
        })
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare CRT vs VCCRT with and without Preprocessing.")
    parser.add_argument("--graph", type=str, default=None, help="Path to a single graph file")
    parser.add_argument("-d", "--data", type=str, default="src/data/fhcpcs", help="Folder containing graph instances")
    parser.add_argument("-n", "--limit", type=int, default=5, help="Limit number of files to run when --data is used")
    parser.add_argument("--solver", type=str, choices=["cadical", "glucose"], default="cadical", help="SAT Solver backend")
    args = parser.parse_args()

    if args.graph:
        graph_files = [args.graph]
    else:
        data_folder = args.data
        if not os.path.exists(data_folder):
            data_folder = os.path.join("/home/wizardap/HCP-SAT", data_folder)
        if not os.path.exists(data_folder):
            print(f"Data folder {args.data} not found.")
            sys.exit(1)
        graph_files = get_files_from_folder(data_folder)[:args.limit]

    print(f"Comparing CRT vs VCCRT on {len(graph_files)} instances...")
    print(f"Solver: {args.solver.upper()}")
    print("-" * 110)

    all_results = []
    for path in graph_files:
        filename = os.path.basename(path)
        print(f"\n--- Running instance: {filename} ---")
        try:
            res = run_comparison(path, args.solver)
            all_results.append((filename, res))
        except Exception as e:
            print(f"Error running instance {filename}: {e}")

    # Print final comparison table
    header = f"{'File / Successor':<35} | {'Vars':<8} | {'Clauses':<10} | {'Build (s)':<10} | {'Solve (s)':<10} | {'Status':<12}"
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))
    for filename, res_list in all_results:
        for idx, res in enumerate(res_list):
            label = f"{filename} ({res['name']})" if idx == 0 else f"  └─ {res['name']}"
            print(f"{label:<35} | {res['vars']:<8} | {res['clauses']:<10} | {res['build_time']:<10.4f} | {res['solve_time']:<10.4f} | {res['verification']:<12}")
        print("-" * len(header))
    print("=" * len(header) + "\n")
