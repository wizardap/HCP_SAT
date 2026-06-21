#include "libs/encodings/cardinality_one/BimanderEncoder.hpp"
#include "libs/encodings/cardinality_one/BinomialEncoder.hpp"
#include "libs/encodings/cardinality_one/BinaryEncoder.hpp"
#include "libs/encodings/cardinality_one/CommanderEncoder.hpp"
#include "libs/encodings/cardinality_one/ProductEncoder.hpp"
#include "libs/encodings/cardinality_one/SequentialEncoder.hpp"
#include "libs/encodings/cardinality_one/HybridEncoder.hpp"
#include "libs/encodings/cardinality_one/NSCEncoding.hpp"
#include "libs/encodings/cardinality_one/PbLibEncoder.hpp"
#include "libs/encodings/cardinality_one/HeuleEncoder.hpp"

#include "libs/models/successor/Unary.hpp"
#include "libs/models/successor/LFSR.hpp"
#include "libs/models/successor/BinaryAdder.hpp"
#include "libs/models/successor/BinaryAdderOriginal.hpp"
#include "libs/models/successor/CRT.hpp"
#include "libs/models/successor/Vee.hpp"
#include "libs/models/successor/IncCRT.hpp"
#include "libs/models/successor/OCRT.hpp"
#include "libs/models/successor/IncOCRT.hpp"
#include "libs/models/successor/OUnary.hpp"
#include "libs/models/successor/PruningCRT.hpp"
#include "libs/models/successor/IncPruningCRT.hpp"

#include "libs/utils/Common.hpp"
#include "solver.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <algorithm>

std::unique_ptr<CardinalityOneEncoder> make_encoder(const std::string& type) {
    if (type == "bimander") return std::make_unique<BimanderEncoder>();
    if (type == "binomial") return std::make_unique<BinomialEncoder>();
    if (type == "binary") return std::make_unique<BinaryEncoder>();
    if (type == "commander") return std::make_unique<CommanderEncoder>();
    if (type == "product") return std::make_unique<ProductEncoder>();
    if (type == "sequential") return std::make_unique<SequentialEncoder>();
    if (type == "hybrid") return std::make_unique<HybridEncoder>();
    if (type == "pblib") return std::make_unique<PbLibEncoder>();
    if (type == "scl") return std::make_unique<NSCEncoding>();
    if (type == "heule") return std::make_unique<HeuleEncoder>();
    throw std::runtime_error("Unknown encoder: " + type);
}

std::unique_ptr<SuccessorMethod> make_successor(const std::string& type, CardinalityOneEncoder* encoder, int cycle_len) {
    if (type == "unary") return std::make_unique<Unary>(encoder);
    if (type == "lfsr") return std::make_unique<LFSR>(encoder);
    if (type == "crt") return std::make_unique<CRT>(encoder, cycle_len);
    if (type == "inc_crt") return std::make_unique<IncCRT>(encoder, cycle_len);
    if (type == "vee") return std::make_unique<Vee>(encoder);
    if (type == "ocrt") return std::make_unique<OCRT>(nullptr, encoder, cycle_len);
    if (type == "inc_ocrt") return std::make_unique<IncOCRT>(nullptr, encoder, cycle_len);
    if (type == "ounary") return std::make_unique<OUnary>(nullptr);
    if (type == "pruning_crt") return std::make_unique<PruningCRT>(encoder, cycle_len);
    if (type == "inc_pruning_crt") return std::make_unique<IncPruningCRT>(encoder, cycle_len);
    if (type == "binary_adder_original") return std::make_unique<BinaryAdderOriginal>(encoder);
    if (type == "binary_adder") return std::make_unique<BinaryAdder>(encoder);
    throw std::runtime_error("Unknown successor: " + type);
}

