#pragma once
#include "SuccessorMethod.hpp"
#include "../../utils/Common.hpp"
#include "../../encodings/cardinality_one/HeuleEncoder.hpp"
#include <map>
#include <cmath>
#include <string>
#include <algorithm>
#include <memory>

class BinaryAdderOriginal : public SuccessorMethod {
private:
    std::map<std::pair<int, int>, int> H;
    std::map<std::pair<int, int>, int> P;
    std::unique_ptr<CardinalityOneEncoder> default_encoder;

public:
    BinaryAdderOriginal(CardinalityOneEncoder* enc = nullptr) : SuccessorMethod(enc) {
        if (encoder == nullptr) {
            default_encoder = std::make_unique<HeuleEncoder>();
            encoder = default_encoder.get();
        }
    }

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

    void preprocessing(const Graph& graph, int m) {
        int n = graph.v;
        int start_v = graph.start_vertex;
        auto distance = find_distance(graph.adj_list, start_v, n);
        auto distance_to_first = graph.is_directed ? find_distance_to_first(graph.adj_list, start_v, n) : distance;

        for (int vertex = 1; vertex <= n; ++vertex) {
            if (vertex == start_v) continue;

            // Pi != t if the minimum distance from vertex 1 to vertex i is greater than t
            for (int t = 1; t <= distance[vertex]; ++t) {
                std::vector<char> binaryT = get_binary_bits(t, m);
                std::vector<int> clause;
                for (int bit = 0; bit < m; ++bit) {
                    clause.push_back((binaryT[bit] == '1') ? -getP(vertex, bit) : getP(vertex, bit));
                }
                add_clause(clause);
            }

            // Pi != t if the minimum distance from vertex i to vertex 1 is greater than n-t
            for (int t = n - distance_to_first[vertex] + 2; t <= n; ++t) {
                std::vector<char> binaryT = get_binary_bits(t, m);
                std::vector<int> clause;
                for (int bit = 0; bit < m; ++bit) {
                    clause.push_back((binaryT[bit] == '1') ? -getP(vertex, bit) : getP(vertex, bit));
                }
                add_clause(clause);
            }
        }

        // If start vertex of undirected graph has exactly 2 neighbors, neighbor positions can't be 3, 4, ..., n-1
        if (start_v <= (int)graph.adj_list.size() && graph.adj_list[start_v].size() == 2 && !graph.is_directed) {
            for (int neighbor : graph.adj_list[start_v]) {
                for (int t = 3; t < n; ++t) {
                    std::vector<char> binaryT = get_binary_bits(t, m);
                    std::vector<int> clause;
                    for (int bit = 0; bit < m; ++bit) {
                        clause.push_back((binaryT[bit] == '1') ? -getP(neighbor, bit) : getP(neighbor, bit));
                    }
                    add_clause(clause);
                }
            }
        }
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

    void vertex_start(int n, int m, const Graph& graph) {
        int start_v = graph.start_vertex;
        for (int i = 1; i <= n; ++i) {
            if (i == start_v) continue;
            for (int bit = 0; bit < m; ++bit) {
                if (bit == 1) {
                    add_clause({-getH(start_v, i), getP(i, bit)});
                } else {
                    add_clause({-getH(start_v, i), -getP(i, bit)});
                }
            }
        }
    }

    void vertex_end(int n, int m, const Graph& graph) {
        int start_v = graph.start_vertex;
        std::vector<char> binaryN = get_binary_bits(n, m);
        for (int i = 1; i <= n; ++i) {
            if (i == start_v) continue;
            for (int bit = 0; bit < m; ++bit) {
                if (binaryN[bit] == '1') {
                    add_clause({-getH(i, start_v), getP(i, bit)});
                } else {
                    add_clause({-getH(i, start_v), -getP(i, bit)});
                }
            }
        }
    }

    void vertex_positions_final(int n, int m, const Graph& graph) {
        int start_v = graph.start_vertex;
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                if (i == start_v || j == start_v || i == j) continue;

                add_clause({-getH(i, j), getP(i, 0), getP(j, 0)});
                add_clause({-getH(i, j), -getP(i, 0), -getP(j, 0)});

                add_clause({-getH(i, j), -getP(i, 0), getP(i, 1), getP(j, 1)});
                add_clause({-getH(i, j), -getP(i, 0), -getP(i, 1), -getP(j, 1)});
                add_clause({-getH(i, j), getP(i, 0), getP(i, 1), -getP(j, 1)});
                add_clause({-getH(i, j), getP(i, 0), -getP(i, 1), getP(j, 1)});

                for (int bit = 2; bit < m - 4; bit += 2) {
                    add_clause({-getH(i, j), getP(j, bit - 1), -getP(i, bit - 1), getP(j, bit), getP(i, bit)});
                    add_clause({-getH(i, j), getP(j, bit - 1), -getP(i, bit - 1), -getP(i, bit), getP(j, bit + 1), getP(i, bit + 1)});
                    add_clause({-getH(i, j), getP(j, bit - 1), -getP(i, bit - 1), -getP(i, bit), -getP(j, bit + 1), -getP(i, bit + 1)});
                    add_clause({-getH(i, j), -getP(j, bit - 1), -getP(i, bit), getP(j, bit)});
                    add_clause({-getH(i, j), -getP(j, bit - 1), getP(i, bit), -getP(j, bit)});
                    add_clause({-getH(i, j), getP(i, bit - 1), -getP(i, bit), getP(j, bit)});
                    add_clause({-getH(i, j), getP(i, bit - 1), getP(i, bit), -getP(j, bit)});
                    add_clause({-getH(i, j), getP(i, bit), getP(j, bit + 1), -getP(i, bit + 1)});
                    add_clause({-getH(i, j), getP(i, bit), -getP(j, bit + 1), getP(i, bit + 1)});
                    add_clause({-getH(i, j), -getP(j, bit), getP(j, bit + 1), -getP(i, bit + 1)});
                    add_clause({-getH(i, j), -getP(j, bit), -getP(j, bit + 1), getP(i, bit + 1)});
                }

                if (m % 2 == 1) {
                    int bit = m - 5;
                    add_clause({-getH(i, j), getP(j, bit - 1), -getP(i, bit - 1), getP(j, bit), getP(i, bit)});
                    add_clause({-getH(i, j), getP(j, bit - 1), -getP(i, bit - 1), -getP(j, bit), -getP(i, bit)});
                    add_clause({-getH(i, j), getP(i, bit - 1), getP(j, bit), -getP(i, bit)});
                    add_clause({-getH(i, j), getP(i, bit - 1), -getP(j, bit), getP(i, bit)});
                    add_clause({-getH(i, j), -getP(j, bit - 1), getP(j, bit), -getP(i, bit)});
                    add_clause({-getH(i, j), -getP(j, bit - 1), -getP(j, bit), getP(i, bit)});
                }

                int bit = m;
                add_clause({-getH(i, j), getP(i, bit - 1), getP(i, bit - 4), -getP(j, bit - 1)});
                add_clause({-getH(i, j), getP(i, bit - 1), -getP(j, bit - 1), -getP(j, bit - 2)});
                add_clause({-getH(i, j), getP(i, bit - 1), -getP(j, bit - 1), -getP(j, bit - 3)});
                add_clause({-getH(i, j), getP(i, bit - 1), -getP(j, bit - 1), -getP(j, bit - 4)});
                add_clause({-getH(i, j), getP(i, bit - 3), -getP(j, bit - 3), -getP(j, bit - 4)});
                add_clause({-getH(i, j), -getP(i, bit - 4), -getP(j, bit - 5), getP(j, bit - 4)});
                add_clause({-getH(i, j), getP(i, bit - 2), -getP(i, bit - 3), getP(j, bit - 2), getP(j, bit - 3)});
                add_clause({-getH(i, j), getP(i, bit - 4), getP(i, bit - 5), -getP(j, bit - 4)});
                add_clause({-getH(i, j), -getP(i, bit - 1), getP(j, bit - 1)});
                add_clause({-getH(i, j), -getP(i, bit - 2), -getP(i, bit - 3), -getP(j, bit - 2), getP(j, bit - 3)});
                add_clause({-getH(i, j), -getP(i, bit - 3), -getP(i, bit - 4), -getP(i, bit - 5), getP(j, bit - 5), -getP(j, bit - 3)});
                add_clause({-getH(i, j), getP(i, bit - 2), getP(i, bit - 4), -getP(j, bit - 2)});
                add_clause({-getH(i, j), getP(i, bit - 3), getP(i, bit - 4), -getP(j, bit - 3)});
                add_clause({-getH(i, j), getP(i, bit - 2), -getP(j, bit - 2), -getP(j, bit - 3)});
                add_clause({-getH(i, j), getP(i, bit - 2), -getP(j, bit - 2), -getP(j, bit - 4)});
                add_clause({-getH(i, j), -getP(i, bit - 4), getP(i, bit - 5), getP(j, bit - 4)});
                add_clause({-getH(i, j), getP(i, bit - 1), -getP(i, bit - 2), getP(j, bit - 1), getP(j, bit - 2)});
                add_clause({-getH(i, j), getP(i, bit - 4), -getP(i, bit - 5), getP(j, bit - 5), getP(j, bit - 4)});
                add_clause({-getH(i, j), getP(i, bit - 4), -getP(j, bit - 5), -getP(j, bit - 4)});
                add_clause({-getH(i, j), -getP(i, bit - 1), -getP(i, bit - 2), -getP(j, bit - 1), getP(j, bit - 2)});
                add_clause({-getH(i, j), getP(i, bit - 3), -getP(i, bit - 4), -getP(i, bit - 5), getP(j, bit - 5), getP(j, bit - 3)});
            }
        }
    }

    EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) override {
        context = ctx;
        H.clear();
        P.clear();
        int n = graph.v;
        int m = std::ceil(std::log2(n));
        if (std::pow(2, m) == n) m++;

        if (is_preprocessing) {
            preprocessing(graph, m);
        }

        add_default_variables(n, m, graph);
        vertex_outgoing_arcs(n);
        vertex_incoming_arcs(n);
        vertex_start(n, m, graph);
        vertex_end(n, m, graph);
        vertex_positions_final(n, m, graph);

        return context;
    }
};
