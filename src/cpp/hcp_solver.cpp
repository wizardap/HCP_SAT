#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <thread>
#include "cadical.hpp"

using namespace std;

// Graph representation matching Python's Graph.py
struct Graph {
    int v = 0;
    int e = 0;
    int start_vertex = 1;
    bool is_directed = false;
    vector<vector<int>> adj_list;

    void load_graph(const string &filename) {
        string norm_name = filename;
        replace(norm_name.begin(), norm_name.end(), '\\', '/');
        size_t last_slash = norm_name.find_last_of('/');
        string test_set = "";
        if (last_slash != string::npos) {
            size_t second_last = norm_name.find_last_of('/', last_slash - 1);
            if (second_last != string::npos) {
                test_set = norm_name.substr(second_last + 1, last_slash - second_last - 1);
            }
        }

        ifstream infile(filename);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << filename << endl;
            exit(1);
        }

        string line;
        if (test_set == "vset" || test_set == "graphs") {
            is_directed = (test_set == "vset");
            while (getline(infile, line)) {
                if (line.empty()) continue;
                if (line[0] == 'p') {
                    stringstream ss(line);
                    string p, cnf;
                    int n;
                    ss >> p >> cnf >> n;
                    v = n;
                    adj_list.resize(v + 1);
                } else if (line[0] == 'e') {
                    stringstream ss(line);
                    char e_char;
                    int u, w;
                    ss >> e_char >> u >> w;
                    if (find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (!is_directed) {
                        if (find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                            adj_list[w].push_back(u);
                        }
                    }
                } else if (isdigit(line[0])) {
                    stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (!is_directed) {
                        if (find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                            adj_list[w].push_back(u);
                        }
                    }
                }
            }
            if (!is_directed) {
                int min_deg = 1e9;
                for (int i = 1; i <= v; ++i) {
                    if ((int)adj_list[i].size() < min_deg) {
                        min_deg = adj_list[i].size();
                        start_vertex = i;
                    }
                }
            }
        } else if (test_set == "fhcpcs" || test_set == "fhcpsl" || test_set == "fhcppp") {
            is_directed = false;
            while (getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                        adj_list[w].push_back(u);
                    }
                }
            }
            int min_deg = 1e9;
            for (int i = 1; i <= v; ++i) {
                if ((int)adj_list[i].size() < min_deg) {
                    min_deg = adj_list[i].size();
                    start_vertex = i;
                }
            }
        } else if (test_set == "tsphcp") {
            is_directed = false;
            while (getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (u != -1 && w != -1) {
                        if (find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                            adj_list[u].push_back(w);
                        }
                    }
                }
            }
            int min_deg = 1e9;
            for (int i = 1; i <= v; ++i) {
                if ((int)adj_list[i].size() < min_deg) {
                    min_deg = adj_list[i].size();
                    start_vertex = i;
                }
            }
        } else {
            // Default fallback parser
            while (getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (u > 0 && w > 0) {
                        if (v == 0) {
                            v = max(v, max(u, w));
                            adj_list.resize(v + 1);
                        }
                        if (find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                            adj_list[u].push_back(w);
                        }
                        if (!is_directed) {
                            if (find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                                adj_list[w].push_back(u);
                            }
                        }
                    }
                }
            }
            int min_deg = 1e9;
            for (int i = 1; i <= v; ++i) {
                if ((int)adj_list[i].size() < min_deg) {
                    min_deg = adj_list[i].size();
                    start_vertex = i;
                }
            }
        }
        infile.close();

        for (int i = 1; i <= v; ++i) {
            e += adj_list[i].size();
        }
        if (!is_directed) e /= 2;
    }
};

struct Edge {
    int u, v;
};

struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

class BimanderEncoder {
public:
    void naive_amo(vector<vector<int>> &clauses, const vector<int> &variables) {
        int n = variables.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                clauses.push_back({-variables[i], -variables[j]});
            }
        }
    }

    void encode_amo(vector<vector<int>> &clauses, const vector<int> &variables, int &next_var) {
        int n = variables.size();
        if (n <= 1) return;

        int g = ceil(sqrt(n));
        int m_groups = ceil((double)n / g);

        if (m_groups <= 1) {
            naive_amo(clauses, variables);
            return;
        }

        vector<vector<int>> groups;
        for (int i = 0; i < m_groups; ++i) {
            int start = i * g;
            int end = min(start + g, n);
            if (start < end) {
                groups.push_back(vector<int>(variables.begin() + start, variables.begin() + end));
            }
        }

        int num_groups = groups.size();

        for (const auto& group : groups) {
            naive_amo(clauses, group);
        }

        int num_aux = ceil(log2(num_groups));
        if (num_aux > 0) {
            vector<int> aux_vars;
            for (int j = 0; j < num_aux; ++j) {
                aux_vars.push_back(next_var++);
            }

            for (int i = 0; i < num_groups; ++i) {
                for (int var : groups[i]) {
                    for (int j = 0; j < num_aux; ++j) {
                        int bit = (i >> j) & 1;
                        if (bit == 1) {
                            clauses.push_back({-var, aux_vars[j]});
                        } else {
                            clauses.push_back({-var, -aux_vars[j]});
                        }
                    }
                }
            }
        }
    }

    void encode_exactly_one(vector<vector<int>> &clauses, const vector<int> &variables, int &next_var) {
        if (variables.empty()) return;
        clauses.push_back(variables);
        encode_amo(clauses, variables, next_var);
    }
};

class HCPPropagator : public CaDiCaL::ExternalPropagator {
public:
    int N;
    unordered_map<int, Edge> var_to_edge;
    unordered_map<pair<int, int>, int, PairHash> edge_to_var;
    queue<vector<int>> pending_clauses;
    size_t clause_idx = 0;
    bool is_directed = false;
    int check_calls = 0;

