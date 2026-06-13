import os
import sys
import argparse

# Automatically add the current directory (src) to the Python path
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

from libs.encodings.cardinality_one.bimander import BimanderEncoder
from libs.encodings.cardinality_one.binomial import BinomialEncoder
from libs.encodings.cardinality_one.binary import BinaryEncoder
from libs.encodings.cardinality_one.commander import CommanderEncoder
from libs.encodings.cardinality_one.product import ProductEncoder
from libs.encodings.cardinality_one.sequential import SequentialEncoder
from libs.encodings.cardinality_one.hybrid import HybridEncoder
from libs.encodings.cardinality_one.scl import NSCEncoding
from libs.encodings.cardinality_one.pblib import PbLibEncoder
from libs.encodings.cardinality_one.heule import HeuleEncoder
from libs.models.nqueens import NQueensModel
from pysat.solvers import Glucose4


def print_board(solution):
    if solution is None:
        print("No solution found.")
    else:
        for row in solution:
            print(" ".join("Q" if cell == 1 else "." for cell in row))


def check_valid(solution):
    if solution is None:
        return False
    n = len(solution)
    # Check rows
    for i in range(n):
        if solution[i].count(1) != 1:
            print(f"Row conflict at row: {i}")
            return False
    # Check columns
    for j in range(n):
        if [solution[i][j] for i in range(n)].count(1) != 1:
            print(f"Column conflict at col: {j}")
            return False
    # Check diagonals
    for i in range(n):
        for j in range(n):
            if solution[i][j] == 1:
                for k in range(1, n):
                    # Main diagonal down-right
                    if i + k < n and j + k < n and solution[i + k][j + k] == 1:
                        return False
                    # Anti-diagonal down-left
                    if i + k < n and j - k >= 0 and solution[i + k][j - k] == 1:
                        return False
    return True


def main():
    parser = argparse.ArgumentParser(description="Solve N-Queens using SAT and Polymorphic Encodings.")
    parser.add_argument("-n", "--size", type=int, default=8, help="Size of the board (default: 8)")
    parser.add_argument("-e", "--encoder", choices=["bimander", "binomial", "binary", "commander", "product", "sequential", "hybrid", "pblib", "scl", "heule"], default="bimander", help="Polymorphic encoder to use (default: bimander)")
    parser.add_argument("-m", "--groups", type=int, default=None, help="Number of groups for Bimander (default: auto)")

    args = parser.parse_args()
    n = args.size
    m = args.groups
    encoder_type = args.encoder

    if encoder_type == "bimander":
        print(f"Configuring N-Queens (N={n}) with BimanderEncoder (m={m if m is not None else 'auto'})...")
        encoder = BimanderEncoder(m=m)
    elif encoder_type == "binomial":
        print(f"Configuring N-Queens (N={n}) with BinomialEncoder...")
        encoder = BinomialEncoder()
    elif encoder_type == "binary":
        print(f"Configuring N-Queens (N={n}) with BinaryEncoder...")
        encoder = BinaryEncoder()
    elif encoder_type == "commander":
        print(f"Configuring N-Queens (N={n}) with CommanderEncoder...")
        encoder = CommanderEncoder()
    elif encoder_type == "product":
        print(f"Configuring N-Queens (N={n}) with ProductEncoder...")
        encoder = ProductEncoder()
    elif encoder_type == "sequential":
        print(f"Configuring N-Queens (N={n}) with SequentialEncoder...")
        encoder = SequentialEncoder()
    elif encoder_type == "hybrid":
        print(f"Configuring N-Queens (N={n}) with HybridEncoder...")
        encoder = HybridEncoder()
    elif encoder_type == "pblib":
        print(f"Configuring N-Queens (N={n}) with PbLibEncoder...")
        encoder = PbLibEncoder()
    elif encoder_type == "heule":
        print(f"Configuring N-Queens (N={n}) with HeuleEncoder...")
        encoder = HeuleEncoder()
    else:
        print(f"Configuring N-Queens (N={n}) with NSCEncoding...")
        encoder = NSCEncoding()

    model = NQueensModel(n, encoder)

    # Encode constraints
    context = model.encode()
    print("Encoding statistics:")
    print(f"  - Variables: {context.total_vars()}")
    print(f"  - Clauses: {context.total_clauses()}")

    # Solve with Glucose4
    solver = Glucose4()
    solver.append_formula(context.cnf.clauses)

    print("Solving SAT formula...")
    if solver.solve():
        model_vars = solver.get_model()
        solution = [[0] * n for _ in range(n)]

        # Interpret grid variables from SAT model
        for r in range(n):
            for c in range(n):
                sat_var = model.grid[r][c]
                if sat_var in model_vars:
                    solution[r][c] = 1

        print("\nSolution Board:")
        print_board(solution)

        if check_valid(solution):
            print("\nResult: Solution is VALID!")
        else:
            print("\nResult: Solution is INVALID!")
    else:
        print(f"\nResult: No solution exists for N = {n}")


if __name__ == "__main__":
    main()