void print_help(char* argv[]) {
    std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -s, --successor <type>   Successor method: unary, lfsr, binary_adder, binary_adder_original, crt, vee, ocrt, inc_crt, inc_ocrt, pruning_crt, ounary (default: binary_adder)" << std::endl;
    std::cout << "  -e, --encoder <type>     Cardinality-one encoder: bimander, binomial, binary, commander, product, sequential, hybrid, pblib, scl, heule (default: bimander)" << std::endl;
    std::cout << "  -d, --data <path>        Folder containing graph instances or single file (default: src/data/fhcpcs)" << std::endl;
    std::cout << "  -l, --log <path>         Path to log file (default: log-file.txt)" << std::endl;
    std::cout << "  -c, --cycle <limit>      CRT cycle limit override" << std::endl;
    std::cout << "  -t, --timeout <seconds>  Solver timeout limit in seconds (default: 600)" << std::endl;
    std::cout << "  --solver-mode <0|1>      0 = Propagator (default), 1 = CEGAR (only for incremental solvers)" << std::endl;
    std::cout << "  --opt-level <0|1|2>      Optimization level (default: 1)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string successor_type = "binary_adder";
    std::string encoder_type = "bimander";
    std::string data_path = "src/data/fhcpcs";
    std::string log_file = "log-file.txt";
    int cycle_len = -1;
    int solver_mode = 0; // Propagator
    int timeout_seconds = 600;
    int opt_level = 1;
    int seed = 0;
    int assume_u = 0;
    int assume_v = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--successor") {
            if (i + 1 < argc) successor_type = argv[++i];
        } else if (arg == "-e" || arg == "--encoder") {
            if (i + 1 < argc) encoder_type = argv[++i];
        } else if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) data_path = argv[++i];
        } else if (arg == "-l" || arg == "--log") {
            if (i + 1 < argc) log_file = argv[++i];
        } else if (arg == "-c" || arg == "--cycle") {
            if (i + 1 < argc) cycle_len = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--timeout") {
            if (i + 1 < argc) timeout_seconds = std::stoi(argv[++i]);
        } else if (arg == "--solver-mode") {
            if (i + 1 < argc) solver_mode = std::stoi(argv[++i]);
        } else if (arg == "--opt-level") {
            if (i + 1 < argc) opt_level = std::stoi(argv[++i]);
        } else if (arg == "--seed") {
            if (i + 1 < argc) seed = std::stoi(argv[++i]);
        } else if (arg == "--assume") {
            if (i + 1 < argc) {
                std::string assume_str = argv[++i];
                size_t comma_pos = assume_str.find(',');
                if (comma_pos != std::string::npos) {
                    assume_u = std::stoi(assume_str.substr(0, comma_pos));
                    assume_v = std::stoi(assume_str.substr(comma_pos + 1));
                }
            }
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv);
            return 0;
        }
    }

    auto encoder = make_encoder(encoder_type);
    auto listFiles = get_files_from_folder(data_path);

    if (listFiles.empty()) {
        std::cout << "No graph instances found in: " << data_path << std::endl;
        return 0;
    }

    std::cout << "Found " << listFiles.size() << " graph instances in " << data_path << "." << std::endl;
    std::cout << "Using Successor: " << successor_type << " | Encoder: " << encoder_type << " | Opt-level: " << opt_level << std::endl;

    for (const auto& path : listFiles) {
        std::cout << "\n==================================================" << std::endl;
        std::cout << "Start build clauses for: " << fs::path(path).filename().string() << std::endl;

        int local_cycle = cycle_len;
        if (local_cycle == -1 && (successor_type == "inc_crt" || successor_type == "inc_ocrt" || successor_type == "inc_pruning_crt")) {
            local_cycle = 12;
        }

        auto successor = make_successor(successor_type, encoder.get(), local_cycle);
        
        Graph graph;
        graph.load_graph_from_file(path);
        
        int orig_v = graph.v;
        if (opt_level >= 1) {
            graph.preprocess();
            if (graph.v < orig_v) {
                std::cout << "Preprocessing (SOL-5): reduced vertices from " << orig_v << " to " << graph.v << std::endl;
            }
        }

        SolverResult result;
        if (successor->is_incremental) {
            IncHcpSolver solver(successor.get(), encoder.get(), opt_level);

            result = solver.solve_incremental(graph, solver_mode, timeout_seconds, seed, assume_u, assume_v);
            std::cout << "Solving finished with status: " << result.status << std::endl;
            std::cout << "  - Variables: " << result.nofVariables << std::endl;
            std::cout << "  - Clauses: " << result.nofClauses << std::endl;
            std::cout << "  - Solve Time: " << result.time << "s" << std::endl;
            if (result.status == "SAT") {
                solver.print_result(graph, result);
            }
        } else {
            HcpSolver solver(successor.get(), encoder.get(), opt_level);
            result = solver.solve(graph, timeout_seconds, seed, assume_u, assume_v);
            std::cout << "Solving finished with status: " << result.status << std::endl;
            std::cout << "  - Variables: " << result.nofVariables << std::endl;
            std::cout << "  - Clauses: " << result.nofClauses << std::endl;
            std::cout << "  - Solve Time: " << result.time << "s" << std::endl;
            if (result.status == "SAT") {
                solver.print_result(graph, result);
            }
        }

        if (!log_file.empty()) {
            std::ofstream log(log_file, std::ios::app);
            if (log.is_open()) {
                std::string filename = fs::path(path).filename().string();
                log << std::left << std::setw(30) << filename
                    << std::setw(10) << result.nofVariables
                    << std::setw(10) << result.nofClauses
                    << std::setw(10) << result.status
                    << std::setw(10) << result.time
                    << std::endl;
            }
        }
    }

    return 0;
}
