#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include "../../encodings/cardinality_one/NSCEncoding.hpp"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <memory>
#include <iostream>

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

class OCRT : public SuccessorMethod {
private:
    CardinalityOneEncoder* vee_encoder = nullptr;
    CardinalityOneEncoder* crt_encoder = nullptr;
    std::unique_ptr<CardinalityOneEncoder> default_vee_encoder;
    std::unique_ptr<CardinalityOneEncoder> default_crt_encoder;

    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> original_H;
    std::map<int, int> V;
    std::map<std::pair<int, int>, int> P;
    bool is_directed = true;
    int _dummy_v = -1;

public:
    OCRT(CardinalityOneEncoder* vee_enc = nullptr, CardinalityOneEncoder* crt_enc = nullptr, int cycle_len = -1)
        : SuccessorMethod(crt_enc), vee_encoder(vee_enc), crt_encoder(crt_enc) {
        cycle_override = cycle_len;
        if (vee_encoder == nullptr) {
            default_vee_encoder = std::make_unique<NSCEncoding>();
            vee_encoder = default_vee_encoder.get();
        }
        if (crt_encoder == nullptr) {
            default_crt_encoder = std::make_unique<NSCEncoding>();
            crt_encoder = default_crt_encoder.get();
        }
    }

    int getH(int u, int w) override {
        auto key = std::make_pair(u, w);
        if (original_H.find(key) == original_H.end()) {
            return _dummy_var();
        }
        return original_H[key];
    }

