#pragma once
#include "SuccessorMethod.hpp"
#include <map>
#include <cmath>
#include <algorithm>

inline const std::map<int, std::vector<int>> LFSR_TAPS = {
    {2, {1, 0}},
    {3, {2, 0}},
    {4, {3, 0}},
    {5, {4, 1}},
    {6, {5, 0}},
    {7, {6, 2}},
    {8, {7, 3, 2, 1}},
    {9, {8, 3}},
    {10, {9, 2}},
    {11, {10, 1}},
    {12, {11, 5, 3, 0}},
    {13, {12, 3, 2, 0}},
    {14, {13, 9, 5, 0}},
    {15, {14, 0}},
    {16, {15, 11, 2, 0}}
};

inline std::vector<int> lfsr_next(const std::vector<int>& state, int m) {
    auto it = LFSR_TAPS.find(m);
    if (it == LFSR_TAPS.end()) return state;
    const auto& taps = it->second;
    int feedback = 0;
    for (int tap : taps) {
        feedback ^= state[tap];
    }
    std::vector<int> next_state;
    next_state.push_back(feedback);
    next_state.insert(next_state.end(), state.begin(), state.end() - 1);
    return next_state;
}

inline std::vector<int> get_target_state(int m, int step) {
    std::vector<int> state(m, 0);
    state[m - 1] = 1;
    for (int i = 0; i < step; ++i) {
        state = lfsr_next(state, m);
    }
    return state;
}

class LFSR : public SuccessorMethod {
private:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> P;

public:
    LFSR(CardinalityOneEncoder* enc = nullptr) : SuccessorMethod(enc) {}

    int getH(int i, int j) override {
        auto key = std::make_pair(i, j);
        if (H.find(key) == H.end()) {
            H[key] = new_var();
        }
        return H[key];
    }

    int getP(int i, int bit) {
        auto key = std::make_pair(i, bit);
        if (P.find(key) == P.end()) {
            P[key] = new_var();
        }
        return P[key];
    }

    void add_default_variables(int n, int m, const Graph& graph) {
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
        for (int bit = 0; bit < m; ++bit) {
            if (bit == m - 1) {
                add_clause({getP(1, bit)});
            } else {
                add_clause({-getP(1, bit)});
            }
        }
    }

    void vertex_outgoing_arcs(int n) {
        for (int i = 1; i <= n; ++i) {
            std::vector<int> literals;
            for (int j = 1; j <= n; ++j) {
                literals.push_back(getH(i, j));
            }
            exactly_one_constraint(literals);
        }
    }

    void vertex_incoming_arcs(int n) {
        for (int j = 1; j <= n; ++j) {
            std::vector<int> literals;
            for (int i = 1; i <= n; ++i) {
                literals.push_back(getH(i, j));
            }
            exactly_one_constraint(literals);
        }
    }

    void vertex_start(int n, int m) {
        for (int i = 2; i <= n; ++i) {
            for (int bit = 0; bit < m; ++bit) {
                if (bit == 0) {
                    add_clause({-getH(1, i), getP(i, bit)});
                } else {
                    add_clause({-getH(1, i), -getP(i, bit)});
                }
            }
        }
    }

    void vertex_end(int n, int m) {
        std::vector<int> target_state = get_target_state(m, n - 1);
        for (int i = 2; i <= n; ++i) {
            for (int bit = 0; bit < m; ++bit) {
                if (target_state[bit] == 1) {
                    add_clause({-getH(i, 1), getP(i, bit)});
                } else {
                    add_clause({-getH(i, 1), -getP(i, bit)});
                }
            }
        }
    }

    void vertex_positions(int n, int m) {
        auto it = LFSR_TAPS.find(m);
        if (it == LFSR_TAPS.end()) return;
        const auto& taps = it->second;

        for (int i = 2; i <= n; ++i) {
            for (int j = 2; j <= n; ++j) {
                for (int bit = 0; bit < m - 1; ++bit) {
                    add_clause({-getH(i, j), -getP(i, bit), getP(j, bit + 1)});
                    add_clause({-getH(i, j), getP(i, bit), -getP(j, bit + 1)});
                }

                int num_vars = taps.size() + 1;
                for (int comb = 0; comb < (1 << num_vars); ++comb) {
                    int set_bits = 0;
                    for (int b = 0; b < num_vars; ++b) {
                        if ((comb & (1 << b))) set_bits++;
                    }
                    if (set_bits % 2 == 1) {
                        std::vector<int> clause = {-getH(i, j)};
                        if (comb & 1) {
                            clause.push_back(-getP(j, 0));
                        } else {
                            clause.push_back(getP(j, 0));
                        }
                        for (size_t idx = 0; idx < taps.size(); ++idx) {
                            int tap = taps[idx];
                            if (comb & (1 << (idx + 1))) {
                                clause.push_back(-getP(i, tap));
                            } else {
                                clause.push_back(getP(i, tap));
                            }
                        }
                        add_clause(clause);
                    }
                }
            }
        }
    }

    void prevent_zero_state(int n, int m) {
        for (int i = 2; i <= n; ++i) {
            std::vector<int> clause;
            for (int bit = 0; bit < m; ++bit) {
                clause.push_back(getP(i, bit));
            }
            add_clause(clause);
        }
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        H.clear();
        P.clear();
        int n = graph.v;
        int m = std::ceil(std::log2(n));
        if (std::pow(2, m) == n) m++; // if log2(n) is an integer, add 1

        if (m > 16) {
            throw std::runtime_error("LFSR currently only supports graphs up to 65,535 vertices.");
        }

        add_default_variables(n, m, graph);
        vertex_outgoing_arcs(n);
        vertex_incoming_arcs(n);
        vertex_start(n, m);
        vertex_end(n, m);
        vertex_positions(n, m);
        prevent_zero_state(n, m);
        return context;
    }
};
