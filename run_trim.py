import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))
from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.crt import CRT

class CRT_Optimized(CRT):
    def build_clauses(self, context, graph):
        self.context = context
        n = graph.v
        first = graph.start_vertex
        cycle, nBits = self.get_cycle_and_nbits(n)

        # SKIP STEP 1!
        
        # 2. Exactly one outgoing edge
        for i in range(1, n + 1):
            literals = [self.getH(i, j) for j in graph.graph[i]]
            self.exactly_one_constraint(literals)

        # 3. Exactly one incoming edge
        for j in range(1, n + 1):
            literals = [self.getH(i, j) for i in range(1, n + 1) if j in graph.graph[i]]
            self.exactly_one_constraint(literals)

        # 4-9 same as before
        self.enforce_non_zero_lfsr(n, cycle, nBits)
        
        first_neighbors = [u for u in range(1, n + 1) if first in graph.graph[u]]
        self.add_clause([self.getH(neighbor, first) for neighbor in first_neighbors])
        
        if not graph.is_directed:
            for i in range(len(first_neighbors)):
                clause = [self.getH(first, first_neighbors[j]) for j in range(i)]
                clause.append(-self.getH(first_neighbors[i], first))
                self.add_clause(clause)
                
        self.init_starting_position(first, cycle)
        self.init_termination_position(n, first, first_neighbors, cycle)
        self.add_transitions(n, graph, first, cycle)
        return self.context

def test(graph_path, cls):
    enc = NSCEncoding()
    successor = cls(enc)
    solver = HcpSolver(successor, enc)
    solver.graph.load_graph_from_file(graph_path)
    solver.successor.build_clauses(solver.context, solver.graph)
    t0 = time.time()
    res = solver.solve_cadical()
    t1 = time.time() - t0
    print(f"{cls.__name__} | Vars: {res['nofVariables']:5} | Clauses: {res['nofClauses']:6} | Solve: {t1:.4f}s")

test("src/data/fhcpcs/graph30.hcp", CRT)
test("src/data/fhcpcs/graph30.hcp", CRT_Optimized)
