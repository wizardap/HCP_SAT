import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))
from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.crt import CRT

def test(graph_path, cycle_val):
    print(f"Testing {graph_path} with cycle={cycle_val}")
    enc = NSCEncoding()
    successor = CRT(enc, cycle=cycle_val)
    solver = HcpSolver(successor, enc)
    solver.graph.load_graph_from_file(graph_path)
    solver.successor.build_clauses(solver.context, solver.graph)
    t0 = time.time()
    res = solver.solve_cadical()
    t1 = time.time() - t0
    print(f"Vars: {res['nofVariables']:5} | Clauses: {res['nofClauses']:6} | Solve: {t1:.4f}s | Status: {res['status']}")

test("src/data/fhcpcs/graph15.hcp", None)
test("src/data/fhcpcs/graph15.hcp", 511)
