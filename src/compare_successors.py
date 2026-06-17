import sys
import os
import time
import argparse

# Ensure src is in python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".")))

from libs.utils.graph import Graph
from libs.encodings.cardinality_one.scl import NSCEncoding
from libs.encodings.cardinality_one.pblib import PbLibEncoder
from solver import HcpSolver, IncHcpSolver

# Successors
from libs.models.successor.unary import Unary
from libs.models.successor.lfsr import LFSR
from libs.models.successor.binary_adder import BinaryAdder
from libs.models.successor.binary_adder_original import BinaryAdderOriginal
from libs.models.successor.crt import CRT
from libs.models.successor.vee import Vee
from libs.models.successor.ocrt import OCRT
from libs.models.successor.inc_crt import IncCRT
from libs.models.successor.inc_ocrt import IncOCRT
from libs.models.successor.ounary import OUnary
from libs.models.successor.pruning_crt import PruningCRT

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


def run_benchmark(graph_path, solver_name):
    # Successor classes mapping
    successors_info = [
        ("Unary", lambda enc: Unary(enc)),
        ("LFSR", lambda enc: LFSR(enc)),
        ("BinaryAdder", lambda enc: BinaryAdder(enc)),
        ("BinaryAdderOriginal", lambda enc: BinaryAdderOriginal(enc)),
        ("CRT", lambda enc: CRT(enc)),
        ("IncCRT", lambda enc: IncCRT(enc)),
        ("Vee", lambda enc: Vee(enc)),
        ("OCRT", lambda enc: OCRT(vee_encoder=NSCEncoding(), crt_encoder=enc)),
        ("IncOCRT", lambda enc: IncOCRT(vee_encoder=NSCEncoding(), crt_encoder=enc)),
        ("OUnary", lambda enc: OUnary(vee_encoder=NSCEncoding())),
        ("PruningCRT", lambda enc: PruningCRT(enc)),
     ]

    print(f"Graph: {os.path.basename(graph_path)}")
    print(f"Solver: {solver_name.upper()}\n")

    results = []

    # Choose encoder
    # Using PbLibEncoder as default if available, else NSCEncoding
    try:
        encoder = PbLibEncoder()
    except Exception:
        encoder = NSCEncoding()

    for name, init_successor in successors_info:
        print(f"Running {name}...")
        
        # Instantiate successor and solver
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
            # Using solve_cadical()
            solve_res = hcpSolver.solve_cadical()
        else:
            # Using solve() (Glucose 4)
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

    # Print nice table
    header = f"{'Successor':<22} | {'Vars':<8} | {'Clauses':<10} | {'Build (s)':<10} | {'Solve (s)':<10} | {'Status':<12}"
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))
    for res in results:
        print(f"{res['name']:<22} | {res['vars']:<8} | {res['clauses']:<10} | {res['build_time']:<10.4f} | {res['solve_time']:<10.4f} | {res['verification']:<12}")
    print("=" * len(header) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare Successor Encoders on HCP-SAT.")
    parser.add_argument("--graph", type=str, default="src/data/fhcpcs/graph1.hcp", help="Path to graph file")
    parser.add_argument("--solver", type=str, choices=["cadical", "glucose"], default="cadical", help="SAT Solver backend")
    args = parser.parse_args()

    # Resolve relative path if run from project root
    graph_path = args.graph
    if not os.path.exists(graph_path):
        # try prepending project dir
        graph_path = os.path.join("/home/wizardap/HCP-SAT", graph_path)

    run_benchmark(graph_path, args.solver)
