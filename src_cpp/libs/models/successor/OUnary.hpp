#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include "../../encodings/cardinality_one/NSCEncoding.hpp"
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdexcept>

class OUnary : public SuccessorMethod {
private:
    CardinalityOneEncoder* vee_encoder = nullptr;
    std::unique_ptr<CardinalityOneEncoder> default_vee_encoder;
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> original_H;
    std::map<int, int> V;
    std::map<std::pair<int, int>, int> U;
    bool is_directed = true;
    int _dummy_v = -1;

public:
    OUnary(CardinalityOneEncoder* vee_enc = nullptr) : SuccessorMethod(nullptr), vee_encoder(vee_enc) {
        if (vee_encoder == nullptr) {
            default_vee_encoder = std::make_unique<NSCEncoding>();
            vee_encoder = default_vee_encoder.get();
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

    void _vee_eo(const std::vector<int>& literals) {
        if (vee_encoder == nullptr) {
            throw std::runtime_error("No vee_encoder set for OUnary.");
        }
        vee_encoder->encode_eo(*context, literals);
    }

    void _vee_amo(const std::vector<int>& literals) {
        if (vee_encoder == nullptr) {
            throw std::runtime_error("No vee_encoder set for OUnary.");
        }
        vee_encoder->encode_amo(*context, literals);
    }

    int getU(int i, int p) {
        auto key = std::make_pair(i, p);
        if (U.find(key) == U.end()) {
            U[key] = new_var();
        }
        return U[key];
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        int n = graph.v;
        is_directed = graph.is_directed;

        original_H.clear();
        H.clear();
        V.clear();
        U.clear();
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
            for (int w : active_edges[v]) {
                outgoing_edges.push_back({v, w});
            }
            std::vector<std::pair<int, int>> incoming_edges;
            for (int u : in_adj[v]) {
                incoming_edges.push_back({u, v});
            }

            std::vector<int> incoming_vars;
            for (auto edge : incoming_edges) {
                incoming_vars.push_back(H[edge]);
            }
            std::vector<int> outgoing_vars;
            for (auto edge : outgoing_edges) {
                outgoing_vars.push_back(H[edge]);
            }

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

        // Find vertex in active_vertices with minimum out-degree
        int first = -1;
        int min_out_deg = 1e9;
        for (int u : active_vertices) {
            if ((int)active_edges[u].size() < min_out_deg) {
                min_out_deg = active_edges[u].size();
                first = u;
            }
        }

        // 1. Exactly one outgoing edge
        for (int i : active_vertices) {
            std::vector<int> literals;
            for (int j : active_edges[i]) {
                literals.push_back(H[std::make_pair(i, j)]);
            }
            _vee_eo(literals);
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
            _vee_eo(literals);
        }

        // 3. Connect one neighbor back to start (final edge)
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

        std::vector<int> final_literals;
        for (int neighbor : first_neighbors) {
            final_literals.push_back(H[std::make_pair(neighbor, first)]);
        }
        add_clause(final_literals);

        // 4. Symmetry breaking
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

        // 5. Unary Phase
        // start_vertex position is 1
        add_clause({getU(first, 1)});

        // Exactly one position per vertex
        for (int i : active_vertices) {
            std::vector<int> literals;
            for (int p = 1; p <= n_prime; ++p) {
                literals.push_back(getU(i, p));
            }
            _vee_eo(literals);
        }

        // Transitions
        for (int i : active_vertices) {
            for (int j : active_edges[i]) {
                if (i == first && j != first) {
                    add_clause({-H[std::make_pair(i, j)], getU(j, 2)});
                } else if (j == first && i != first) {
                    add_clause({-H[std::make_pair(i, j)], getU(i, n_prime)});
                } else if (i != first && j != first) {
                    for (int p = 2; p < n_prime; ++p) {
                        add_clause({-H[std::make_pair(i, j)], -getU(i, p), getU(j, p + 1)});
                    }
                }
            }
        }

        return context;
    }
};
