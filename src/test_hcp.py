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

from libs.models.successor.base import SuccessorMethod
from libs.models.successor.unary import Unary
from libs.models.successor.lfsr import LFSR
from libs.models.successor.binary_adder import BinaryAdder
from libs.models.successor.binary_adder_original import BinaryAdderOriginal
from libs.models.successor.crt import CRT
from libs.models.successor.vee import Vee
from libs.models.successor.inc_crt import IncCRT
from libs.models.successor.inc_ocrt import IncOCRT

from libs.utils.common import get_files_from_folder
from solver import HcpSolver, IncHcpSolver


def main():
    parser = argparse.ArgumentParser(description="Solve Hamiltonian Cycle Problem (HCP) using SAT Encodings.")
    parser.add_argument("-s", "--successor", choices=["unary", "lfsr", "binary_adder", "binary_adder_original", "crt", "vee", "inc_crt", "inc_ocrt"], default="binary_adder", help="Successor method to use (default: binary_adder)")
    parser.add_argument("-e", "--encoder", choices=["bimander", "binomial", "binary", "commander", "product", "sequential", "hybrid", "pblib", "scl", "heule"], default="bimander", help="Polymorphic encoder to use (default: bimander)")
    parser.add_argument("-d", "--data", type=str, default="src/data/fhcpcs", help="Folder containing graph instances (default: src/data/fhcpcs)")
    parser.add_argument("--solver", choices=["glucose", "cadical"], default="glucose", help="Solver backend (default: glucose)")
    parser.add_argument("-l", "--log", type=str, default="log-file.txt", help="Path to log file (default: log-file.txt)")

    args = parser.parse_args()
    successor_type = args.successor
    encoder_type = args.encoder
    data_folder = args.data
    solver_backend = args.solver
    log_file = args.log

    # 1. Instantiate Card-One Encoder
    if encoder_type == "bimander":
        encoder = BimanderEncoder()
    elif encoder_type == "binomial":
        encoder = BinomialEncoder()
    elif encoder_type == "binary":
        encoder = BinaryEncoder()
    elif encoder_type == "commander":
        encoder = CommanderEncoder()
    elif encoder_type == "product":
        encoder = ProductEncoder()
    elif encoder_type == "sequential":
        encoder = SequentialEncoder()
    elif encoder_type == "hybrid":
        encoder = HybridEncoder()
    elif encoder_type == "pblib":
        encoder = PbLibEncoder()
    elif encoder_type == "scl":
        encoder = NSCEncoding()
    elif encoder_type == "heule":
        encoder = HeuleEncoder()
    else:
        raise ValueError(f"Unknown encoder type: {encoder_type}")

    # 2. Get Files
    if not os.path.exists(data_folder):
        print(f"Data folder or file {data_folder} not found.")
        sys.exit(1)

    if os.path.isfile(data_folder):
        listFiles = [data_folder]
    else:
        listFiles = get_files_from_folder(data_folder)
    if not listFiles:
        print(f"No files found in {data_folder}.")
        sys.exit(0)

    print(f"Found {len(listFiles)} graph instances in {data_folder}.")
    print(f"Using Successor: {successor_type.upper()} | Encoder: {encoder_type.upper()} | Solver: {solver_backend.upper()}")

    for path in listFiles:  # Process all files
        print("\n" + "=" * 50)
        print("Start build clauses for:", path.split("/")[-1])

        # 3. Instantiate Successor Method
        if successor_type == "unary":
            successor = Unary()
        elif successor_type == "lfsr":
            successor = LFSR()
        elif successor_type == "crt":
            successor = CRT()
        elif successor_type == "inc_crt":
            successor = IncCRT()
        elif successor_type == "inc_ocrt":
            successor = IncOCRT(vee_encoder=NSCEncoding(), crt_encoder=encoder)
        elif successor_type == "binary_adder_original":
            successor = BinaryAdderOriginal()
        elif successor_type == "vee":
            successor = Vee()
        else:
            successor = BinaryAdder()

        # 4. Set up Solver
        if getattr(successor, 'is_incremental', False):
            hcpSolver = IncHcpSolver(successor, encoder)
        else:
            hcpSolver = HcpSolver(successor, encoder)
        hcpSolver.graph.load_graph_from_file(path)

        # 5. Solve
        if solver_backend == "glucose":
            result = hcpSolver.solve()
        else:
            result = hcpSolver.solve_cadical()

        # 6. Print Results
        print("Solving finished with status:", result["status"])
        print(f"  - Variables: {result['nofVariables']}")
        print(f"  - Clauses: {result['nofClauses']}")
        if result["time"] is not None:
            print(f"  - Solve Time: {result['time']}s")

        if result["status"] == "SAT":
            hcpSolver.print_result(result["model"], hcpSolver.successor.getH, result)
            status = "valid" if result['vOfHC'] == hcpSolver.graph.v else "invalid"
            print(f"Result verification: {status.upper()}")

        # 7. Save to log file
        if log_file:
            with open(log_file, "a") as f:
                filename = path.split("/")[-1]
                status_str = "valid" if result.get('vOfHC') == hcpSolver.graph.v else "None"
                f.write(f"{filename.ljust(30)} {str(result['nofVariables']).ljust(10)} {str(result['nofClauses']).ljust(10)} {result['status'].ljust(10)}  {str(result['time']).ljust(10)} {status_str.ljust(10)}\n")


if __name__ == "__main__":
    main()
