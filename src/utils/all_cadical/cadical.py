# target: run a cadical solver on a given cnf file
# input: input.txt
# output: output.txt

from pysat.solvers import Solver, Cadical195
import time

sat_solver: Solver = Cadical195()

with open("src/utils/all_cadical/input.txt", 'r') as lines:
    for line in lines:
        # change the line to array of int
        arr = list(map(int, line.split()))
        # add the clause to the solver
        sat_solver.add_clause(arr)

with open("src/utils/all_cadical/output.txt", 'w') as writer: writer.write("-1\n")

start_time = time.time()
result = sat_solver.solve()
elapsed_time = float(format(time.time() - start_time, ".3f"))

with open("src/utils/all_cadical/output.txt", 'w') as writer:
    print("Result: ", result)
    if result is False:
        writer.write("0\n")
        writer.write(f"{elapsed_time}")
    else:
        writer.write("1\n")
        writer.write(f"{elapsed_time}\n")
        # get the model
        model = sat_solver.get_model()
        writer.write(" ".join(map(str, model)))
    