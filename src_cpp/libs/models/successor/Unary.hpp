#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include <map>

class Unary : public SuccessorMethod {
private:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> U;

public:
    Unary(CardinalityOneEncoder* enc = nullptr) : SuccessorMethod(enc) {}

    int getH(int i, int j) override {
        auto key = std::make_pair(i, j);
        if (H.find(key) == H.end()) {
            H[key] = new_var();
        }
        return H[key];
    }

    int getU(int i, int j) {
        auto key = std::make_pair(i, j);
        if (U.find(key) == U.end()) {
            U[key] = new_var();
        }
        return U[key];
    }

    void preprocessing(const Graph& graph) {
        int n = graph.v;
        int start_v = graph.start_vertex;
        auto distance = find_distance(graph.adj_list, start_v, n);
        auto distance_to_first = graph.is_directed ? find_distance_to_first(graph.adj_list, start_v, n) : distance;

        for (int vertex = 1; vertex <= n; ++vertex) {
            if (vertex == start_v) continue;

            for (int t = 1; t <= distance[vertex]; ++t) {
                add_clause({-getU(vertex, t)});
            }

            for (int t = n - distance_to_first[vertex] + 2; t <= n; ++t) {
                add_clause({-getU(vertex, t)});
            }
        }
    }

    void add_default_variables(int n, const Graph& graph) {
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
        int start_v = graph.start_vertex;
        add_clause({getU(start_v, 1)});
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

    void vertex_start(int n, const Graph& graph) {
        int start_v = graph.start_vertex;
        for (int i = 1; i <= n; ++i) {
            if (i == start_v) continue;
            add_clause({-getH(start_v, i), getU(i, 2)});
        }
    }

    void vertex_end(int n, const Graph& graph) {
        int start_v = graph.start_vertex;
        for (int i = 1; i <= n; ++i) {
            if (i == start_v) continue;
            add_clause({-getH(i, start_v), getU(i, n)});
        }
    }

    void vertex_positions(int n, const Graph& graph) {
        int start_v = graph.start_vertex;
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                if (i != start_v && j != start_v && i != j) {
                    for (int p = 2; p < n; ++p) {
                        add_clause({-getH(i, j), -getU(i, p), getU(j, p + 1)});
                    }
                }
            }
        }
    }

    void vertex_EO_positions(int n) {
        for (int i = 1; i <= n; ++i) {
            std::vector<int> literals;
            for (int p = 1; p <= n; ++p) {
                literals.push_back(getU(i, p));
            }
            exactly_one_constraint(literals);
        }
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        H.clear();
        U.clear();
        int n = graph.v;

        if (is_preprocessing) {
            preprocessing(graph);
        }

        add_default_variables(n, graph);
        vertex_outgoing_arcs(n);
        vertex_incoming_arcs(n);
        vertex_start(n, graph);
        vertex_end(n, graph);
        vertex_positions(n, graph);
        vertex_EO_positions(n);
        return context;
    }
};
