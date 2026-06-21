#pragma once
#include "CRT.hpp"

class PruningCRT : public CRT {
public:
    PruningCRT(CardinalityOneEncoder* enc = nullptr, int cycle_len = -1) : CRT(enc, cycle_len) {}

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        H.clear();
        P.clear();
        int n = graph.v;
        int first = graph.start_vertex;

        auto [cycle_len, nBits] = get_cycle_and_nbits(n);
        cycle = cycle_len;

        // 2. Exactly one outgoing edge
        for (int i = 1; i <= n; ++i) {
            std::vector<int> literals;
            if (i <= (int)graph.adj_list.size()) {
                for (int j : graph.adj_list[i]) {
                    literals.push_back(getH(i, j));
                }
            }
            exactly_one_constraint(literals);
        }

        // 3. Exactly one incoming edge
        for (int j = 1; j <= n; ++j) {
            std::vector<int> literals;
            for (int i = 1; i <= n; ++i) {
                if (i <= (int)graph.adj_list.size()) {
                    const auto& neighbors = graph.adj_list[i];
                    if (std::find(neighbors.begin(), neighbors.end(), j) != neighbors.end()) {
                        literals.push_back(getH(i, j));
                    }
                }
            }
            exactly_one_constraint(literals);
        }

        // 4. Enforce non-zero representation
        enforce_non_zero_lfsr(n, cycle_len, nBits);

        // 5. Connect one neighbor back to start (final edge)
        std::vector<int> first_neighbors;
        for (int u = 1; u <= n; ++u) {
            if (u <= (int)graph.adj_list.size()) {
                const auto& neighbors = graph.adj_list[u];
                if (std::find(neighbors.begin(), neighbors.end(), first) != neighbors.end()) {
                    first_neighbors.push_back(u);
                }
            }
        }

        std::vector<int> first_literals;
        for (int neighbor : first_neighbors) {
            first_literals.push_back(getH(neighbor, first));
        }
        add_clause(first_literals);

        // 6. Symmetry breaking
        if (!graph.is_directed) {
            for (size_t i = 0; i < first_neighbors.size(); ++i) {
                std::vector<int> clause;
                for (size_t j = 0; j < i; ++j) {
                    clause.push_back(getH(first, first_neighbors[j]));
                }
                clause.push_back(-getH(first_neighbors[i], first));
                add_clause(clause);
            }
        }

        // 7. Initialize starting position
        init_starting_position(first, cycle_len);

        // 8. Initialize termination position
        init_termination_position(n, first, first_neighbors, cycle_len);

        // 9. Add transitions
        add_transitions(n, graph, first, cycle_len);

        return context;
    }
};
