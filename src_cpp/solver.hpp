#pragma once
#include "libs/utils/Graph.hpp"
#include "libs/utils/EncodingContext.hpp"
#include "libs/encodings/cardinality_one/CardinalityOneEncoder.hpp"
#include "libs/models/successor/SuccessorMethod.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

struct SolverResult {
    int nofVariables = 0;
    int nofClauses = 0;
    std::string status = "TIMEOUT";
    std::vector<int> model;
    double time = 0.0;
    int vOfHC = 0;
};

class HcpSolver {
protected:
    SuccessorMethod* successor = nullptr;
    CardinalityOneEncoder* encoder = nullptr;
    EncodingContext context;
    int opt_level = 1;

public:
    HcpSolver(SuccessorMethod* succ, CardinalityOneEncoder* enc, int opt_lvl = 1) : successor(succ), encoder(enc), opt_level(opt_lvl) {
        if (successor) {
            successor->set_encoder(encoder);
        }
    }
    virtual ~HcpSolver() = default;

    virtual SolverResult solve(const Graph& graph, int timeout_seconds = 600, int seed = 0, int assume_u = 0, int assume_v = 0);
    void print_result(const Graph& graph, const SolverResult& result);
};

class IncHcpSolver : public HcpSolver {
public:
    using HcpSolver::HcpSolver;

    // solver_mode: 0 = Propagator, 1 = CEGAR
    SolverResult solve_incremental(const Graph& graph, int solver_mode = 0, int timeout_seconds = 600, int seed = 0, int assume_u = 0, int assume_v = 0);
    std::vector<std::vector<std::pair<int, int>>> find_all_cycles(const Graph& graph, const std::vector<int>& model);
};