    // Active propagator tracking
    vector<int> next_of;
    struct AssignmentChange {
        int u;
        int prev_val;
    };
    vector<vector<AssignmentChange>> level_stack;
    int current_level = 0;

    // Stats
    long long num_assignments = 0;
    long long num_backtracks = 0;
    long long num_levels = 0;
    long long num_subcycles_found = 0;

    HCPPropagator(int n, bool directed) : N(n), is_directed(directed) {
        this->is_lazy = false; // Set to false for active propagation!
        next_of.assign(N + 1, 0);
        level_stack.resize(1); // Level 0 exists initially
        current_level = 0;
    }

    ~HCPPropagator() override {
        cerr << "[C++ Propagator] Summary: assignments=" << num_assignments 
             << " backtracks=" << num_backtracks 
             << " levels=" << num_levels 
             << " subcycles=" << num_subcycles_found << endl;
    }

    void notify_new_decision_level () override {
        num_levels++;
        current_level++;
        if (current_level >= (int)level_stack.size()) {
            level_stack.resize(current_level + 1);
        }
    }

    void notify_backtrack (size_t new_level) override {
        num_backtracks++;
        while (current_level > (int)new_level) {
            if (current_level < (int)level_stack.size()) {
                for (auto it = level_stack[current_level].rbegin(); it != level_stack[current_level].rend(); ++it) {
                    next_of[it->u] = it->prev_val;
                }
                level_stack[current_level].clear();
            }
            current_level--;
        }
        if (level_stack.size() > new_level + 1) {
            level_stack.resize(new_level + 1);
        }
        
        // Clear pending clauses on backtrack
        while (!pending_clauses.empty()) {
            pending_clauses.pop();
        }
        clause_idx = 0;
    }

    void notify_assignment (int lit, bool is_fixed) override {
        if (lit > 0) {
            int abs_lit = abs(lit);
            if (var_to_edge.count(abs_lit)) {
                num_assignments++;
                Edge e = var_to_edge[abs_lit];
                int u = e.u;
                int v = e.v;

                if (current_level >= (int)level_stack.size()) {
                    level_stack.resize(current_level + 1);
                }

                int prev_val = next_of[u];
                level_stack[current_level].push_back({u, prev_val});
                next_of[u] = v;

                // Trace path from v to see if it reaches u
                int curr = v;
                int steps = 0;
                vector<int> cycle_vertices;
                cycle_vertices.push_back(u);

                while (curr != 0 && steps <= N) {
                    cycle_vertices.push_back(curr);
                    if (curr == u) {
                        break;
                    }
                    curr = next_of[curr];
                    steps++;
                }

                // If a cycle is detected and its length is less than N, it's a subcycle!
                if (curr == u && (int)cycle_vertices.size() - 1 < N) {
                    num_subcycles_found++;
                    vector<int> clause_cw;
                    for (size_t i = 0; i < cycle_vertices.size() - 1; ++i) {
                        int su = cycle_vertices[i];
                        int sv = cycle_vertices[i+1];
                        auto it = edge_to_var.find({su, sv});
                        if (it != edge_to_var.end()) {
                            clause_cw.push_back(-it->second);
                        }
                    }
                    if (!clause_cw.empty()) {
                        pending_clauses.push(clause_cw);
                    }

                    if (!is_directed) {
                        vector<int> clause_ccw;
                        for (size_t i = 0; i < cycle_vertices.size() - 1; ++i) {
                            int su = cycle_vertices[i];
                            int sv = cycle_vertices[i+1];
                            auto it = edge_to_var.find({sv, su});
                            if (it != edge_to_var.end()) {
                                clause_ccw.push_back(-it->second);
                            }
                        }
                        if (!clause_ccw.empty()) {
                            pending_clauses.push(clause_ccw);
                        }
                    }
                }
            }
        }
    }

    bool cb_check_found_model (const std::vector<int> &model) override {
        check_calls++;
        unordered_map<int, int> adj;
        for (int val : model) {
            int abs_val = abs(val);
            if (val > 0 && var_to_edge.count(abs_val)) {
                Edge e = var_to_edge[abs_val];
                adj[e.u] = e.v;
            }
        }

        vector<vector<Edge>> cycles;
        vector<int> visited(N + 1, 0);
        for (int start = 1; start <= N; ++start) {
            if (visited[start] == 0 && adj.count(start)) {
                vector<int> path;
                int curr = start;
                while (curr != 0 && visited[curr] == 0) {
                    visited[curr] = 1;
                    path.push_back(curr);
                    if (adj.count(curr)) {
                        curr = adj[curr];
                    } else {
                        curr = 0;
                    }
                }
                if (curr != 0 && visited[curr] == 1) {
                    vector<Edge> cycle;
                    auto it = find(path.begin(), path.end(), curr);
                    for (auto jt = it; jt != path.end(); ++jt) {
                        int u = *jt;
                        int v = (jt + 1 == path.end()) ? curr : *(jt + 1);
                        cycle.push_back({u, v});
                    }
                    if (!cycle.empty()) {
                        cycles.push_back(cycle);
                    }
                }
                for (int u : path) {
                    visited[u] = 2;
                }
            }
        }

        cerr << "[C++ Propagator] check " << check_calls << ": active_edges=" << adj.size() 
             << " cycles_found=" << cycles.size() << endl;

        if (cycles.size() == 1 && cycles[0].size() == N) {
            return true;
        }

        int added_clauses = 0;
        for (const auto& cycle : cycles) {
            vector<int> clause_cw;
            for (const auto& e : cycle) {
                auto it = edge_to_var.find({e.u, e.v});
                if (it != edge_to_var.end()) {
                    clause_cw.push_back(-it->second);
                }
            }
            if (!clause_cw.empty()) {
                pending_clauses.push(clause_cw);
                added_clauses++;
            }

            if (!is_directed) {
                vector<int> clause_ccw;
                for (const auto& e : cycle) {
                    auto it = edge_to_var.find({e.v, e.u});
                    if (it != edge_to_var.end()) {
                        clause_ccw.push_back(-it->second);
                    }
                }
                if (!clause_ccw.empty()) {
                    pending_clauses.push(clause_ccw);
                    added_clauses++;
                }
            }
        }

        if (added_clauses == 0) {
            vector<int> block_clause;
            for (const auto& pair : adj) {
                auto it = edge_to_var.find({pair.first, pair.second});
                if (it != edge_to_var.end()) {
                    block_clause.push_back(-it->second);
                }
            }
            if (!block_clause.empty()) {
                pending_clauses.push(block_clause);
            }
        }

        return false;
    }

