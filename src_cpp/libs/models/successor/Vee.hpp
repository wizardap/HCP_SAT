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

class Vee : public SuccessorMethod {
private:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> original_H;
    std::map<int, int> V;
    std::unique_ptr<CardinalityOneEncoder> default_encoder;

public:
    Vee(CardinalityOneEncoder* enc = nullptr) : SuccessorMethod(enc) {
        if (encoder == nullptr) {
            default_encoder = std::make_unique<NSCEncoding>();
            encoder = default_encoder.get();
        }
    }

    int getH(int u, int w) override {
        auto key = std::make_pair(u, w);
        if (original_H.find(key) == original_H.end()) {
            original_H[key] = new_var();
        }
        return original_H[key];
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        int n = graph.v;

        original_H.clear();
        H.clear();
        V.clear();

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

        // 2. Sequential Elimination Loop
        for (int step = 1; step <= n - 3; ++step) {
            std::map<int, std::set<int>> in_adj;
            for (int u : active_vertices) {
                for (int w : active_edges[u]) {
                    if (active_vertices.find(w) != active_vertices.end()) {
                        in_adj[w].insert(u);
                    }
                }
            }

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

            if (in_adj[v].empty() || active_edges[v].empty()) {
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

            exactly_one_constraint(incoming_vars);
            exactly_one_constraint(outgoing_vars);

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
                encoder->encode_amo(*context, new_arc_vars);
            }

            active_vertices.erase(v);
            active_edges = next_active_edges;
        }

        // 3. Base Case: len(active_vertices) == 3
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

            exactly_one_constraint(incoming);
            exactly_one_constraint(outgoing);
        }

        return context;
    }
};
