import os
import sys
import argparse
import re

# Automatically add the current directory (src) to the Python path
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))


def get_files_from_folder(folder_path):
    def extract_numbers(filename):
        numbers = re.findall(r'\d+', filename)
        return list(map(int, numbers))

    return sorted(
        [entry.path.replace("\\", "/") for entry in os.scandir(folder_path) if entry.is_file()],
        key=lambda x: extract_numbers(os.path.basename(x))
    )


def run_original_solver(successor_name, encoder_name, graph_path):
    ref_path = os.path.abspath("src_ref/SAT_HCP/src")
    if ref_path not in sys.path:
        sys.path.insert(0, ref_path)

    # Import original classes
    from Graph import Graph as RefGraph
    from HcpSolver import HcpSolver as RefHcpSolver

    # Map successor
    if successor_name == "unary":
        from successor.successorMethod.Unary import Unary as RefUnary
        successor = RefUnary()
    elif successor_name == "lfsr":
        from successor.successorMethod.LFSR import LFSR as RefLFSR
        successor = RefLFSR()
    else:
        from successor.successorMethod.BinaryAdder import BinaryAdder as RefBinaryAdder
        successor = RefBinaryAdder()

    # Map encoder
    if encoder_name == "binomial":
        from exactlyone.exactlyoneMethod.EOBinomial import EOBinomial as RefEnc
    elif encoder_name == "binary":
        from exactlyone.exactlyoneMethod.EOBinary import EOBinary as RefEnc
    elif encoder_name == "commander":
        from exactlyone.exactlyoneMethod.EOCommander import EOCommander as RefEnc
    elif encoder_name == "product":
        from exactlyone.exactlyoneMethod.EOProduct import EOProduct as RefEnc
    elif encoder_name == "sequential":
        from exactlyone.exactlyoneMethod.EOSequentialEncounter import EOSequentialEncounter as RefEnc
    elif encoder_name == "pblib":
        from exactlyone.exactlyoneMethod.EOPbLib import EOPbLib as RefEnc
    elif encoder_name == "hybrid":
        from exactlyone.exactlyoneMethod.EOHybrid import EOHybrid as RefEnc
    else:
        # Original doesn't support scl or bimander
        # Clean up path and raise
        if ref_path in sys.path:
            sys.path.remove(ref_path)
        raise ValueError(f"Original solver does not support encoder: {encoder_name}")

    encoder = RefEnc()
    hcpSolver = RefHcpSolver(successor, encoder)
    hcpSolver.graph.load_graph_from_file(graph_path)

    cnf = hcpSolver.hcpCnf
    cnf = hcpSolver.successor.build_clauses(cnf, hcpSolver.graph)
    result = hcpSolver.solve(cnf)

    if result["status"] == "SAT":
        hcpSolver.print_result(result["model"], hcpSolver.graph, hcpSolver.successor.getH, result)

    # Clean up sys.path and sys.modules
    if ref_path in sys.path:
        sys.path.remove(ref_path)
    for k in list(sys.modules.keys()):
        if any(prefix in k for prefix in ["Graph", "HcpSolver", "successor.", "exactlyone.", "utils."]):
            del sys.modules[k]

    return result