    bool cb_has_external_clause () override {
        return !pending_clauses.empty();
    }

    int cb_add_external_clause_lit () override {
        if (pending_clauses.empty()) return 0;
        const auto &clause = pending_clauses.front();
        if (clause_idx < clause.size()) {
            return clause[clause_idx++];
        } else {
            pending_clauses.pop();
            clause_idx = 0;
            return 0;
        }
    }
};

int lfsr(int n, int size, int xor_tap) {
    int m = n << 1;
    int x = m & (1 << size);
    if (x) {
        m = m - x + 1;
    }
    m ^= x >> xor_tap;
    return m;
}

pair<int, int> get_cycle_and_nbits(int nNode, int cycle_override) {
    int cycle = cycle_override;
    int nBits = 0;
    int k = 1;
    while (true) {
        if ((cycle % (1 << k)) == 0) {
            nBits++;
        } else {
            break;
        }
        k++;
    }
    if ((cycle % 3) == 0) nBits += 2;
    if ((cycle % 5) == 0) nBits += 3;
    if ((cycle % 7) == 0) nBits += 3;
    if ((cycle % 511) == 0) nBits += 9;
    if ((cycle % 1023) == 0) nBits += 10;
    if ((cycle % 2047) == 0) nBits += 11;
    return {cycle, nBits};
}

void enforce_non_zero_lfsr(vector<vector<int>> &clauses, int n, int cycle, const vector<vector<int>> &P) {
    for (int i = 1; i <= n; ++i) {
        int b = 0;
        int k = 1;
        while (true) {
            if ((cycle % (1 << k)) == 0) {
                b++;
            } else {
                break;
            }
            k++;
        }

        if ((cycle % 3) == 0) {
            clauses.push_back({P[i][b], P[i][b + 1]});
            b += 2;
        }
        if ((cycle % 5) == 0) {
            clauses.push_back({-P[i][b], -P[i][b + 2]});
            clauses.push_back({-P[i][b + 1], -P[i][b + 2]});
            b += 3;
        }
        if ((cycle % 7) == 0) {
            clauses.push_back({P[i][b], P[i][b + 1], P[i][b + 2]});
            b += 3;
        }
        if ((cycle % 511) == 0) {
            vector<int> c;
            for (int j = 0; j < 9; ++j) c.push_back(P[i][b + j]);
            clauses.push_back(c);
            b += 9;
        }
        if ((cycle % 1023) == 0) {
            vector<int> c;
            for (int j = 0; j < 10; ++j) c.push_back(P[i][b + j]);
            clauses.push_back(c);
            b += 10;
        }
        if ((cycle % 2047) == 0) {
            vector<int> c;
            for (int j = 0; j < 11; ++j) c.push_back(P[i][b + j]);
            clauses.push_back(c);
            b += 11;
        }
    }
}

void init_starting_position(vector<vector<int>> &clauses, int first, int cycle, const vector<vector<int>> &P) {
    int b = 0;
    int k = 1;
    while (true) {
        if ((cycle % (1 << k)) == 0) {
            clauses.push_back({-P[first][b]});
            b++;
        } else {
            break;
        }
        k++;
    }

    if ((cycle % 3) == 0) {
        clauses.push_back({P[first][b]}); b++;
        clauses.push_back({-P[first][b]}); b++;
    }
    if ((cycle % 5) == 0) {
        clauses.push_back({-P[first][b]}); b++;
        clauses.push_back({-P[first][b]}); b++;
        clauses.push_back({-P[first][b]}); b++;
    }
    if ((cycle % 7) == 0) {
        clauses.push_back({P[first][b]}); b++;
        clauses.push_back({-P[first][b]}); b++;
        clauses.push_back({-P[first][b]}); b++;
    }
    if ((cycle % 511) == 0) {
        clauses.push_back({P[first][b]}); b++;
        for (int k = 2; k < 10; ++k) {
            clauses.push_back({-P[first][b]}); b++;
        }
    }
    if ((cycle % 1023) == 0) {
        clauses.push_back({P[first][b]}); b++;
        for (int k = 2; k < 11; ++k) {
            clauses.push_back({-P[first][b]}); b++;
        }
    }
    if ((cycle % 2047) == 0) {
        clauses.push_back({P[first][b]}); b++;
        for (int k = 2; k < 12; ++k) {
            clauses.push_back({-P[first][b]}); b++;
        }
    }
}

