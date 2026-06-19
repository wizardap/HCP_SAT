import sys, os, time, random
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "src")))
from libs.encodings.cardinality_one.scl import NSCEncoding
from solver import HcpSolver
from libs.models.successor.crt import CRT
from libs.models.successor.pruning_crt import PruningCRT

graph_path = "src/data/fhcppp/graph48.hcp"

print(f"Testing on {graph_path} with 5 different random seeds (clause shuffles)")
print(f"{'Model':15} | {'Seed':4} | {'Time (s)':10} | {'Status'}")
print("-" * 50)

for ModelName, ModelClass in [("CRT", CRT), ("PruningCRT", PruningCRT)]:
    # We will instantiate the models and build clauses once per seed
    # to ensure identical logic except for the shuffle
    for seed in range(5):
        random.seed(seed)
        
        enc = NSCEncoding()
        solver = HcpSolver(ModelClass(enc), enc)
        solver.graph.load_graph_from_file(graph_path)
        
        # Build clauses
        solver.successor.build_clauses(solver.context, solver.graph)
        cnf = solver.context.cnf.clauses
        
        # Shuffle clauses based on the seed
        random.shuffle(cnf)
        
        input_file = 'src/utils/all_cadical/input.txt'
        output_file = 'src/utils/all_cadical/output.txt'
        
        # Clean up old output
        if os.path.exists(output_file):
            os.remove(output_file)
            
        with open(input_file, 'w') as writer:
            for clause in cnf:
                writer.write(" ".join(map(str, clause)) + "\n")
                
        # Run cadical bypassing the iterative deepening in solver.py
        t0 = time.time()
        bashCommand = f"./src/utils/all_cadical/runlim -r 300 -o src/utils/all_cadical/report.txt {sys.executable} src/utils/all_cadical/cadical.py > /dev/null 2>&1"
        os.system(bashCommand)
        
        t_solve = time.time() - t0
        status = "UNKNOWN"
        try:
            with open(output_file, 'r') as f:
                lines = f.readlines()
            if len(lines) > 1 and lines[0].strip() != "-1":
                t_solve = float(lines[1].strip())
                status = "SAT" if lines[0].strip() == "1" else "UNSAT"
            else:
                status = "TIMEOUT/ERROR"
        except Exception:
            status = "TIMEOUT/ERROR"
            
        print(f"{ModelName:15} | {seed:4} | {t_solve:10.4f} | {status}")
