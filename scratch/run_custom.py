import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))
from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.crt import CRT
from libs.models.successor.pruning_crt import PruningCRT
from libs.models.successor.ocrt import OCRT
from libs.models.successor.ounary import OUnary
from compare_successors import verify_hamiltonian_cycle

def test(graph_path):
    print("Testing:", graph_path)
    for name, Model in [("CRT", lambda e: CRT(e)), ("PruningCRT", lambda e: PruningCRT(e))]:
        enc = NSCEncoding()
        solver = HcpSolver(Model(enc), enc)
        solver.graph.load_graph_from_file(graph_path)
        solver.successor.build_clauses(solver.context, solver.graph)
        t0 = time.time()
        res = solver.solve_cadical()
        t1 = time.time() - t0
        print(f"{name:10} | Vars: {res['nofVariables']:5} | Clauses: {res['nofClauses']:6} | Solve: {t1:.4f}s | Status: {res['status']}")

test("src/data/fhcppp/graph223.hcp")