void init_termination_position(vector<vector<int>> &clauses, int n, int first, const vector<int> &first_neighbors, int cycle, const vector<vector<int>> &P, const vector<vector<int>> &H) {
    for (int neighbor : first_neighbors) {
        int b = 0;
        int k = 1;
        while (true) {
            if ((cycle % (1 << k)) == 0) {
                int expected_val = (((n - 1) & (1 << (k - 1))) != 0);
                int p_lit = expected_val ? P[neighbor][b] : -P[neighbor][b];
                clauses.push_back({-H[neighbor][first], p_lit});
                b++;
            } else {
                break;
            }
            k++;
        }

        if ((cycle % 3) == 0) {
            int mask = 1;
            for (int i = 0; i < (n - 1) % 3; ++i) {
                mask = lfsr(mask, 2, 1);
            }
            for (int i = 0; i < 2; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 2;
        }

        if ((cycle % 5) == 0) {
            int mask = (n + 4) % 5;
            for (int i = 0; i < 3; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 3;
        }

        if ((cycle % 7) == 0) {
            int mask = 1;
            for (int i = 0; i < (n - 1) % 7; ++i) {
                mask = lfsr(mask, 3, 1);
            }
            for (int i = 0; i < 3; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 3;
        }

        if ((cycle % 511) == 0) {
            int mask = 1;
            for (int i = 0; i < (n - 1) % 511; ++i) {
                mask = lfsr(mask, 9, 4);
            }
            for (int i = 0; i < 9; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 9;
        }

        if ((cycle % 1023) == 0) {
            int mask = 1;
            for (int i = 0; i < (n - 1) % 1023; ++i) {
                mask = lfsr(mask, 10, 3);
            }
            for (int i = 0; i < 10; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 10;
        }

        if ((cycle % 2047) == 0) {
            int mask = 1;
            for (int i = 0; i < (n - 1) % 2047; ++i) {
                mask = lfsr(mask, 11, 2);
            }
            for (int i = 0; i < 11; ++i) {
                int expected_val = (mask & 1) != 0;
                int p_lit = expected_val ? P[neighbor][b + i] : -P[neighbor][b + i];
                clauses.push_back({-H[neighbor][first], p_lit});
                mask = mask >> 1;
            }
            b += 11;
        }
    }
}

void add_transitions(vector<vector<int>> &clauses, int n, const vector<vector<int>> &adj_list, int first, int cycle, const vector<vector<int>> &P, const vector<vector<int>> &H) {
    for (int i = 1; i <= n; ++i) {
        for (int j : adj_list[i]) {
            if (j != first) {
                int b = 0;
                if ((cycle % 2) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], P[i][b]});
                    clauses.push_back({-H[i][j], -P[j][b], -P[i][b]});
                    b += 1;
                }

                int k = 2;
                while (true) {
                    if ((cycle % (1 << k)) == 0) {
                        for (int l = 1; l < k; ++l) {
                            clauses.push_back({-H[i][j], P[i][b - l], P[j][b], -P[i][b]});
                            clauses.push_back({-H[i][j], P[i][b - l], -P[j][b], P[i][b]});
                        }
                        vector<int> c1, c2;
                        for (int l = 1; l < k; ++l) {
                            c1.push_back(-P[i][b - l]);
                            c2.push_back(-P[i][b - l]);
                        }
                        c1.push_back(-H[i][j]); c1.push_back(P[j][b]); c1.push_back(P[i][b]);
                        c2.push_back(-H[i][j]); c2.push_back(-P[j][b]); c2.push_back(-P[i][b]);
                        clauses.push_back(c1);
                        clauses.push_back(c2);
                        b += 1;
                    } else {
                        break;
                    }
                    k++;
                }

                if ((cycle % 3) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], -P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b], P[i][b + 1]});
                    clauses.push_back({-H[i][j], P[j][b + 1], P[i][b], -P[i][b + 1]});
                    clauses.push_back({-H[i][j], P[j][b + 1], -P[i][b], P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b + 1], P[i][b], P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b + 1], -P[i][b], -P[i][b + 1]});
                    b += 2;
                }

                if ((cycle % 5) == 0) {
                    clauses.push_back({-H[i][j], -P[j][b], -P[i][b]});
                    clauses.push_back({-H[i][j], -P[j][b], -P[i][b + 2]});
                    clauses.push_back({-H[i][j], P[j][b], P[i][b], P[i][b + 2]});
                    clauses.push_back({-H[i][j], P[j][b + 1], P[i][b], -P[i][b + 1]});
                    clauses.push_back({-H[i][j], P[j][b + 1], -P[i][b], P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b + 1], P[i][b], P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b + 1], -P[i][b], -P[i][b + 1]});
                    clauses.push_back({-H[i][j], -P[j][b + 2], P[i][b]});
                    clauses.push_back({-H[i][j], -P[j][b + 2], P[i][b + 1]});
                    clauses.push_back({-H[i][j], P[j][b + 2], -P[i][b], -P[i][b + 1]});
                    b += 3;
                }

                if ((cycle % 7) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], -P[i][b + 2]});
                    clauses.push_back({-H[i][j], -P[j][b], P[i][b + 2]});
                    clauses.push_back({-H[i][j], P[j][b + 1], -P[i][b]});
                    clauses.push_back({-H[i][j], -P[j][b + 1], P[i][b]});
                    clauses.push_back({-H[i][j], P[j][b + 2], P[i][b + 1], -P[i][b + 2]});
                    clauses.push_back({-H[i][j], P[j][b + 2], -P[i][b + 1], P[i][b + 2]});
                    clauses.push_back({-H[i][j], -P[j][b + 2], P[i][b + 1], P[i][b + 2]});
                    clauses.push_back({-H[i][j], -P[j][b + 2], -P[i][b + 1], -P[i][b + 2]});
                    b += 3;
                }

                if ((cycle % 511) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], -P[i][b + 8]});
                    clauses.push_back({-H[i][j], -P[j][b], P[i][b + 8]});
                    for (int idx = 1; idx <= 4; ++idx) {
                        clauses.push_back({-H[i][j], P[j][b + idx], -P[i][b + idx - 1]});
                        clauses.push_back({-H[i][j], -P[j][b + idx], P[i][b + idx - 1]});
                    }
                    clauses.push_back({-H[i][j], P[j][b + 5], P[i][b + 4], -P[i][b + 8]});
                    clauses.push_back({-H[i][j], P[j][b + 5], -P[i][b + 4], P[i][b + 8]});
                    clauses.push_back({-H[i][j], -P[j][b + 5], P[i][b + 4], P[i + 0][b + 8]}); // wait, P[i][b+8]
                    // Fix typo in line above
                    clauses.pop_back();
                    clauses.push_back({-H[i][j], -P[j][b + 5], P[i][b + 4], P[i][b + 8]});
                    clauses.push_back({-H[i][j], -P[j][b + 5], -P[i][b + 4], -P[i][b + 8]});
                    for (int idx = 6; idx <= 8; ++idx) {
                        clauses.push_back({-H[i][j], P[j][b + idx], -P[i][b + idx - 1]});
                        clauses.push_back({-H[i][j], -P[j][b + idx], P[i][b + idx - 1]});
                    }
                    b += 9;
                }

                if ((cycle % 1023) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], -P[i][b + 9]});
                    clauses.push_back({-H[i][j], -P[j][b], P[i][b + 9]});
                    for (int idx = 1; idx <= 6; ++idx) {
                        clauses.push_back({-H[i][j], P[j][b + idx], -P[i][b + idx - 1]});
                        clauses.push_back({-H[i][j], -P[j][b + idx], P[i][b + idx - 1]});
                    }
                    clauses.push_back({-H[i][j], P[j][b + 7], P[i][b + 6], -P[i][b + 9]});
                    clauses.push_back({-H[i][j], P[j][b + 7], -P[i][b + 6], P[i][b + 9]});
                    clauses.push_back({-H[i][j], -P[j][b + 7], P[i][b + 6], P[i][b + 9]});
                    clauses.push_back({-H[i][j], -P[j][b + 7], -P[i][b + 6], -P[i][b + 9]});
                    for (int idx = 8; idx <= 9; ++idx) {
                        clauses.push_back({-H[i][j], P[j][b + idx], -P[i][b + idx - 1]});
                        clauses.push_back({-H[i][j], -P[j][b + idx], P[i][b + idx - 1]});
                    }
                    b += 10;
                }

                if ((cycle % 2047) == 0) {
                    clauses.push_back({-H[i][j], P[j][b], -P[i][b + 10]});
                    clauses.push_back({-H[i][j], -P[j][b], P[i][b + 10]});
                    for (int idx = 1; idx <= 8; ++idx) {
                        clauses.push_back({-H[i][j], P[j][b + idx], -P[i][b + idx - 1]});
                        clauses.push_back({-H[i][j], -P[j][b + idx], P[i][b + idx - 1]});
                    }
                    clauses.push_back({-H[i][j], P[j][b + 9], P[i][b + 8], -P[i][b + 10]});
                    clauses.push_back({-H[i][j], P[j][b + 9], -P[i][b + 8], P[i][b + 10]});
                    clauses.push_back({-H[i][j], -P[j][b + 9], P[i][b + 8], P[i][b + 10]});
                    clauses.push_back({-H[i][j], -P[j][b + 9], -P[i][b + 8], -P[i][b + 10]});
                    clauses.push_back({-H[i][j], P[j][b + 10], -P[i][b + 9]});
                    clauses.push_back({-H[i][j], -P[j][b + 10], P[i][b + 9]});
                    b += 11;
                }
            }
        }
    }
}

