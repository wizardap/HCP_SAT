#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <utility>

#ifndef LFSR_STEP_DEFINED
#define LFSR_STEP_DEFINED
inline int lfsr_step(int n, int size, int xor_tap) {
    int m = n << 1;
    int x = m & (1 << size);
    if (x) {
        m = m - x + 1;
    }
    m ^= x >> xor_tap;
    return m;
}
#endif

struct CRTBlock {
    int bits;
    int clauses_per_edge;
    int period;
    std::string name;
    int xor_tap;
};

class OptimizedCRT : public SuccessorMethod {
private:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> P;
    std::vector<CRTBlock> blocks;

public:
    OptimizedCRT(CardinalityOneEncoder* enc = nullptr) : SuccessorMethod(enc) {
        blocks = {
            {1, 2, 2, "Mod2", -1},
            {2, 6, 3, "Size2", 1},
            {3, 8, 7, "Size3", 1},
            {5, 12, 31, "Size5", 2},
            {7, 16, 127, "Size7", 1},
            {11, 24, 2047, "Size11", 2}
        };
    }

    int getH(int i, int j) override {
        auto key = std::make_pair(i, j);
        if (H.find(key) == H.end()) {
            H[key] = new_var();
        }
        return H[key];
    }

    int getP(int i, int bit_idx) {
        auto key = std::make_pair(i, bit_idx);
        if (P.find(key) == P.end()) {
            P[key] = new_var();
        }
        return P[key];
    }

    std::vector<CRTBlock> get_optimal_combo(int nNode) {
        int best_bits = 999;
        int best_clauses = 999;
        std::vector<CRTBlock> best_combo;
        bool found = false;

        int num_blocks = blocks.size();
        for (int mask = 1; mask < (1 << num_blocks); ++mask) {
            long long prod = 1;
            int bits = 0;
            int clauses = 0;
            std::vector<CRTBlock> current_combo;
            for (int i = 0; i < num_blocks; ++i) {
                if ((mask & (1 << i))) {
                    prod *= blocks[i].period;
                    bits += blocks[i].bits;
                    clauses += blocks[i].clauses_per_edge;
                    current_combo.push_back(blocks[i]);
                }
            }
            if (prod >= nNode) {
                if (bits < best_bits || (bits == best_bits && clauses < best_clauses)) {
                    best_bits = bits;
                    best_clauses = clauses;
                    best_combo = current_combo;
                    found = true;
                }
            }
        }
        if (!found) {
            throw std::runtime_error("Graph too large, max period is 169M");
        }
        return best_combo;
    }

    void enforce_non_zero_lfsr(int n, const std::vector<CRTBlock>& combo) {
        for (int i = 1; i <= n; ++i) {
            int b = 0;
            for (const auto& c : combo) {
                int bits = c.bits;
                std::string name = c.name;
                if (name == "Mod2") {
                    b += 1;
                } else {
                    std::vector<int> clause;
                    for (int j = 0; j < bits; ++j) {
                        clause.push_back(getP(i, b + j));
                    }
                    add_clause(clause);
                    b += bits;
                }
            }
        }
    }

    void init_starting_position(int first, const std::vector<CRTBlock>& combo) {
        int b = 0;
        for (const auto& c : combo) {
            int bits = c.bits;
            std::string name = c.name;
            if (name == "Mod2") {
                add_clause({-getP(first, b)});
                b += 1;
            } else {
                add_clause({getP(first, b)});
                for (int j = 1; j < bits; ++j) {
                    add_clause({-getP(first, b + j)});
                }
                b += bits;
            }
        }
    }

    void init_termination_position(int n, int first, const std::vector<int>& first_neighbors, const std::vector<CRTBlock>& combo) {
        for (int neighbor : first_neighbors) {
            int b = 0;
            for (const auto& c : combo) {
                int bits = c.bits;
                int period = c.period;
                std::string name = c.name;
                int xor_tap = c.xor_tap;

                if (name == "Mod2") {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if (((n - 1) % 2) == 0) {
                        clause.push_back(-getP(neighbor, b));
                    } else {
                        clause.push_back(getP(neighbor, b));
                    }
                    add_clause(clause);
                    b += 1;
                } else {
                    int mask = 1;
                    for (int step = 0; step < (n - 1) % period; ++step) {
                        mask = lfsr_step(mask, bits, xor_tap);
                    }

                    for (int i = 0; i < bits; ++i) {
                        std::vector<int> clause = {-getH(neighbor, first)};
                        if ((mask & 1) == 0) {
                            clause.push_back(-getP(neighbor, b + i));
                        } else {
                            clause.push_back(getP(neighbor, b + i));
                        }
                        add_clause(clause);
                        mask = mask >> 1;
                    }
                    b += bits;
                }
            }
        }
    }

    void add_transitions(int n, const Graph& graph, int first, const std::vector<CRTBlock>& combo) {
        for (int i = 1; i <= n; ++i) {
            if (i > (int)graph.adj_list.size()) continue;
            for (int j : graph.adj_list[i]) {
                if (j != first) {
                    int b = 0;
                    for (const auto& c : combo) {
                        int bits = c.bits;
                        std::string name = c.name;
                        int xor_tap = c.xor_tap;

                        if (name == "Mod2") {
                            add_clause({-getH(i, j), getP(j, b), getP(i, b)});
                            add_clause({-getH(i, j), -getP(j, b), -getP(i, b)});
                            b += 1;
                        } else {
                            add_clause({-getH(i, j), getP(j, b), -getP(i, b + bits - 1)});
                            add_clause({-getH(i, j), -getP(j, b), getP(i, b + bits - 1)});

                            for (int idx = 1; idx < bits; ++idx) {
                                if (idx == bits - xor_tap) {
                                    add_clause({-getH(i, j), getP(j, b + idx), getP(i, b + idx - 1), -getP(i, b + bits - 1)});
                                    add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1), getP(i, b + bits - 1)});
                                    add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1), getP(i, b + bits - 1)});
                                    add_clause({-getH(i, j), -getP(j, b + idx), -getP(i, b + idx - 1), -getP(i, b + bits - 1)});
                                } else {
                                    add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                                    add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                                }
                            }
                            b += bits;
                        }
                    }
                }
            }
        }
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        int n = graph.v;
        int first = graph.start_vertex;

        auto combo = get_optimal_combo(n);

        // 1. Non-edges are set to False
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                if (i <= (int)graph.adj_list.size()) {
                    const auto& neighbors = graph.adj_list[i];
                    if (std::find(neighbors.begin(), neighbors.end(), j) == neighbors.end()) {
                        add_clause({-getH(i, j)});
                    }
                }
            }
        }

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
        enforce_non_zero_lfsr(n, combo);

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
        init_starting_position(first, combo);

        // 8. Initialize termination position
        init_termination_position(n, first, first_neighbors, combo);

        // 9. Add transitions
        add_transitions(n, graph, first, combo);

        return context;
    }
};
