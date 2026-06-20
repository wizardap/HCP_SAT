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

class HCPPropagator : public CaDiCaL::ExternalPropagator {
public:
    int N;
    unordered_map<int, Edge> var_to_edge;
    unordered_map<pair<int, int>, int, PairHash> edge_to_var;
    queue<vector<int>> pending_clauses;
    size_t clause_idx = 0;
    bool is_directed = false;
    int check_calls = 0;

    HCPPropagator(int n, bool directed) : N(n), is_directed(directed) {
        this->is_lazy = true;
    }

    void notify_assignment (int lit, bool is_fixed) override {}
    void notify_new_decision_level () override {}
    void notify_backtrack (size_t new_level) override {}

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
            cerr << "[C++ Propagator] Valid Hamiltonian cycle found!" << endl;
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

int main(int argc, char* argv[]) {
    string filename = "-";
    int timeout = 600;
    int mode = 0; // 0 = Propagator, 1 = CEGAR

    if (argc >= 2) {
        filename = argv[1];
    }
    if (argc >= 3) {
        timeout = atoi(argv[2]);
    }
    if (argc >= 4) {
        mode = atoi(argv[3]);
    }

    istream* input = &cin;
    ifstream infile;

    if (filename != "-") {
        infile.open(filename);
        if (!infile.is_open()) {
            cerr << "Error opening file: " << filename << endl;
            return 1;
        }
        input = &infile;
    }

    CaDiCaL::Solver solver;
    HCPPropagator* propagator = nullptr;

    unordered_map<int, Edge> var_to_edge;
    unordered_map<pair<int, int>, int, PairHash> edge_to_var;
    int N = 0;
    bool is_directed = false;
    bool has_meta = false;
    int max_var = 0;

    string line;
    while (getline(*input, line)) {
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

    if (filename != "-") {
        infile.close();
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

    if (mode == 1) {
        // Run CEGAR mode
        run_cegar(solver, N, is_directed, var_to_edge, edge_to_var, max_var, elapsed_time, timeout);
    } else {
        // Run Propagator mode
        if (has_meta) {
            propagator = new HCPPropagator(N, is_directed);
            propagator->var_to_edge = var_to_edge;
            propagator->edge_to_var = edge_to_var;
            solver.connect_external_propagator(propagator);
            for (const auto& pair : propagator->var_to_edge) {
                solver.add_observed_var(pair.first);
            }
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
                int val = solver.val(i);
                cout << val << " ";
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

    solver_finished = true;
    if (timer_thread.joinable()) {
        timer_thread.join();
    }

    return 0;
}