bool run_cegar(CaDiCaL::Solver &solver, int N, bool is_directed, 
               const unordered_map<int, Edge> &var_to_edge,
               const unordered_map<pair<int, int>, int, PairHash> &edge_to_var,
               int max_var, double &elapsed_time, int timeout) {
    
    int cegar_calls = 0;
    auto start_time = chrono::steady_clock::now();

    while (true) {
        cegar_calls++;
        auto now = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(now - start_time).count();
        if (elapsed >= timeout) {
            elapsed_time = elapsed;
            cout << "STATUS: TIMEOUT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            return false;
        }

        int res = solver.solve();

        now = chrono::steady_clock::now();
        elapsed = chrono::duration<double>(now - start_time).count();
        if (res == 0) {
            elapsed_time = elapsed;
            cout << "STATUS: TIMEOUT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            return false;
        } else if (res == 20) {
            elapsed_time = elapsed;
            cout << "STATUS: UNSAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            return false;
        }

        unordered_map<int, int> adj;
        for (int i = 1; i <= max_var; ++i) {
            int val = solver.val(i);
            if (val > 0 && var_to_edge.count(i)) {
                Edge e = var_to_edge.at(i);
                adj[e.u] = e.v;
            }
        }

        vector<vector<Edge>> cycles;
        vector<int> visited(N + 1, 0);
        for (int start = 1; start <= N; ++start) {
            if (visited[start] == 0 && adj.count(start)) {
                vector<int> path;
                int curr = start;
                while (curr != 0 && visited[curr] == 0) {
                    visited[curr] = 1;
                    path.push_back(curr);
                    if (adj.count(curr)) {
                        curr = adj[curr];
                    } else {
                        curr = 0;
                    }
                }
                if (curr != 0 && visited[curr] == 1) {
                    vector<Edge> cycle;
                    auto it = find(path.begin(), path.end(), curr);
                    for (auto jt = it; jt != path.end(); ++jt) {
                        int u = *jt;
                        int v = (jt + 1 == path.end()) ? curr : *(jt + 1);
                        cycle.push_back({u, v});
                    }
                    if (!cycle.empty()) {
                        cycles.push_back(cycle);
                    }
                }
                for (int u : path) {
                    visited[u] = 2;
                }
            }
        }

        cerr << "[C++ CEGAR] check " << cegar_calls << ": active_edges=" << adj.size() 
             << " cycles_found=" << cycles.size() << endl;

        if (cycles.size() == 1 && cycles[0].size() == N) {
            elapsed_time = elapsed;
            cout << "STATUS: SAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            cout << "MODEL: ";
            for (int i = 1; i <= max_var; ++i) {
                cout << solver.val(i) << " ";
            }
            cout << "0" << endl;
            return true;
        }

        int added = 0;
        for (const auto& cycle : cycles) {
            vector<int> clause_cw;
            for (const auto& e : cycle) {
                auto it = edge_to_var.find({e.u, e.v});
                if (it != edge_to_var.end()) {
                    clause_cw.push_back(-it->second);
                }
            }
            if (!clause_cw.empty()) {
                for (int l : clause_cw) solver.add(l);
                solver.add(0);
                added++;
            }

            if (!is_directed) {
                vector<int> clause_ccw;
                for (const auto& e : cycle) {
                    auto it = edge_to_var.find({e.v, e.u});
                    if (it != edge_to_var.end()) {
                        clause_ccw.push_back(-it->second);
                    }
                }
                if (!clause_ccw.empty()) {
                    for (int l : clause_ccw) solver.add(l);
                    solver.add(0);
                    added++;
                }
            }
        }

        if (added == 0) {
            for (const auto& pair : adj) {
                auto it = edge_to_var.find({pair.first, pair.second});
                if (it != edge_to_var.end()) {
                    solver.add(-it->second);
                }
            }
            solver.add(0);
        }
    }
}