def run_new_solver(successor_name, encoder_name, graph_path):
    src_path = os.path.abspath("src")
    if src_path not in sys.path:
        sys.path.insert(0, src_path)

    from libs.utils.graph import Graph as NewGraph
    from solver import HcpSolver as NewHcpSolver

    # Map successor
    from libs.models.successor.unary import Unary as NewUnary
    from libs.models.successor.lfsr import LFSR as NewLFSR
    from libs.models.successor.binary_adder import BinaryAdder as NewBinaryAdder
    from libs.models.successor.binary_adder_original import BinaryAdderOriginal as NewBinaryAdderOriginal
    from libs.models.successor.vee import Vee as NewVee

    if successor_name == "unary":
        successor = NewUnary()
    elif successor_name == "lfsr":
        successor = NewLFSR()
    elif successor_name == "binary_adder_original":
        successor = NewBinaryAdderOriginal()
    elif successor_name == "vee":
        successor = NewVee()
    else:
        successor = NewBinaryAdder()

    # Map encoder
    from libs.encodings.cardinality_one.bimander import BimanderEncoder
    from libs.encodings.cardinality_one.binomial import BinomialEncoder
    from libs.encodings.cardinality_one.binary import BinaryEncoder
    from libs.encodings.cardinality_one.commander import CommanderEncoder
    from libs.encodings.cardinality_one.product import ProductEncoder
    from libs.encodings.cardinality_one.sequential import SequentialEncoder
    from libs.encodings.cardinality_one.hybrid import HybridEncoder
    from libs.encodings.cardinality_one.pblib import PbLibEncoder
    from libs.encodings.cardinality_one.scl import NSCEncoding
    from libs.encodings.cardinality_one.heule import HeuleEncoder

    if encoder_name == "bimander":
        encoder = BimanderEncoder()
    elif encoder_name == "binomial":
        encoder = BinomialEncoder()
    elif encoder_name == "binary":
        encoder = BinaryEncoder()
    elif encoder_name == "commander":
        encoder = CommanderEncoder()
    elif encoder_name == "product":
        encoder = ProductEncoder()
    elif encoder_name == "sequential":
        encoder = SequentialEncoder()
    elif encoder_name == "hybrid":
        encoder = HybridEncoder()
    elif encoder_name == "pblib":
        encoder = PbLibEncoder()
    elif encoder_name == "scl":
        encoder = NSCEncoding()
    elif encoder_name == "heule":
        encoder = HeuleEncoder()

    hcpSolver = NewHcpSolver(successor, encoder)
    hcpSolver.graph.load_graph_from_file(graph_path)
    result = hcpSolver.solve()

    if result["status"] == "SAT":
        hcpSolver.print_result(result["model"], hcpSolver.successor.getH, result)

    # Clean up sys.path
    if src_path in sys.path:
        sys.path.remove(src_path)

    return result


def main():
    parser = argparse.ArgumentParser(description="Compare Original vs Refactored HCP SAT Solver.")
    parser.add_argument("-s", "--successor", choices=["unary", "lfsr", "binary_adder", "binary_adder_original", "vee"], default="binary_adder", help="Successor method (default: binary_adder)")
    parser.add_argument("-e", "--encoder", choices=["binomial", "binary", "commander", "product", "sequential", "hybrid", "pblib", "heule"], default="pblib", help="Card-one encoder supported by both solvers")
    parser.add_argument("-d", "--data", type=str, default="src/data/fhcpcs", help="Folder containing graph instances (default: src/data/fhcpcs)")
    parser.add_argument("-n", "--limit", type=int, default=3, help="Limit files to run (default: 3)")

    args = parser.parse_args()
    successor_type = args.successor
    encoder_type = args.encoder
    data_folder = args.data
    limit = args.limit

    if not os.path.exists(data_folder):
        print(f"Data folder {data_folder} not found.")
        sys.exit(1)

    listFiles = get_files_from_folder(data_folder)
    if not listFiles:
        print(f"No files found in {data_folder}.")
        sys.exit(0)

    print(f"Comparing solvers on first {min(limit, len(listFiles))} instances...")
    print(f"Successor: {successor_type.upper()} | Encoder: {encoder_type.upper()}")
    print("-" * 110)

    results = []

    for path in listFiles[:limit]:
        filename = path.split("/")[-1]
        print(f"\nRunning original solver on: {filename}")
        orig_res = run_original_solver(successor_type, encoder_type, path)

        print(f"Running new refactored solver on: {filename}")
        new_res = run_new_solver(successor_type, encoder_type, path)

        results.append({
            "filename": filename,
            "orig_vars": orig_res["nofVariables"],
            "new_vars": new_res["nofVariables"],
            "orig_clauses": orig_res["nofClauses"],
            "new_clauses": new_res["nofClauses"],
            "orig_time": orig_res["time"],
            "new_time": new_res["time"],
            "orig_status": orig_res["status"],
            "new_status": new_res["status"]
        })

    print("\n" + "=" * 45 + " COMPARISON RESULTS " + "=" * 45)
    print(f"{'File Name':<20} | {'Vars (Orig / New)':<20} | {'Clauses (Orig / New)':<24} | {'Time (Orig / New)':<20} | {'Status'}")
    print("-" * 110)
    for r in results:
        vars_str = f"{r['orig_vars']} / {r['new_vars']}"
        clauses_str = f"{r['orig_clauses']} / {r['new_clauses']}"
        time_str = f"{r['orig_time']:.3f}s / {r['new_time']:.3f}s"
        status_str = f"{r['orig_status']} / {r['new_status']}"
        print(f"{r['filename']:<20} | {vars_str:<20} | {clauses_str:<24} | {time_str:<20} | {status_str}")
    print("=" * 110)


if __name__ == "__main__":
    main()
