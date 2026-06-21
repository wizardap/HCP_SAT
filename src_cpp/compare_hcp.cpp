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

#include "libs/utils/Common.hpp"
#include "solver.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

std::unique_ptr<CardinalityOneEncoder> make_encoder(const std::string& type);
std::unique_ptr<SuccessorMethod> make_successor(const std::string& type, CardinalityOneEncoder* encoder, int cycle_len);

struct CompareRow {
    std::string filename;
    int py_vars = 0;
    int cpp_vars = 0;
    int py_clauses = 0;
    int cpp_clauses = 0;
    double py_time = 0.0;
    double cpp_time = 0.0;
    std::string py_status;
    std::string cpp_status;
};

int main(int argc, char* argv[]) {
    std::string successor_type = "binary_adder";
    std::string encoder_type = "pblib";
    std::string data_path = "src/data/fhcpcs";
    int limit = 3;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--successor") {
            if (i + 1 < argc) successor_type = argv[++i];
        } else if (arg == "-e" || arg == "--encoder") {
            if (i + 1 < argc) encoder_type = argv[++i];
        } else if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) data_path = argv[++i];
        } else if (arg == "-n" || arg == "--limit") {
            if (i + 1 < argc) limit = std::stoi(argv[++i]);
        }
    }

    auto listFiles = get_files_from_folder(data_path);
    if (listFiles.empty()) {
        std::cout << "No graph instances found in: " << data_path << std::endl;
        return 0;
    }

    int files_to_run = std::min(limit, (int)listFiles.size());
    std::cout << "Comparing solvers on first " << files_to_run << " instances..." << std::endl;
    std::cout << "Successor: " << successor_type << " | Encoder: " << encoder_type << std::endl;
    std::cout << std::string(110, '-') << std::endl;

    std::vector<CompareRow> rows;

    for (int i = 0; i < files_to_run; ++i) {
        std::string path = listFiles[i];
        std::string filename = fs::path(path).filename().string();

        // 1. Run Python solver as subprocess and redirect log
        std::remove("temp_py_log.txt");
        std::string py_cmd = "python3 src/test_hcp.py -s " + successor_type + " -e " + encoder_type + " -d " + path + " -l temp_py_log.txt > /dev/null 2>&1";
        std::system(py_cmd.c_str());

        CompareRow row;
        row.filename = filename;

        // Parse temp_py_log.txt
        std::ifstream py_log("temp_py_log.txt");
        if (py_log.is_open()) {
            std::string line;
            if (std::getline(py_log, line)) {
                std::stringstream ss(line);
                std::string fname, vars, clauses, status, time;
                ss >> fname >> vars >> clauses >> status >> time;
                row.py_vars = std::stoi(vars);
                row.py_clauses = std::stoi(clauses);
                row.py_status = status;
                row.py_time = std::stod(time);
            }
            py_log.close();
        }
        std::remove("temp_py_log.txt");

        // 2. Run new C++ solver
        auto encoder = make_encoder(encoder_type);
        int cycle_override = -1;
        if (successor_type == "inc_crt" || successor_type == "inc_ocrt") {
            cycle_override = 12;
        }
        auto successor = make_successor(successor_type, encoder.get(), cycle_override);

        Graph graph;
        graph.load_graph_from_file(path);

        SolverResult cpp_res;
        if (successor->is_incremental) {
            IncHcpSolver solver(successor.get(), encoder.get());
            cpp_res = solver.solve_incremental(graph, 0); // Propagator mode
        } else {
            HcpSolver solver(successor.get(), encoder.get());
            cpp_res = solver.solve(graph);
        }

        row.cpp_vars = cpp_res.nofVariables;
        row.cpp_clauses = cpp_res.nofClauses;
        row.cpp_status = cpp_res.status;
        row.cpp_time = cpp_res.time;

        rows.push_back(row);
    }

    std::cout << "\n" << std::string(45, '=') << " COMPARISON RESULTS " << std::string(45, '=') << std::endl;
    std::cout << std::left << std::setw(20) << "File Name" << " | "
              << std::setw(20) << "Vars (Py / C++)" << " | "
              << std::setw(24) << "Clauses (Py / C++)" << " | "
              << std::setw(20) << "Time (Py / C++)" << " | "
              << "Status" << std::endl;
    std::cout << std::string(110, '-') << std::endl;

    for (const auto& r : rows) {
        std::string vars_str = std::to_string(r.py_vars) + " / " + std::to_string(r.cpp_vars);
        std::string clauses_str = std::to_string(r.py_clauses) + " / " + std::to_string(r.cpp_clauses);
        
        std::stringstream time_ss;
        time_ss << std::fixed << std::setprecision(3) << r.py_time << "s / " << r.cpp_time << "s";
        std::string time_str = time_ss.str();

        std::string status_str = r.py_status + " / " + r.cpp_status;

        std::cout << std::left << std::setw(20) << r.filename << " | "
                  << std::setw(20) << vars_str << " | "
                  << std::setw(24) << clauses_str << " | "
                  << std::setw(20) << time_str << " | "
                  << status_str << std::endl;
    }
    std::cout << std::string(110, '=') << std::endl;

    return 0;
}

// Re-using implementations from test_hcp.cpp
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
    if (type == "binary_adder_original") return std::make_unique<BinaryAdderOriginal>(encoder);
    if (type == "binary_adder") return std::make_unique<BinaryAdder>(encoder);
    throw std::runtime_error("Unknown successor: " + type);
}
