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
#include <set>
#include <unordered_map>

bool verify_hamiltonian_cycle(const std::vector<std::pair<int, int>>& HCP, const Graph& graph, int N, std::string& err_msg) {
    if ((int)HCP.size() != N) {
        err_msg = "Incorrect number of edges: got " + std::to_string(HCP.size()) + ", expected " + std::to_string(N);
        return false;
    }

    std::unordered_map<int, int> out_degree;
    std::unordered_map<int, int> in_degree;
    std::unordered_map<int, int> adj;
    for (const auto& edge : HCP) {
        out_degree[edge.first]++;
        in_degree[edge.second]++;
        adj[edge.first] = edge.second;
    }

    for (int i = 1; i <= N; ++i) {
        if (out_degree[i] != 1) {
            err_msg = "Vertex " + std::to_string(i) + " has out-degree " + std::to_string(out_degree[i]) + " (expected 1)";
            return false;
        }
        if (in_degree[i] != 1) {
            err_msg = "Vertex " + std::to_string(i) + " has in-degree " + std::to_string(in_degree[i]) + " (expected 1)";
            return false;
        }
    }

    std::set<int> visited;
    int curr = 1;
    for (int step = 0; step < N; ++step) {
        if (visited.count(curr)) {
            err_msg = "Sub-cycle detected (loop at vertex " + std::to_string(curr) + ")";
            return false;
        }
        visited.insert(curr);
        if (!adj.count(curr)) {
            err_msg = "No outgoing edge from vertex " + std::to_string(curr);
            return false;
        }
        curr = adj[curr];
    }

    if (curr != 1) {
        err_msg = "Cycle did not return to start vertex (ended at " + std::to_string(curr) + ")";
        return false;
    }
    if ((int)visited.size() != N) {
        err_msg = "Only visited " + std::to_string(visited.size()) + " vertices (expected " + std::to_string(N) + ")";
        return false;
    }

    for (const auto& edge : HCP) {
        int u = edge.first;
        int v = edge.second;
        if (u > (int)graph.adj_list.size()) {
            err_msg = "Vertex " + std::to_string(u) + " is out of bounds";
            return false;
        }
        const auto& neighbors = graph.adj_list[u];
        if (std::find(neighbors.begin(), neighbors.end(), v) == neighbors.end()) {
            err_msg = "Edge (" + std::to_string(u) + ", " + std::to_string(v) + ") does not exist in the original graph";
            return false;
        }
    }

    return true;
}

struct BenchmarkResult {
    std::string name;
    int vars = 0;
    int clauses = 0;
    double build_time = 0.0;
    double solve_time = 0.0;
    std::string verification = "N/A";
};

// Inline helper for lowercase conversion
inline std::string successor_type_to_lower(const std::string& name) {
    if (name == "Unary") return "unary";
    if (name == "LFSR") return "lfsr";
    if (name == "BinaryAdder") return "binary_adder";
    if (name == "BinaryAdderOriginal") return "binary_adder_original";
    if (name == "CRT") return "crt";
    if (name == "IncCRT") return "inc_crt";
    if (name == "Vee") return "vee";
    if (name == "OCRT") return "ocrt";
    if (name == "IncOCRT") return "inc_ocrt";
    if (name == "OUnary") return "ounary";
    if (name == "PruningCRT") return "pruning_crt";
    if (name == "IncPruningCRT") return "inc_pruning_crt";
    return "";
}

inline std::unique_ptr<SuccessorMethod> make_successor(const std::string& type, CardinalityOneEncoder* encoder, int cycle_len) {
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


int main(int argc, char* argv[]) {
    std::string graph_path = "src/data/fhcpcs/graph1.hcp";
    if (argc > 1) {
        graph_path = argv[1];
    }

    Graph graph;
    graph.load_graph_from_file(graph_path);

    std::cout << "Graph: " << fs::path(graph_path).filename().string() << std::endl;
    std::cout << "Vertices: " << graph.v << ", Edges: " << graph.e << std::endl;

    std::vector<std::pair<std::string, int>> successors_info = {
        {"Unary", -1},
        {"LFSR", -1},
        {"BinaryAdder", -1},
        {"BinaryAdderOriginal", -1},
        {"CRT", -1},
        {"IncCRT", 12},
        {"Vee", -1},
        {"OCRT", -1},
        {"IncOCRT", 12},
        {"OUnary", -1},
        {"PruningCRT", -1},
        {"IncPruningCRT", 12}
    };

    auto encoder = std::make_unique<BimanderEncoder>(); // bimander as default
    std::vector<BenchmarkResult> results;

    for (const auto& info : successors_info) {
        std::string name = info.first;
        int cycle_override = info.second;

        std::cout << "Running " << name << "..." << std::endl;
        auto successor = make_successor(successor_type_to_lower(name), encoder.get(), cycle_override);

        auto t0 = std::chrono::steady_clock::now();
        EncodingContext context;
        successor->build_clauses(&context, graph);
        auto t1 = std::chrono::steady_clock::now();
        double build_time = std::chrono::duration<double>(t1 - t0).count();

        SolverResult solve_res;
        if (successor->is_incremental) {
            IncHcpSolver solver(successor.get(), encoder.get());
            // Clear prior context and resolve on clean solver
            solve_res = solver.solve_incremental(graph, 0); // Propagator mode
        } else {
            HcpSolver solver(successor.get(), encoder.get());
            solve_res = solver.solve(graph);
        }

        std::string verif = solve_res.status;
        if (solve_res.status == "SAT" && !solve_res.model.empty()) {
            std::vector<std::pair<int, int>> HCP;
            for (int i = 1; i <= graph.v; ++i) {
                if (i <= (int)graph.adj_list.size()) {
                    for (int j : graph.adj_list[i]) {
                        if (solve_res.model[successor->getH(i, j) - 1] > 0) {
                            HCP.push_back({i, j});
                        }
                    }
                }
            }
            std::string err;
            if (verify_hamiltonian_cycle(HCP, graph, graph.v, err)) {
                verif = "SUCCESS";
            } else {
                verif = "FAILED";
                std::cerr << "Verification failed for " << name << ": " << err << std::endl;
            }
        }

        results.push_back({
            name,
            (int)solve_res.nofVariables,
            (int)solve_res.nofClauses,
            build_time,
            solve_res.time,
            verif
        });
    }

    std::string header = "Successor            | Vars     | Clauses    | Build (s)  | Solve (s)  | Status";
    std::cout << "\n" << std::string(header.length(), '=') << std::endl;
    std::cout << header << std::endl;
    std::cout << std::string(header.length(), '-') << std::endl;
    for (const auto& res : results) {
        std::cout << std::left << std::setw(20) << res.name << " | "
                  << std::setw(8) << res.vars << " | "
                  << std::setw(10) << res.clauses << " | "
                  << std::fixed << std::setprecision(4) << std::setw(10) << res.build_time << " | "
                  << std::setw(10) << res.solve_time << " | "
                  << std::setw(12) << res.verification << std::endl;
    }
    std::cout << std::string(header.length(), '=') << std::endl;

    return 0;
}

