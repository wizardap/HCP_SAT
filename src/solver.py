import os
import sys
import random
import time
from threading import Timer
from pysat.solvers import Solver

from libs.utils.graph import Graph
from libs.utils.context import EncodingContext
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.models.successor.base import SuccessorMethod

TIME_BUDGET = 600


def interrupt(s):
    s.interrupt()


class HcpSolver:
    def __init__(self, successor: SuccessorMethod, encoder: CardinalityOneEncoder):
        self.successor = successor
        self.encoder = encoder
        self.successor.set_encoder(encoder)

        self.sat_solver = Solver(name='g4', use_timer=True)
        self.graph = Graph()
        self.context = EncodingContext()

    def solve(self):
        print("Running SAT solver...")

        result = {
            "nofVariables": None,
            "nofClauses": None,
            "status": None,
            "model": None,
            "time": None,
            "vOfHC": None
        }

        # Build clauses into self.context
        self.successor.build_clauses(self.context, self.graph)

        sat_solver = self.sat_solver
        sat_solver.append_formula(self.context.cnf.clauses)

        result["nofClauses"] = sat_solver.nof_clauses()
        result["nofVariables"] = sat_solver.nof_vars()

        timer = Timer(TIME_BUDGET, interrupt, [sat_solver])
        timer.start()

        sat_status = sat_solver.solve_limited(expect_interrupt=True)

        if sat_status is False:
            elapsed_time = float(format(sat_solver.time(), ".3f"))
            result["status"] = "UNSAT"
            result["time"] = elapsed_time
        else:
            solution = sat_solver.get_model()
            if solution is None:
                result["status"] = "TIMEOUT"
                result["time"] = 9999
            else:
                elapsed_time = float(format(sat_solver.time(), ".3f"))
                result["model"] = solution
                result["status"] = "SAT"
                result["time"] = elapsed_time

        timer.cancel()
        sat_solver.delete()
        return result

    def _solve_with_cpp(self, use_propagator=True):
        import subprocess
        
        # Build initial clauses into self.context
        self.successor.build_clauses(self.context, self.graph)
        cnf = self.context.cnf.clauses
        
        metadata = []
        if use_propagator:
            metadata.append(f"c hcp_metadata {self.graph.v} {1 if self.graph.is_directed else 0}")
            for i in range(1, self.graph.v + 1):
                for j in range(1, self.graph.v + 1):
                    var_id = self.successor.getH(i, j)
                    if var_id > 0:
                        metadata.append(f"c edge {var_id} {i} {j}")
        
        dimacs_lines = metadata
        dimacs_lines.append(f"p cnf {self.context.total_vars()} {len(cnf)}")
        for clause in cnf:
            dimacs_lines.append(" ".join(map(str, clause)) + " 0")
        
        dimacs_content = "\n".join(dimacs_lines) + "\n"
        
        print("Running C++ SAT Solver (CaDiCaL Native)...")
        # Ensure executable exists, compile if not
        cpp_exe = "./src/cpp/hcp_solver"
        if not os.path.exists(cpp_exe):
            print("Compiling C++ solver...")
            os.system(f"g++ -O3 -std=c++17 src/cpp/hcp_solver.cpp -o {cpp_exe} -lcadical -lpthread")

        process = subprocess.Popen(
            [cpp_exe, "-", str(TIME_BUDGET)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        try:
            stdout_data, stderr_data = process.communicate(input=dimacs_content, timeout=TIME_BUDGET + 10)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout_data, stderr_data = process.communicate()
            
        if stderr_data:
            print(stderr_data, file=sys.stderr, end="")
            
        result = {
            "nofVariables": self.context.total_vars(),
            "nofClauses": len(cnf),
            "status": "TIMEOUT",
            "model": None,
            "time": TIME_BUDGET,
            "vOfHC": None
        }
        
        for line in stdout_data.splitlines():
            if line.startswith("STATUS:"):
                result["status"] = line.split()[1]
            elif line.startswith("TIME:"):
                result["time"] = float(line.split()[1])
            elif line.startswith("MODEL:"):
                model_str = line[len("MODEL:"):].strip()
                result["model"] = list(map(int, model_str.split()))
                
        return result

    def solve_cadical(self):
        return self._solve_with_cpp(use_propagator=False)

    def print_result(self, model, getH, result):
        N = self.graph.v
        if model is None:
            print("No solution found.")
            return
        model_set = set(model)
        HCP = []
        for i in range(1, N + 1):
            for j in range(1, N + 1):
                if getH(i, j) in model_set:
                    HCP.append((i, j))

        print("Hamiltonian cycle:", end=" ")
        vertex = 1
        num = 1
        while True:
            print(vertex, end=" ")
            for i, j in HCP:
                if i == vertex:
                    vertex = j
                    break
            if vertex == 1:
                print("-> 1")
                break
            print("->", end=" ")
            num += 1
        print("Vertices of HC: ", num)
        result["vOfHC"] = num


class IncHcpSolver(HcpSolver):
    def find_all_cycles(self, model):
        N = self.graph.v
        model_set = set(model)
        
        # Extract active edges
        active_edges = []
        for i in range(1, N + 1):
            for j in range(1, N + 1):
                if self.successor.getH(i, j) in model_set:
                    active_edges.append((i, j))
                    
        # Build adjacency map
        adj = {}
        for u, v in active_edges:
            adj[u] = v
            
        visited = set()
        cycles = []
        
        for start in range(1, N + 1):
            if start not in visited and start in adj:
                cycle = []
                curr = start
                while curr not in visited:
                    visited.add(curr)
                    cycle.append(curr)
                    if curr in adj:
                        curr = adj[curr]
                    else:
                        break
                if curr == start:
                    cycle_edges = []
                    for k in range(len(cycle)):
                        u = cycle[k]
                        v = cycle[(k + 1) % len(cycle)]
                        cycle_edges.append((u, v))
                    cycles.append(cycle_edges)
        return cycles

    def solve(self):
        print("Running Incremental SAT solver (Glucose 4)...")
        import time

        result = {
            "nofVariables": None,
            "nofClauses": None,
            "status": None,
            "model": None,
            "time": None,
            "vOfHC": None
        }

        # Build initial clauses into self.context
        self.successor.build_clauses(self.context, self.graph)
        
        sat_solver = self.sat_solver
        sat_solver.append_formula(self.context.cnf.clauses)

        start_time = time.time()
        timeout = TIME_BUDGET
        
        while True:
            elapsed = time.time() - start_time
            if elapsed >= timeout:
                result["status"] = "TIMEOUT"
                result["time"] = timeout
                break
                
            timer = Timer(timeout - elapsed, interrupt, [sat_solver])
            timer.start()
            
            sat_status = sat_solver.solve_limited(expect_interrupt=True)
            timer.cancel()
            
            if sat_status is False:
                result["status"] = "UNSAT"
                result["time"] = time.time() - start_time
                break
            elif sat_status is None:
                result["status"] = "TIMEOUT"
                result["time"] = timeout
                break
            else:
                model = sat_solver.get_model()
                subcycles = self.find_all_cycles(model)
                
                # Check if it's a single cycle covering all vertices
                if len(subcycles) == 1 and len(subcycles[0]) == self.graph.v:
                    result["status"] = "SAT"
                    result["model"] = model
                    result["time"] = time.time() - start_time
                    break
                else:
                    # Add blocking clauses
                    for cycle in subcycles:
                        clause_cw = [-self.successor.getH(u, v) for u, v in cycle]
                        sat_solver.add_clause(clause_cw)
                        
                        if not self.graph.is_directed:
                            clause_ccw = [-self.successor.getH(v, u) for u, v in cycle]
                            sat_solver.add_clause(clause_ccw)
                            
        result["nofClauses"] = sat_solver.nof_clauses()
        result["nofVariables"] = sat_solver.nof_vars()
        sat_solver.delete()
        return result

    def solve_cadical(self):
        return self._solve_with_cpp(use_propagator=True)