bool run_stdin_solver(int timeout, int solver_mode, int seed) {
    CaDiCaL::Solver solver;
    if (seed != 0) {
        solver.set("seed", seed);
    }
    HCPPropagator* propagator = nullptr;

    unordered_map<int, Edge> var_to_edge;
    unordered_map<pair<int, int>, int, PairHash> edge_to_var;
    int N = 0;
    bool is_directed = false;
    bool has_meta = false;
    int max_var = 0;

    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        if (line[0] == 'c') {
            stringstream ss(line);
            string c, tag;
            ss >> c >> tag;
            if (tag == "hcp_metadata") {
                ss >> N >> is_directed;
                has_meta = true;
            } else if (tag == "edge" && has_meta) {
                int var_id, u, v;
                ss >> var_id >> u >> v;
                var_to_edge[var_id] = {u, v};
                edge_to_var[{u, v}] = var_id;
                max_var = max(max_var, var_id);
            }
        } else if (line[0] == 'p') {
            stringstream ss(line);
            string p, cnf;
            int vars, clauses;
            ss >> p >> cnf >> vars >> clauses;
            max_var = max(max_var, vars);
        } else {
            stringstream ss(line);
            int lit;
            vector<int> clause;
            while (ss >> lit) {
                if (lit == 0) break;
                clause.push_back(lit);
                max_var = max(max_var, abs(lit));
            }
            if (!clause.empty()) {
                for (int l : clause) {
                    solver.add(l);
                }
                solver.add(0);
            }
        }
    }

    // Set up terminator thread for global timeout
    bool solver_finished = false;
    thread timer_thread([&solver, &solver_finished, timeout]() {
        auto start = chrono::steady_clock::now();
        while (true) {
            if (solver_finished) break;
            auto now = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();
            if (elapsed >= timeout) {
                solver.terminate();
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });

    double elapsed_time = 0;

    if (has_meta) {
        if (solver_mode == 1) {
            // Run CEGAR mode
            run_cegar(solver, N, is_directed, var_to_edge, edge_to_var, max_var, elapsed_time, timeout);
        } else {
            // Run Propagator mode
            propagator = new HCPPropagator(N, is_directed);
            propagator->var_to_edge = var_to_edge;
            propagator->edge_to_var = edge_to_var;
            solver.connect_external_propagator(propagator);
            for (const auto& pair : propagator->var_to_edge) {
                solver.add_observed_var(pair.first);
            }

            auto solve_start = chrono::steady_clock::now();
            int res = solver.solve();
            auto solve_end = chrono::steady_clock::now();

            elapsed_time = chrono::duration<double>(solve_end - solve_start).count();

            if (res == 10) {
                cout << "STATUS: SAT" << endl;
                cout << "TIME: " << elapsed_time << endl;
                cout << "MODEL: ";
                for (int i = 1; i <= max_var; ++i) {
                    cout << solver.val(i) << " ";
                }
                cout << "0" << endl;
            } else if (res == 20) {
                cout << "STATUS: UNSAT" << endl;
                cout << "TIME: " << elapsed_time << endl;
            } else {
                cout << "STATUS: TIMEOUT" << endl;
                cout << "TIME: " << elapsed_time << endl;
            }

            if (propagator) {
                delete propagator;
            }
        }
    } else {
        // Standard solving
        auto solve_start = chrono::steady_clock::now();
        int res = solver.solve();
        auto solve_end = chrono::steady_clock::now();

        elapsed_time = chrono::duration<double>(solve_end - solve_start).count();

        if (res == 10) {
            cout << "STATUS: SAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            cout << "MODEL: ";
            for (int i = 1; i <= max_var; ++i) {
                cout << solver.val(i) << " ";
            }
            cout << "0" << endl;
        } else if (res == 20) {
            cout << "STATUS: UNSAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
        } else {
            cout << "STATUS: TIMEOUT" << endl;
            cout << "TIME: " << elapsed_time << endl;
        }
    }

    solver_finished = true;
    if (timer_thread.joinable()) {
        timer_thread.join();
    }

    return true;
}

void print_help(char* argv[]) {
    cout << "Usage: " << argv[0] << " [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -d, --data <path>        Path to the HCP graph file (required)" << endl;
    cout << "  -t, --timeout <seconds>   Time budget for solving (default: 600)" << endl;
    cout << "  -c, --cycle <limit>       CRT cycle size override (default: 12 if incremental, N if non-incremental)" << endl;
    cout << "  --incremental <0|1>      1 = Incremental (CEGAR/Propagator), 0 = Non-incremental (default: 1)" << endl;
    cout << "  --solver-mode <0|1>      0 = Propagator (default), 1 = CEGAR (only active if incremental)" << endl;
    cout << "  --seed <value>           Random seed for the solver (default: 0)" << endl;
    cout << "  -h, --help               Show this help message" << endl;
}

int main(int argc, char* argv[]) {
    string graph_path = "";
    int timeout = 600;
    int cycle_override = -1;
    int incremental = 1;
    int solver_mode = 0; // 0 = Propagator, 1 = CEGAR
    int seed = 0;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) graph_path = argv[++i];
        } else if (arg == "-t" || arg == "--timeout") {
            if (i + 1 < argc) timeout = atoi(argv[++i]);
        } else if (arg == "-c" || arg == "--cycle") {
            if (i + 1 < argc) cycle_override = atoi(argv[++i]);
        } else if (arg == "--incremental") {
            if (i + 1 < argc) incremental = atoi(argv[++i]);
        } else if (arg == "--solver-mode") {
            if (i + 1 < argc) solver_mode = atoi(argv[++i]);
        } else if (arg == "--seed") {
            if (i + 1 < argc) seed = atoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv);
            return 0;
        }
    }

    if (graph_path.empty()) {
        cerr << "Error: Graph path (-d/--data) is required." << endl;
        print_help(argv);
        return 1;
    }

    if (graph_path == "-") {
        run_stdin_solver(timeout, solver_mode, seed);
        return 0;
    }

    // 1. Load Graph
    Graph graph;
    cerr << "Loading graph: " << graph_path << "..." << endl;
    graph.load_graph(graph_path);
    cerr << "Graph dimensions: N = " << graph.v << ", E = " << graph.e 
         << ", Directed = " << (graph.is_directed ? "YES" : "NO") 
         << ", Start Vertex = " << graph.start_vertex << endl;

    // 2. Determine cycle length
    int cycle = cycle_override;
    if (cycle == -1) {
        cycle = (incremental == 1) ? 12 : graph.v;
    }

    // 3. Build CNF
    cerr << "Building CNF using CRT Successor Encoding (Cycle limit: " << cycle << ")..." << endl;
    int next_var = 1;
    
    // Map directed edge variables H
    vector<vector<int>> H(graph.v + 1, vector<int>(graph.v + 1, 0));
    unordered_map<int, Edge> var_to_edge;
    unordered_map<pair<int, int>, int, PairHash> edge_to_var;
    for (int i = 1; i <= graph.v; ++i) {
        for (int j : graph.adj_list[i]) {
            H[i][j] = next_var++;
            var_to_edge[H[i][j]] = {i, j};
            edge_to_var[{i, j}] = H[i][j];
        }
    }

    // Determine bits for counters
    auto cycle_n_bits = get_cycle_and_nbits(graph.v, cycle);
    int actual_cycle = cycle_n_bits.first;
    int nBits = cycle_n_bits.second;

    // Map bit position variables P
    vector<vector<int>> P(graph.v + 1, vector<int>(nBits, 0));
    for (int i = 1; i <= graph.v; ++i) {
        for (int b = 0; b < nBits; ++b) {
            P[i][b] = next_var++;
        }
    }

    vector<vector<int>> clauses;

    // Cardinality constraints (Exactly-One outgoing and incoming)
    BimanderEncoder card_encoder;
    for (int i = 1; i <= graph.v; ++i) {
        vector<int> outgoing;
        for (int j : graph.adj_list[i]) {
            outgoing.push_back(H[i][j]);
        }
        card_encoder.encode_exactly_one(clauses, outgoing, next_var);
    }

    // Enforce exactly one incoming
    for (int j = 1; j <= graph.v; ++j) {
        vector<int> incoming;
        for (int i = 1; i <= graph.v; ++i) {
            if (find(graph.adj_list[i].begin(), graph.adj_list[i].end(), j) != graph.adj_list[i].end()) {
                incoming.push_back(H[i][j]);
            }
        }
        card_encoder.encode_exactly_one(clauses, incoming, next_var);
    }

    // Connect one neighbor back to start
    vector<int> first_neighbors;
    for (int u = 1; u <= graph.v; ++u) {
        if (find(graph.adj_list[u].begin(), graph.adj_list[u].end(), graph.start_vertex) != graph.adj_list[u].end()) {
            first_neighbors.push_back(u);
        }
    }
    vector<int> alo_back;
    for (int neighbor : first_neighbors) {
        alo_back.push_back(H[neighbor][graph.start_vertex]);
    }
    clauses.push_back(alo_back);

    // Symmetry breaking
    if (!graph.is_directed) {
        for (int i = 0; i < (int)first_neighbors.size(); ++i) {
            vector<int> clause;
            for (int j = 0; j < i; ++j) {
                clause.push_back(H[graph.start_vertex][first_neighbors[j]]);
            }
            clause.push_back(-H[first_neighbors[i]][graph.start_vertex]);
            clauses.push_back(clause);
        }
    }

    // Enforce transitions and starting positions
    enforce_non_zero_lfsr(clauses, graph.v, actual_cycle, P);
    init_starting_position(clauses, graph.start_vertex, actual_cycle, P);
    init_termination_position(clauses, graph.v, graph.start_vertex, first_neighbors, actual_cycle, P, H);
    add_transitions(clauses, graph.v, graph.adj_list, graph.start_vertex, actual_cycle, P, H);

    cerr << "CNF Built: Variables = " << next_var - 1 << ", Clauses = " << clauses.size() << endl;

    // 4. Initialize CaDiCaL Solver
    CaDiCaL::Solver solver;
    if (seed != 0) {
        solver.set("seed", seed);
    }
    for (const auto& clause : clauses) {
        for (int lit : clause) {
            solver.add(lit);
        }
        solver.add(0);
    }

    // Set up terminator thread for global timeout
    bool solver_finished = false;
    thread timer_thread([&solver, &solver_finished, timeout]() {
        auto start = chrono::steady_clock::now();
        while (true) {
            if (solver_finished) break;
            auto now = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();
            if (elapsed >= timeout) {
                solver.terminate();
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });

    double elapsed_time = 0;
    HCPPropagator* propagator = nullptr;

    if (incremental == 1) {
        if (solver_mode == 1) {
            cerr << "Running C++ Stateful CEGAR Loop..." << endl;
            run_cegar(solver, graph.v, graph.is_directed, var_to_edge, edge_to_var, next_var - 1, elapsed_time, timeout);
        } else {
            cerr << "Running C++ External Propagator (Lazy)..." << endl;
            propagator = new HCPPropagator(graph.v, graph.is_directed);
            propagator->var_to_edge = var_to_edge;
            propagator->edge_to_var = edge_to_var;
            solver.connect_external_propagator(propagator);
            for (const auto& pair : propagator->var_to_edge) {
                solver.add_observed_var(pair.first);
            }

            auto solve_start = chrono::steady_clock::now();
            int res = solver.solve();
            auto solve_end = chrono::steady_clock::now();

            elapsed_time = chrono::duration<double>(solve_end - solve_start).count();

            if (res == 10) {
                cout << "STATUS: SAT" << endl;
                cout << "TIME: " << elapsed_time << endl;
                cout << "MODEL: ";
                for (int i = 1; i < next_var; ++i) {
                    cout << solver.val(i) << " ";
                }
                cout << "0" << endl;

                // Trace and print result cycle
                vector<int> model;
                for (int i = 1; i < next_var; ++i) {
                    model.push_back(solver.val(i));
                }
                
                unordered_map<int, int> adj;
                for (int val : model) {
                    if (val > 0 && var_to_edge.count(val)) {
                        Edge e = var_to_edge.at(val);
                        adj[e.u] = e.v;
                    }
                }

                cout << "Hamiltonian cycle: ";
                int vertex = graph.start_vertex;
                int num = 1;
                while (true) {
                    cout << vertex << " -> ";
                    if (adj.count(vertex)) {
                        vertex = adj[vertex];
                    } else {
                        cout << "BROKEN" << endl;
                        break;
                    }
                    if (vertex == graph.start_vertex) {
                        cout << graph.start_vertex << endl;
                        break;
                    }
                    num++;
                }
                cout << "Vertices of HC: " << num << endl;
                if (num == graph.v) {
                    cout << "Result verification: VALID" << endl;
                } else {
                    cout << "Result verification: INVALID" << endl;
                }
            } else if (res == 20) {
                cout << "STATUS: UNSAT" << endl;
                cout << "TIME: " << elapsed_time << endl;
            } else {
                cout << "STATUS: TIMEOUT" << endl;
                cout << "TIME: " << elapsed_time << endl;
            }

            if (propagator) {
                delete propagator;
            }
        }
    } else {
        // Non-incremental standard solving
        cerr << "Running standard non-incremental solve..." << endl;
        auto solve_start = chrono::steady_clock::now();
        int res = solver.solve();
        auto solve_end = chrono::steady_clock::now();

        elapsed_time = chrono::duration<double>(solve_end - solve_start).count();

        if (res == 10) {
            cout << "STATUS: SAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
            cout << "MODEL: ";
            for (int i = 1; i < next_var; ++i) {
                cout << solver.val(i) << " ";
            }
            cout << "0" << endl;

            vector<int> model;
            for (int i = 1; i < next_var; ++i) {
                model.push_back(solver.val(i));
            }
            
            unordered_map<int, int> adj;
            for (int val : model) {
                if (val > 0 && var_to_edge.count(val)) {
                    Edge e = var_to_edge.at(val);
                    adj[e.u] = e.v;
                }
            }

            cout << "Hamiltonian cycle: ";
            int vertex = graph.start_vertex;
            int num = 1;
            while (true) {
                cout << vertex << " -> ";
                if (adj.count(vertex)) {
                    vertex = adj[vertex];
                } else {
                    cout << "BROKEN" << endl;
                    break;
                }
                if (vertex == graph.start_vertex) {
                    cout << graph.start_vertex << endl;
                    break;
                }
                num++;
            }
            cout << "Vertices of HC: " << num << endl;
            if (num == graph.v) {
                cout << "Result verification: VALID" << endl;
            } else {
                cout << "Result verification: INVALID" << endl;
            }
        } else if (res == 20) {
            cout << "STATUS: UNSAT" << endl;
            cout << "TIME: " << elapsed_time << endl;
        } else {
            cout << "STATUS: TIMEOUT" << endl;
            cout << "TIME: " << elapsed_time << endl;
        }
    }

    solver_finished = true;
    if (timer_thread.joinable()) {
        timer_thread.join();
    }

    return 0;
}
