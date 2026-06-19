import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))
from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.optimized_crt import OptimizedCRT
from libs.models.successor.crt import CRT

def test(graph_path, method_cls):
    print(f"Testing {graph_path} with {method_cls.__name__}")
    enc = NSCEncoding()
    successor = method_cls(enc)
    solver = HcpSolver(successor, enc)
    solver.graph.load_graph_from_file(graph_path)
    solver.successor.build_clauses(solver.context, solver.graph)
    t0 = time.time()
    res = solver.solve_cadical()
    t1 = time.time() - t0
    print(f"Vars: {res['nofVariables']:5} | Clauses: {res['nofClauses']:6} | Solve: {t1:.4f}s | Status: {res['status']}")

test("src/data/fhcpcs/graph14.hcp", CRT)
test("src/data/fhcpcs/graph14.hcp", OptimizedCRT)
test("src/data/fhcpcs/graph15.hcp", CRT)
test("src/data/fhcpcs/graph15.hcp", OptimizedCRT)
test("src/data/fhcpcs/graph27.hcp", CRT)
test("src/data/fhcpcs/graph27.hcp", OptimizedCRT)
