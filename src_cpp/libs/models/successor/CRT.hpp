#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include <map>
#include <vector>
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


class CRT : public SuccessorMethod {
protected:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> P;

public:
    CRT(CardinalityOneEncoder* enc = nullptr, int cycle_len = -1) : SuccessorMethod(enc) {
        cycle_override = cycle_len;
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

    std::pair<int, int> get_cycle_and_nbits(int nNode) {
        int cycle_len = 2;
        if (cycle_override != -1) {
            cycle_len = cycle_override;
        } else {
            if (cycle_len <= nNode) {
                cycle_len *= 3;
            }
            if (cycle_len <= nNode) {
                cycle_len *= 5;
            }
            if (cycle_len <= nNode) {
                cycle_len *= 7;
            }
            while (cycle_len <= nNode) {
                cycle_len *= 2;
            }
        }

        int nBits = 0;
        int k = 1;
        while (true) {
            if ((cycle_len % (1 << k)) == 0) {
                nBits += 1;
            } else {
                break;
            }
            k += 1;
        }

        if ((cycle_len % 3) == 0) nBits += 2;
        if ((cycle_len % 5) == 0) nBits += 3;
        if ((cycle_len % 7) == 0) nBits += 3;
        if ((cycle_len % 511) == 0) nBits += 9;
        if ((cycle_len % 1023) == 0) nBits += 10;
        if ((cycle_len % 2047) == 0) nBits += 11;

        return {cycle_len, nBits};
    }

    void enforce_non_zero_lfsr(int n, int cycle_len, int nBits) {
        for (int i = 1; i <= n; ++i) {
            int b = 0;
            int k = 1;
            while (true) {
                if ((cycle_len % (1 << k)) == 0) {
                    b += 1;
                } else {
                    break;
                }
                k += 1;
            }

            if ((cycle_len % 3) == 0) {
                add_clause({getP(i, b), getP(i, b + 1)});
                b += 2;
            }
            if ((cycle_len % 5) == 0) {
                add_clause({-getP(i, b), -getP(i, b + 2)});
                add_clause({-getP(i, b + 1), -getP(i, b + 2)});
                b += 3;
            }
            if ((cycle_len % 7) == 0) {
                add_clause({getP(i, b), getP(i, b + 1), getP(i, b + 2)});
                b += 3;
            }
            if ((cycle_len % 511) == 0) {
                std::vector<int> clause;
                for (int j = 0; j < 9; ++j) {
                    clause.push_back(getP(i, b + j));
                }
                add_clause(clause);
                b += 9;
            }
            if ((cycle_len % 1023) == 0) {
                std::vector<int> clause;
                for (int j = 0; j < 10; ++j) {
                    clause.push_back(getP(i, b + j));
                }
                add_clause(clause);
                b += 10;
            }
            if ((cycle_len % 2047) == 0) {
                std::vector<int> clause;
                for (int j = 0; j < 11; ++j) {
                    clause.push_back(getP(i, b + j));
                }
                add_clause(clause);
                b += 11;
            }
        }
    }

    void init_starting_position(int first, int cycle_len) {
        int b = 0;
        int k = 1;
        while (true) {
            if ((cycle_len % (1 << k)) == 0) {
                add_clause({-getP(first, b)});
                b += 1;
            } else {
                break;
            }
            k += 1;
        }

        if ((cycle_len % 3) == 0) {
            add_clause({getP(first, b)});
            b += 1;
            add_clause({-getP(first, b)});
            b += 1;
        }
        if ((cycle_len % 5) == 0) {
            add_clause({-getP(first, b)});
            b += 1;
            add_clause({-getP(first, b)});
            b += 1;
            add_clause({-getP(first, b)});
            b += 1;
        }
        if ((cycle_len % 7) == 0) {
            add_clause({getP(first, b)});
            b += 1;
            add_clause({-getP(first, b)});
            b += 1;
            add_clause({-getP(first, b)});
            b += 1;
        }

        if ((cycle_len % 511) == 0) {
            add_clause({getP(first, b)});
            b += 1;
            for (int k_idx = 2; k_idx <= 9; ++k_idx) {
                add_clause({-getP(first, b)});
                b += 1;
            }
        }
        if ((cycle_len % 1023) == 0) {
            add_clause({getP(first, b)});
            b += 1;
            for (int k_idx = 2; k_idx <= 10; ++k_idx) {
                add_clause({-getP(first, b)});
                b += 1;
            }
        }
        if ((cycle_len % 2047) == 0) {
            add_clause({getP(first, b)});
            b += 1;
            for (int k_idx = 2; k_idx <= 11; ++k_idx) {
                add_clause({-getP(first, b)});
                b += 1;
            }
        }
    }

    void init_termination_position(int n, int first, const std::vector<int>& first_neighbors, int cycle_len) {
        int minDegree = first_neighbors.size();
        for (int j = 0; j < minDegree; ++j) {
            int neighbor = first_neighbors[j];
            int b = 0;

            int k = 1;
            while (true) {
                if ((cycle_len % (1 << k)) == 0) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if (((n - 1) & ((1 << k) / 2)) == 0) {
                        clause.push_back(-getP(neighbor, b));
                    } else {
                        clause.push_back(getP(neighbor, b));
                    }
                    add_clause(clause);
                    b += 1;
                } else {
                    break;
                }
                k += 1;
            }

            if ((cycle_len % 3) == 0) {
                int mask = 1;
                for (int i = 0; i < (n - 1) % 3; ++i) {
                    mask = lfsr_step(mask, 2, 1);
                }
                for (int i = 0; i < 2; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 2;
            }

            if ((cycle_len % 5) == 0) {
                int mask = (n + 4) % 5;
                for (int i = 0; i < 3; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 3;
            }

            if ((cycle_len % 7) == 0) {
                int mask = 1;
                for (int i = 0; i < (n - 1) % 7; ++i) {
                    mask = lfsr_step(mask, 3, 1);
                }
                for (int i = 0; i < 3; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 3;
            }

            if ((cycle_len % 511) == 0) {
                int mask = 1;
                for (int i = 0; i < (n - 1) % 511; ++i) {
                    mask = lfsr_step(mask, 9, 4);
                }
                for (int i = 0; i < 9; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 9;
            }

            if ((cycle_len % 1023) == 0) {
                int mask = 1;
                for (int i = 0; i < (n - 1) % 1023; ++i) {
                    mask = lfsr_step(mask, 10, 3);
                }
                for (int i = 0; i < 10; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 10;
            }

            if ((cycle_len % 2047) == 0) {
                int mask = 1;
                for (int i = 0; i < (n - 1) % 2047; ++i) {
                    mask = lfsr_step(mask, 11, 2);
                }
                for (int i = 0; i < 11; ++i) {
                    std::vector<int> clause = {-getH(neighbor, first)};
                    if ((mask & 1) == 0) {
                        clause.push_back(-getP(neighbor, b + i));
                    } else {
                        clause.push_back(getP(neighbor, b + i));
                    }
                    add_clause(clause);
                    mask = mask >> 1;
                }
                b += 11;
            }
        }
    }

    void add_transitions(int n, const Graph& graph, int first, int cycle_len) {
        for (int i = 1; i <= n; ++i) {
            if (i > (int)graph.adj_list.size()) continue;
            for (int j : graph.adj_list[i]) {
                if (j != first) {
                    int b = 0;
                    if ((cycle_len % 2) == 0) {
                        add_clause({-getH(i, j), getP(j, b), getP(i, b)});
                        add_clause({-getH(i, j), -getP(j, b), -getP(i, b)});
                        b += 1;
                    }

                    int k = 2;
                    while (true) {
                        if ((cycle_len % (1 << k)) == 0) {
                            for (int l = 1; l < k; ++l) {
                                add_clause({-getH(i, j), getP(i, b - l), getP(j, b), -getP(i, b)});
                                add_clause({-getH(i, j), getP(i, b - l), -getP(j, b), getP(i, b)});
                            }
                            std::vector<int> clause1;
                            for (int l = 1; l < k; ++l) {
                                clause1.push_back(-getP(i, b - l));
                            }
                            clause1.push_back(-getH(i, j));
                            clause1.push_back(getP(j, b));
                            clause1.push_back(getP(i, b));
                            add_clause(clause1);

                            std::vector<int> clause2;
                            for (int l = 1; l < k; ++l) {
                                clause2.push_back(-getP(i, b - l));
                            }
                            clause2.push_back(-getH(i, j));
                            clause2.push_back(-getP(j, b));
                            clause2.push_back(-getP(i, b));
                            add_clause(clause2);
                            b += 1;
                        } else {
                            break;
                        }
                        k += 1;
                    }

                    if ((cycle_len % 3) == 0) {
                        add_clause({-getH(i, j), getP(j, b), -getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b), getP(i, b + 1)});
                        add_clause({-getH(i, j), getP(j, b + 1), getP(i, b), -getP(i, b + 1)});
                        add_clause({-getH(i, j), getP(j, b + 1), -getP(i, b), getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b + 1), getP(i, b), getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b + 1), -getP(i, b), -getP(i, b + 1)});
                        b += 2;
                    }

                    if ((cycle_len % 5) == 0) {
                        add_clause({-getH(i, j), -getP(j, b), -getP(i, b)});
                        add_clause({-getH(i, j), -getP(j, b), -getP(i, b + 2)});
                        add_clause({-getH(i, j), getP(j, b), getP(i, b), getP(i, b + 2)});
                        add_clause({-getH(i, j), getP(j, b + 1), getP(i, b), -getP(i, b + 1)});
                        add_clause({-getH(i, j), getP(j, b + 1), -getP(i, b), getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b + 1), getP(i, b), getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b + 1), -getP(i, b), -getP(i, b + 1)});
                        add_clause({-getH(i, j), -getP(j, b + 2), getP(i, b)});
                        add_clause({-getH(i, j), -getP(j, b + 2), getP(i, b + 1)});
                        add_clause({-getH(i, j), getP(j, b + 2), -getP(i, b), -getP(i, b + 1)});
                        b += 3;
                    }

                    if ((cycle_len % 7) == 0) {
                        add_clause({-getH(i, j), getP(j, b), -getP(i, b + 2)});
                        add_clause({-getH(i, j), -getP(j, b), getP(i, b + 2)});
                        add_clause({-getH(i, j), getP(j, b + 1), -getP(i, b)});
                        add_clause({-getH(i, j), -getP(j, b + 1), getP(i, b)});
                        add_clause({-getH(i, j), getP(j, b + 2), getP(i, b + 1), -getP(i, b + 2)});
                        add_clause({-getH(i, j), getP(j, b + 2), -getP(i, b + 1), getP(i, b + 2)});
                        add_clause({-getH(i, j), -getP(j, b + 2), getP(i, b + 1), getP(i, b + 2)});
                        add_clause({-getH(i, j), -getP(j, b + 2), -getP(i, b + 1), -getP(i, b + 2)});
                        b += 3;
                    }

                    if ((cycle_len % 511) == 0) {
                        add_clause({-getH(i, j), getP(j, b), -getP(i, b + 8)});
                        add_clause({-getH(i, j), -getP(j, b), getP(i, b + 8)});
                        for (int idx = 1; idx <= 4; ++idx) {
                            add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-getH(i, j), getP(j, b + 5), getP(i, b + 4), -getP(i, b + 8)});
                        add_clause({-getH(i, j), getP(j, b + 5), -getP(i, b + 4), getP(i, b + 8)});
                        add_clause({-getH(i, j), -getP(j, b + 5), getP(i, b + 4), getP(i, b + 8)});
                        add_clause({-getH(i, j), -getP(j, b + 5), -getP(i, b + 4), -getP(i, b + 8)});
                        for (int idx = 6; idx <= 8; ++idx) {
                            add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        b += 9;
                    }

                    if ((cycle_len % 1023) == 0) {
                        add_clause({-getH(i, j), getP(j, b), -getP(i, b + 9)});
                        add_clause({-getH(i, j), -getP(j, b), getP(i, b + 9)});
                        for (int idx = 1; idx <= 6; ++idx) {
                            add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-getH(i, j), getP(j, b + 7), getP(i, b + 6), -getP(i, b + 9)});
                        add_clause({-getH(i, j), getP(j, b + 7), -getP(i, b + 6), getP(i, b + 9)});
                        add_clause({-getH(i, j), -getP(j, b + 7), getP(i, b + 6), getP(i, b + 9)});
                        add_clause({-getH(i, j), -getP(j, b + 7), -getP(i, b + 6), -getP(i, b + 9)});
                        for (int idx = 8; idx <= 9; ++idx) {
                            add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        b += 10;
                    }

                    if ((cycle_len % 2047) == 0) {
                        add_clause({-getH(i, j), getP(j, b), -getP(i, b + 10)});
                        add_clause({-getH(i, j), -getP(j, b), getP(i, b + 10)});
                        for (int idx = 1; idx <= 8; ++idx) {
                            add_clause({-getH(i, j), getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-getH(i, j), -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-getH(i, j), getP(j, b + 9), getP(i, b + 8), -getP(i, b + 10)});
                        add_clause({-getH(i, j), getP(j, b + 9), -getP(i, b + 8), getP(i, b + 10)});
                        add_clause({-getH(i, j), -getP(j, b + 9), getP(i, b + 8), getP(i, b + 10)});
                        add_clause({-getH(i, j), -getP(j, b + 9), -getP(i, b + 8), -getP(i, b + 10)});
                        add_clause({-getH(i, j), getP(j, b + 10), -getP(i, b + 9)});
                        add_clause({-getH(i, j), -getP(j, b + 10), getP(i, b + 9)});
                        b += 11;
                    }
                }
            }
        }
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        H.clear();
        P.clear();
        int n = graph.v;
        int first = graph.start_vertex;

        auto [cycle_len, nBits] = get_cycle_and_nbits(n);
        cycle = cycle_len;

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