    int _dummy_var() {
        if (_dummy_v == -1) {
            _dummy_v = new_var();
            add_clause({-_dummy_v});
        }
        return _dummy_v;
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

    void enforce_non_zero_lfsr(const std::set<int>& active_vertices, int cycle_len, int nBits) {
        for (int i : active_vertices) {
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

    void init_termination_position(int n_prime, int first, const std::vector<int>& first_neighbors, int cycle_len) {
        for (int neighbor : first_neighbors) {
            int b = 0;
            int h_var = H[std::make_pair(neighbor, first)];

            int k = 1;
            while (true) {
                if ((cycle_len % (1 << k)) == 0) {
                    std::vector<int> clause = {-h_var};
                    if (((n_prime - 1) & ((1 << k) / 2)) == 0) {
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
                for (int i = 0; i < (n_prime - 1) % 3; ++i) {
                    mask = lfsr_step(mask, 2, 1);
                }
                for (int i = 0; i < 2; ++i) {
                    std::vector<int> clause = {-h_var};
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
                int mask = (n_prime + 4) % 5;
                for (int i = 0; i < 3; ++i) {
                    std::vector<int> clause = {-h_var};
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
                for (int i = 0; i < (n_prime - 1) % 7; ++i) {
                    mask = lfsr_step(mask, 3, 1);
                }
                for (int i = 0; i < 3; ++i) {
                    std::vector<int> clause = {-h_var};
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
                for (int i = 0; i < (n_prime - 1) % 511; ++i) {
                    mask = lfsr_step(mask, 9, 4);
                }
                for (int i = 0; i < 9; ++i) {
                    std::vector<int> clause = {-h_var};
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
                for (int i = 0; i < (n_prime - 1) % 1023; ++i) {
                    mask = lfsr_step(mask, 10, 3);
                }
                for (int i = 0; i < 10; ++i) {
                    std::vector<int> clause = {-h_var};
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
                for (int i = 0; i < (n_prime - 1) % 2047; ++i) {
                    mask = lfsr_step(mask, 11, 2);
                }
                for (int i = 0; i < 11; ++i) {
                    std::vector<int> clause = {-h_var};
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

    void add_transitions(const std::set<int>& active_vertices, const std::map<int, std::set<int>>& active_edges, int first, int cycle_len) {
        for (int i : active_vertices) {
            auto it = active_edges.find(i);
            if (it == active_edges.end()) continue;
            for (int j : it->second) {
                if (j != first) {
                    int h_var = H[std::make_pair(i, j)];
                    int b = 0;
                    if ((cycle_len % 2) == 0) {
                        add_clause({-h_var, getP(j, b), getP(i, b)});
                        add_clause({-h_var, -getP(j, b), -getP(i, b)});
                        b += 1;
                    }

                    int k = 2;
                    while (true) {
                        if ((cycle_len % (1 << k)) == 0) {
                            for (int l = 1; l < k; ++l) {
                                add_clause({-h_var, getP(i, b - l), getP(j, b), -getP(i, b)});
                                add_clause({-h_var, getP(i, b - l), -getP(j, b), getP(i, b)});
                            }
                            std::vector<int> clause1;
                            for (int l = 1; l < k; ++l) {
                                clause1.push_back(-getP(i, b - l));
                            }
                            clause1.push_back(-h_var);
                            clause1.push_back(getP(j, b));
                            clause1.push_back(getP(i, b));
                            add_clause(clause1);

                            std::vector<int> clause2;
                            for (int l = 1; l < k; ++l) {
                                clause2.push_back(-getP(i, b - l));
                            }
                            clause2.push_back(-h_var);
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
                        add_clause({-h_var, getP(j, b), -getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b), getP(i, b + 1)});
                        add_clause({-h_var, getP(j, b + 1), getP(i, b), -getP(i, b + 1)});
                        add_clause({-h_var, getP(j, b + 1), -getP(i, b), getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b + 1), getP(i, b), getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b + 1), -getP(i, b), -getP(i, b + 1)});
                        b += 2;
                    }

                    if ((cycle_len % 5) == 0) {
                        add_clause({-h_var, -getP(j, b), -getP(i, b)});
                        add_clause({-h_var, -getP(j, b), -getP(i, b + 2)});
                        add_clause({-h_var, getP(j, b), getP(i, b), getP(i, b + 2)});
                        add_clause({-h_var, getP(j, b + 1), getP(i, b), -getP(i, b + 1)});
                        add_clause({-h_var, getP(j, b + 1), -getP(i, b), getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b + 1), getP(i, b), getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b + 1), -getP(i, b), -getP(i, b + 1)});
                        add_clause({-h_var, -getP(j, b + 2), getP(i, b)});
                        add_clause({-h_var, -getP(j, b + 2), getP(i, b + 1)});
                        add_clause({-h_var, getP(j, b + 2), -getP(i, b), -getP(i, b + 1)});
                        b += 3;
                    }

                    if ((cycle_len % 7) == 0) {
                        add_clause({-h_var, getP(j, b), -getP(i, b + 2)});
                        add_clause({-h_var, -getP(j, b), getP(i, b + 2)});
                        add_clause({-h_var, getP(j, b + 1), -getP(i, b)});
                        add_clause({-h_var, -getP(j, b + 1), getP(i, b)});
                        add_clause({-h_var, getP(j, b + 2), getP(i, b + 1), -getP(i, b + 2)});
                        add_clause({-h_var, getP(j, b + 2), -getP(i, b + 1), getP(i, b + 2)});
                        add_clause({-h_var, -getP(j, b + 2), getP(i, b + 1), getP(i, b + 2)});
                        add_clause({-h_var, -getP(j, b + 2), -getP(i, b + 1), -getP(i, b + 2)});
                        b += 3;
                    }

                    if ((cycle_len % 511) == 0) {
                        add_clause({-h_var, getP(j, b), -getP(i, b + 8)});
                        add_clause({-h_var, -getP(j, b), getP(i, b + 8)});
                        for (int idx = 1; idx <= 4; ++idx) {
                            add_clause({-h_var, getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-h_var, -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-h_var, getP(j, b + 5), getP(i, b + 4), -getP(i, b + 8)});
                        add_clause({-h_var, getP(j, b + 5), -getP(i, b + 4), getP(i, b + 8)});
                        add_clause({-h_var, -getP(j, b + 5), getP(i, b + 4), getP(i, b + 8)});
                        add_clause({-h_var, -getP(j, b + 5), -getP(i, b + 4), -getP(i, b + 8)});
                        for (int idx = 6; idx <= 8; ++idx) {
                            add_clause({-h_var, getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-h_var, -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        b += 9;
                    }

                    if ((cycle_len % 1023) == 0) {
                        add_clause({-h_var, getP(j, b), -getP(i, b + 9)});
                        add_clause({-h_var, -getP(j, b), getP(i, b + 9)});
                        for (int idx = 1; idx <= 6; ++idx) {
                            add_clause({-h_var, getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-h_var, -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-h_var, getP(j, b + 7), getP(i, b + 6), -getP(i, b + 9)});
                        add_clause({-h_var, getP(j, b + 7), -getP(i, b + 6), getP(i, b + 9)});
                        add_clause({-h_var, -getP(j, b + 7), getP(i, b + 6), getP(i, b + 9)});
                        add_clause({-h_var, -getP(j, b + 7), -getP(i, b + 6), -getP(i, b + 9)});
                        for (int idx = 8; idx <= 9; ++idx) {
                            add_clause({-h_var, getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-h_var, -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        b += 10;
                    }

                    if ((cycle_len % 2047) == 0) {
                        add_clause({-h_var, getP(j, b), -getP(i, b + 10)});
                        add_clause({-h_var, -getP(j, b), getP(i, b + 10)});
                        for (int idx = 1; idx <= 8; ++idx) {
                            add_clause({-h_var, getP(j, b + idx), -getP(i, b + idx - 1)});
                            add_clause({-h_var, -getP(j, b + idx), getP(i, b + idx - 1)});
                        }
                        add_clause({-h_var, getP(j, b + 9), getP(i, b + 8), -getP(i, b + 10)});
                        add_clause({-h_var, getP(j, b + 9), -getP(i, b + 8), getP(i, b + 10)});
                        add_clause({-h_var, -getP(j, b + 9), getP(i, b + 8), getP(i, b + 10)});
                        add_clause({-h_var, -getP(j, b + 9), -getP(i, b + 8), -getP(i, b + 10)});
                        add_clause({-h_var, getP(j, b + 10), -getP(i, b + 9)});
                        add_clause({-h_var, -getP(j, b + 10), getP(i, b + 9)});
                        b += 11;
                    }
                }
            }
        }
    }

    void _vee_eo(const std::vector<int>& literals) {
        vee_encoder->encode_eo(*context, literals);
    }

    void _vee_amo(const std::vector<int>& literals) {
        vee_encoder->encode_amo(*context, literals);
    }

    void _crt_eo(const std::vector<int>& literals) {
        crt_encoder->encode_eo(*context, literals);
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        int n = graph.v;
        is_directed = graph.is_directed;

        original_H.clear();
        H.clear();
        V.clear();
        P.clear();
        _dummy_v = -1;

        // 1. Initialize variables for vertices and original edges
        for (int v = 1; v <= n; ++v) {
            V[v] = new_var();
            add_clause({V[v]});
        }

        for (int u = 1; u <= n; ++u) {
            if (u <= (int)graph.adj_list.size()) {
                for (int w : graph.adj_list[u]) {
                    original_H[std::make_pair(u, w)] = new_var();
                }
            }
        }

        // Copy original variables to active variables mapping
        H = original_H;

        // Handle trivial cases
        if (n < 3) {
            add_clause({});
            return context;
        }

        // Set up active vertices and active edges
        std::set<int> active_vertices;
        for (int i = 1; i <= n; ++i) active_vertices.insert(i);

        std::map<int, std::set<int>> active_edges;
        for (int u = 1; u <= n; ++u) {
            if (u <= (int)graph.adj_list.size()) {
                for (int w : graph.adj_list[u]) {
                    active_edges[u].insert(w);
                }
            }
        }

        // 2. Sequential Elimination Loop (Phase 1 VEE)
        int sigma = 0;
        while (true) {
            int n_prime = active_vertices.size();
            if (n_prime == 3) {
                for (int r : active_vertices) {
                    std::vector<int> incoming;
                    for (int u : active_vertices) {
                        if (active_edges[u].find(r) != active_edges[u].end()) {
                            incoming.push_back(H[std::make_pair(u, r)]);
                        }
                    }
                    std::vector<int> outgoing;
                    for (int w : active_edges[r]) {
                        if (active_vertices.find(w) != active_vertices.end()) {
                            outgoing.push_back(H[std::make_pair(r, w)]);
                        }
                    }
                    _vee_eo(incoming);
                    _vee_eo(outgoing);
                }
                return context;
            }

            if (n_prime < 3) {
                add_clause({});
                return context;
            }

            std::map<int, std::set<int>> in_adj;
            for (int u : active_vertices) {
                for (int w : active_edges[u]) {
                    if (active_vertices.find(w) != active_vertices.end()) {
                        in_adj[w].insert(u);
                    }
                }
            }

            bool has_deg_0 = false;
            for (int u : active_vertices) {
                if (active_edges[u].empty() || in_adj[u].empty()) {
                    has_deg_0 = true;
                    break;
                }
            }

            if (has_deg_0) {
                add_clause({});
                return context;
            }

            // Sparsity check
            int d_min = 1e9;
            for (int u : active_vertices) {
                d_min = std::min(d_min, (int)active_edges[u].size());
            }
            if (!(d_min * sigma <= n)) {
                break;
            }

            // Find vertex v with minimum degree (out-degree + in-degree)
            int v = -1;
            int min_deg = 1e9;
            for (int u : active_vertices) {
                int deg = active_edges[u].size() + in_adj[u].size();
                if (deg < min_deg) {
                    min_deg = deg;
                    v = u;
                }
            }

            if (v == -1) {
                add_clause({});
                return context;
            }

            std::vector<std::pair<int, int>> outgoing_edges;
            for (int w : active_edges[v]) outgoing_edges.push_back({v, w});
            std::vector<std::pair<int, int>> incoming_edges;
            for (int u : in_adj[v]) incoming_edges.push_back({u, v});

            std::vector<int> incoming_vars;
            for (auto edge : incoming_edges) incoming_vars.push_back(H[edge]);
            std::vector<int> outgoing_vars;
            for (auto edge : outgoing_edges) outgoing_vars.push_back(H[edge]);

            _vee_eo(incoming_vars);
            _vee_eo(outgoing_vars);

            for (auto in_edge : incoming_edges) {
                int u = in_edge.first;
                std::pair<int, int> rev_edge = {v, u};
                if (std::find(outgoing_edges.begin(), outgoing_edges.end(), rev_edge) != outgoing_edges.end()) {
                    int b_uv = H[in_edge];
                    int b_vu = H[rev_edge];
                    add_clause({-b_uv, -b_vu});
                }
            }

            std::vector<int> out_neighbors(active_edges[v].begin(), active_edges[v].end());
            std::vector<int> in_neighbors(in_adj[v].begin(), in_adj[v].end());

            std::map<int, std::set<int>> next_active_edges;
            for (int u : active_vertices) {
                if (u == v) continue;
                for (int w : active_edges[u]) {
                    if (w != v) next_active_edges[u].insert(w);
                }
            }

            std::vector<int> new_arc_vars;
            std::vector<std::pair<std::pair<int, int>, int>> updates;

            for (int u : in_neighbors) {
                for (int w : out_neighbors) {
                    if (u == w) continue;
                    bool is_new_arc = (active_edges[u].find(w) == active_edges[u].end());
                    int b_prime_uw = new_var();
                    int b_uv = H[std::make_pair(u, v)];
                    int b_vw = H[std::make_pair(v, w)];

                    if (is_new_arc) {
                        add_clause({-b_prime_uw, b_uv});
                        add_clause({-b_prime_uw, b_vw});
                        add_clause({-b_uv, -b_vw, b_prime_uw});
                        new_arc_vars.push_back(b_prime_uw);
                    } else {
                        int b_uw = H[std::make_pair(u, w)];
                        add_clause({-b_uv, -b_vw, b_prime_uw});
                        add_clause({-b_uv, -b_vw, -b_uw});
                        add_clause({b_uv, -b_uw, b_prime_uw});
                        add_clause({b_vw, -b_uw, b_prime_uw});
                        add_clause({b_uv, b_uw, -b_prime_uw});
                        add_clause({b_vw, b_uw, -b_prime_uw});
                    }

                    updates.push_back({std::make_pair(u, w), b_prime_uw});
                    next_active_edges[u].insert(w);
                }
            }

            for (auto update : updates) {
                H[update.first] = update.second;
            }

            if (new_arc_vars.size() > 1) {
                _vee_amo(new_arc_vars);
            }

            active_vertices.erase(v);
            active_edges = next_active_edges;
            sigma++;
        }

        int n_prime = active_vertices.size();

        if (n_prime == 3) {
            for (int r : active_vertices) {
                std::vector<int> incoming;
                for (int u : active_vertices) {
                    if (active_edges[u].find(r) != active_edges[u].end()) {
                        incoming.push_back(H[std::make_pair(u, r)]);
                    }
                }
                std::vector<int> outgoing;
                for (int w : active_edges[r]) {
                    if (active_vertices.find(w) != active_vertices.end()) {
                        outgoing.push_back(H[std::make_pair(r, w)]);
                    }
                }
                _vee_eo(incoming);
                _vee_eo(outgoing);
            }
            return context;
        }

        if (n_prime < 3) {
            add_clause({});
            return context;
        }

        // Starting vertex
        int first = -1;
        int min_out_deg = 1e9;
        for (int u : active_vertices) {
            if ((int)active_edges[u].size() < min_out_deg) {
                min_out_deg = active_edges[u].size();
                first = u;
            }
        }

        auto [cycle_len, nBits] = get_cycle_and_nbits(n_prime);
        cycle = cycle_len;

        // 1. Exactly one outgoing edge
        for (int i : active_vertices) {
            std::vector<int> literals;
            for (int j : active_edges[i]) {
                literals.push_back(H[std::make_pair(i, j)]);
            }
            _crt_eo(literals);
        }

        // 2. Exactly one incoming edge
        std::map<int, std::set<int>> in_adj;
        for (int u : active_vertices) {
            for (int w : active_edges[u]) {
                if (active_vertices.find(w) != active_vertices.end()) {
                    in_adj[w].insert(u);
                }
            }
        }

        for (int j : active_vertices) {
            std::vector<int> literals;
            for (int i : in_adj[j]) {
                literals.push_back(H[std::make_pair(i, j)]);
            }
            _crt_eo(literals);
        }

        // 3. Enforce non-zero representation
        enforce_non_zero_lfsr(active_vertices, cycle_len, nBits);

        // 4. Connect one neighbor back to start (final edge)
        std::vector<int> first_neighbors;
        for (int u : active_vertices) {
            if (active_edges[u].find(first) != active_edges[u].end()) {
                first_neighbors.push_back(u);
            }
        }
        if (first_neighbors.empty()) {
            add_clause({});
            return context;
        }

        std::vector<int> first_literals;
        for (int neighbor : first_neighbors) {
            first_literals.push_back(H[std::make_pair(neighbor, first)]);
        }
        add_clause(first_literals);

        // 5. Symmetry breaking
        if (!is_directed) {
            for (size_t i = 0; i < first_neighbors.size(); ++i) {
                std::vector<int> clause;
                for (size_t j = 0; j < i; ++j) {
                    int neighbor_j = first_neighbors[j];
                    if (active_edges[first].find(neighbor_j) != active_edges[first].end()) {
                        clause.push_back(H[std::make_pair(first, neighbor_j)]);
                    }
                }
                clause.push_back(-H[std::make_pair(first_neighbors[i], first)]);
                add_clause(clause);
            }
        }

        // 6. Initialize starting position
        init_starting_position(first, cycle_len);

        // 7. Initialize termination position
        init_termination_position(n_prime, first, first_neighbors, cycle_len);

        // 8. Add transitions
        add_transitions(active_vertices, active_edges, first, cycle_len);

        return context;
    }
};
