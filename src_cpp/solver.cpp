#include "solver.hpp"
#include "cadical.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <algorithm>
#include <queue>
#include <unordered_map>

class HCPPropagator : public CaDiCaL::ExternalPropagator {
public:
    int N;
    int opt_level;
    const Graph& graph;
    std::unordered_map<int, std::pair<int, int>> var_to_edge;
    std::map<std::pair<int, int>, int> edge_to_var;
    std::queue<std::vector<int>> pending_clauses;
    size_t clause_idx = 0;
    bool is_directed = false;
    int check_calls = 0;

    std::vector<int> next_of;
    std::vector<int> parent;
    std::vector<int> rank;

    struct AssignmentChange {
        int u;
        int prev_next_of;
        int p_node_u;
        int old_p_u;
        int p_node_v;
        int old_p_v;
        int r_node;
        int old_r;
    };
    std::vector<std::vector<AssignmentChange>> level_stack;
    int current_level = 0;

    long long num_assignments = 0;
    long long num_backtracks = 0;
    long long num_levels = 0;
    long long num_subcycles_found = 0;

    HCPPropagator(int n, bool directed, int opt_lvl, const Graph& g) : N(n), opt_level(opt_lvl), graph(g), is_directed(directed) {
        this->is_lazy = false; 
        next_of.assign(N + 1, 0);
        parent.resize(N + 1);
        rank.assign(N + 1, 0);
        for (int i = 1; i <= N; ++i) parent[i] = i;
        level_stack.resize(1); 
        current_level = 0;
    }

    ~HCPPropagator() override {
        std::cerr << "[C++ Propagator] Summary: assignments=" << num_assignments 
                  << " backtracks=" << num_backtracks 
                  << " levels=" << num_levels 
                  << " subcycles=" << num_subcycles_found << std::endl;
    }

    int find_root(int i) {
        while (parent[i] != i) i = parent[i];
        return i;
    }

    void notify_new_decision_level() override {
        num_levels++;
        current_level++;
        if (current_level >= (int)level_stack.size()) {
            level_stack.resize(current_level + 1);
        }
    }

    void notify_backtrack(size_t new_level) override {
        num_backtracks++;
        while (current_level > (int)new_level) {
            if (current_level < (int)level_stack.size()) {
                for (auto it = level_stack[current_level].rbegin(); it != level_stack[current_level].rend(); ++it) {
                    next_of[it->u] = it->prev_next_of;
                    if (it->p_node_u != 0) parent[it->p_node_u] = it->old_p_u;
                    if (it->p_node_v != 0) parent[it->p_node_v] = it->old_p_v;
                    if (it->r_node != 0) rank[it->r_node] = it->old_r;
                }
                level_stack[current_level].clear();
            }
            current_level--;
        }
        if (level_stack.size() > new_level + 1) {
            level_stack.resize(new_level + 1);
        }
        while (!pending_clauses.empty()) {
            pending_clauses.pop();
        }
        clause_idx = 0;
    }

    void notify_assignment(int lit, bool is_fixed) override {
        if (lit > 0) {
            int abs_lit = std::abs(lit);
            if (var_to_edge.count(abs_lit)) {
                num_assignments++;
                auto e = var_to_edge[abs_lit];
                int u = e.first;
                int v = e.second;

                if (current_level >= (int)level_stack.size()) {
                    level_stack.resize(current_level + 1);
                }

                int prev_next_of = next_of[u];
                int root_u = find_root(u);
                int root_v = find_root(v);

                AssignmentChange ac = {u, prev_next_of, 0, 0, 0, 0, 0, 0};

                if (root_u != root_v) {
                    if (rank[root_u] < rank[root_v]) {
                        ac.p_node_u = root_u;
                        ac.old_p_u = parent[root_u];
                        parent[root_u] = root_v;
                    } else if (rank[root_u] > rank[root_v]) {
                        ac.p_node_v = root_v;
                        ac.old_p_v = parent[root_v];
                        parent[root_v] = root_u;
                    } else {
                        ac.p_node_v = root_v;
                        ac.old_p_v = parent[root_v];
                        ac.r_node = root_u;
                        ac.old_r = rank[root_u];
                        parent[root_v] = root_u;
                        rank[root_u]++;
                    }
                }

                level_stack[current_level].push_back(ac);
                next_of[u] = v;

                if (root_u == root_v) {
                    // Cycle detected! Now trace it to build the exit clause
                    int curr = v;
                    int steps = 0;
                    std::vector<int> cycle_vertices;
                    cycle_vertices.push_back(u);

                    while (curr != 0 && steps <= N) {
                        cycle_vertices.push_back(curr);
                        if (curr == u) {
                            break;
                        }
                        curr = next_of[curr];
                        steps++;
                    }

                    if (curr == u && (int)cycle_vertices.size() - 1 < N) {
                        num_subcycles_found++;
                        int cycle_size = cycle_vertices.size() - 1;
                        int K = 0; while ((K + 1) * (K + 1) <= N) K++;
                        
                        bool use_exit_clause = true; // ALWAYS use exit clauses!
                        
                        if (use_exit_clause) {
                            std::vector<int> exit_clause_cw;
                            std::vector<bool> in_cycle(N + 1, false);
                            for (int i = 0; i < cycle_size; ++i) in_cycle[cycle_vertices[i]] = true;
                            
                            for (int i = 0; i < cycle_size; ++i) {
                                int su = cycle_vertices[i];
                                for (int nx : graph.adj_list[su]) {
                                    if (!in_cycle[nx]) {
                                        auto it = edge_to_var.find({su, nx});
                                        if (it != edge_to_var.end()) {
                                            exit_clause_cw.push_back(it->second);
                                        }
                                    }
                                }
                            }
                            if (!exit_clause_cw.empty()) {
                                pending_clauses.push(exit_clause_cw);
                            }
                        } else {
                            std::vector<int> clause_cw;
                            for (int i = 0; i < cycle_size; ++i) {
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
                                std::vector<int> clause_ccw;
                                for (int i = 0; i < cycle_size; ++i) {
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
        }
    }

    bool cb_check_found_model(const std::vector<int> &model) override {
        check_calls++;
        std::unordered_map<int, int> adj;
        for (int val : model) {
            int abs_val = std::abs(val);
            if (val > 0 && var_to_edge.count(abs_val)) {
                auto e = var_to_edge[abs_val];
                adj[e.first] = e.second;
            }
        }

        std::vector<std::vector<std::pair<int, int>>> cycles;
        std::vector<int> visited(N + 1, 0);
        for (int start = 1; start <= N; ++start) {
            if (visited[start] == 0 && adj.count(start)) {
                std::vector<int> path;
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
                    std::vector<std::pair<int, int>> cycle;
                    auto it = std::find(path.begin(), path.end(), curr);
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

        std::cerr << "[C++ Propagator] check " << check_calls << ": active_edges=" << adj.size() 
                  << " cycles_found=" << cycles.size() << std::endl;

        if (cycles.size() == 1 && cycles[0].size() == N) {
            return true;
        }

        if (num_subcycles_found % 10000 == 0) {
            std::cerr << "[Progress] Found " << num_subcycles_found << " subcycles..." << std::endl;
        }

        int added_clauses = 0;
        int K = 0; while ((K + 1) * (K + 1) <= N) K++;
        
        for (const auto& cycle : cycles) {
            int cycle_size = cycle.size();
            bool use_exit_clause = true;
            
            if (use_exit_clause) {
                std::vector<int> exit_clause_cw;
                std::vector<bool> in_cycle(N + 1, false);
                for (const auto& e : cycle) in_cycle[e.first] = true;
                
                for (const auto& e : cycle) {
                    int su = e.first;
                    for (int nx : graph.adj_list[su]) {
                        if (!in_cycle[nx]) {
                            auto it = edge_to_var.find({su, nx});
                            if (it != edge_to_var.end()) {
                                exit_clause_cw.push_back(it->second);
                            }
                        }
                    }
                }
                
                if (!exit_clause_cw.empty()) {
                    pending_clauses.push(exit_clause_cw);
                    added_clauses++;
                }
            } else {
                std::vector<int> clause_cw;
                for (const auto& e : cycle) {
                    auto it = edge_to_var.find({e.first, e.second});
                    if (it != edge_to_var.end()) {
                        clause_cw.push_back(-it->second);
                    }
                }
                if (!clause_cw.empty()) {
                    pending_clauses.push(clause_cw);
                    added_clauses++;
                }

                if (!is_directed) {
                    std::vector<int> clause_ccw;
                    for (const auto& e : cycle) {
                        auto it = edge_to_var.find({e.second, e.first});
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
        }

        if (added_clauses == 0) {
            std::vector<int> block_clause;
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

    bool cb_has_external_clause() override {
        return !pending_clauses.empty();
    }

    int cb_add_external_clause_lit() override {
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

SolverResult HcpSolver::solve(const Graph& graph, int timeout_seconds, int seed, int assume_u, int assume_v) {
    std::cout << "Running SAT solver..." << std::endl;
    SolverResult result;

    successor->build_clauses(&context, graph);

    CaDiCaL::Solver solver;
    if (seed != 0) {
        solver.set("seed", seed);
    }

    if (assume_u != 0 && assume_v != 0) {
        int assume_var = successor->getH(assume_u, assume_v);
        if (assume_var > 0) {
            solver.add(assume_var);
            solver.add(0);
        }
    }

    for (const auto& clause : context.cnf.clauses) {
        for (int lit : clause) {
            solver.add(lit);
        }
        solver.add(0);
    }

    result.nofClauses = context.total_clauses();
    result.nofVariables = context.total_vars();

    bool solver_finished = false;
    std::thread timer_thread([&solver, &solver_finished, timeout_seconds]() {
        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (solver_finished) break;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed >= timeout_seconds) {
                solver.terminate();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    auto start_solve_time = std::chrono::steady_clock::now();
    int sat_status = solver.solve();
    auto end_solve_time = std::chrono::steady_clock::now();

    solver_finished = true;
    if (timer_thread.joinable()) {
        timer_thread.join();
    }

    result.time = std::chrono::duration<double>(end_solve_time - start_solve_time).count();

    if (sat_status == 10) {
        result.status = "SAT";
        result.model.resize(context.total_vars());
        for (size_t i = 1; i <= context.total_vars(); ++i) {
            result.model[i - 1] = solver.val((int)i);
        }
    } else if (sat_status == 20) {
        result.status = "UNSAT";
    } else {
        result.status = "TIMEOUT";
        result.time = 9999.0;
    }

    return result;
}

void HcpSolver::print_result(const Graph& graph, const SolverResult& result) {
    int N = graph.v;
    if (result.model.empty()) {
        std::cout << "No solution found." << std::endl;
        return;
    }

    std::unordered_map<int, int> adj;
    for (int val : result.model) {
        int abs_val = std::abs(val);
        if (val > 0) {
            // Find if this variable corresponds to an edge
            for (int u = 1; u <= N; ++u) {
                if (u <= (int)graph.adj_list.size()) {
                    for (int w : graph.adj_list[u]) {
                        if (successor->getH(u, w) == abs_val) {
                            adj[u] = w;
                        }
                    }
                }
            }
        }
    }

    std::cout << "Hamiltonian cycle: ";
    int vertex = graph.start_vertex;
    int num = 1;
    while (true) {
        int orig_vertex = graph.orig_labels.empty() ? vertex : graph.orig_labels[vertex];
        std::cout << orig_vertex << " -> ";
        if (adj.count(vertex)) {
            int next_v = adj[vertex];
            std::vector<int> internal = graph.expand_edge(vertex, next_v);
            for (int internal_node : internal) {
                std::cout << internal_node << " -> ";
                num++;
            }
            vertex = next_v;
        } else {
            std::cout << "BROKEN" << std::endl;
            break;
        }
        if (vertex == graph.start_vertex) {
            int orig_start = graph.orig_labels.empty() ? graph.start_vertex : graph.orig_labels[graph.start_vertex];
            std::cout << orig_start << std::endl;
            break;
        }
        num++;
    }
    std::cout << "Vertices of HC: " << num << std::endl;
}

std::vector<std::vector<std::pair<int, int>>> IncHcpSolver::find_all_cycles(const Graph& graph, const std::vector<int>& model) {
    int N = graph.v;
    std::unordered_map<int, int> adj;
    for (int val : model) {
        int abs_val = std::abs(val);
        if (val > 0) {
            for (int u = 1; u <= N; ++u) {
                if (u <= (int)graph.adj_list.size()) {
                    for (int w : graph.adj_list[u]) {
                        if (successor->getH(u, w) == abs_val) {
                            adj[u] = w;
                        }
                    }
                }
            }
        }
    }

    std::vector<std::vector<std::pair<int, int>>> cycles;
    std::vector<int> visited(N + 1, 0);

    for (int start = 1; start <= N; ++start) {
        if (visited[start] == 0 && adj.count(start)) {
            std::vector<int> path;
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
                std::vector<std::pair<int, int>> cycle;
                auto it = std::find(path.begin(), path.end(), curr);
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

    return cycles;
}

SolverResult IncHcpSolver::solve_incremental(const Graph& graph, int solver_mode, int timeout_seconds, int seed, int assume_u, int assume_v) {
    std::cout << "Running Incremental SAT solver..." << std::endl;
    SolverResult result;

    successor->build_clauses(&context, graph);

    CaDiCaL::Solver solver;
    if (seed != 0) {
        solver.set("seed", seed);
    }

    for (const auto& clause : context.cnf.clauses) {
        for (int lit : clause) {
            solver.add(lit);
        }
        solver.add(0);
    }

    std::unordered_map<int, std::pair<int, int>> var_to_edge;
    std::map<std::pair<int, int>, int> edge_to_var;
    for (int i = 1; i <= graph.v; ++i) {
        if (i <= (int)graph.adj_list.size()) {
            for (int j : graph.adj_list[i]) {
                int var_id = successor->getH(i, j);
                if (var_id > 0) {
                    var_to_edge[var_id] = {i, j};
                    edge_to_var[{i, j}] = var_id;
                }
            }
        }
    }

    std::atomic<bool> solver_finished(false);
    std::thread timer_thread([&solver, &solver_finished, timeout_seconds]() {
        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (solver_finished) break;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsed >= timeout_seconds) {
                solver.terminate();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    auto start_time = std::chrono::steady_clock::now();

    if (assume_u != 0 && assume_v != 0) {
        int assume_var = successor->getH(assume_u, assume_v);
        if (assume_var > 0) {
            solver.add(assume_var);
            solver.add(0);
        }
    }

    if (solver_mode == 0) {
        // Propagator mode
        std::cerr << "Running C++ External Propagator..." << std::endl;
        HCPPropagator* propagator = new HCPPropagator(graph.v, graph.is_directed, opt_level, graph);
        propagator->var_to_edge = var_to_edge;
        propagator->edge_to_var = edge_to_var;
        solver.connect_external_propagator(propagator);
        for (const auto& pair : var_to_edge) {
            solver.add_observed_var(pair.first);
        }

        int sat_status = solver.solve();
        auto end_time = std::chrono::steady_clock::now();
        result.time = std::chrono::duration<double>(end_time - start_time).count();

        if (sat_status == 10) {
            result.status = "SAT";
            result.model.resize(context.total_vars());
            for (size_t i = 1; i <= context.total_vars(); ++i) {
                result.model[i - 1] = solver.val((int)i);
            }
        } else if (sat_status == 20) {
            result.status = "UNSAT";
        } else {
            result.status = "TIMEOUT";
        }

        delete propagator;
    } else {
        // CEGAR mode
        std::cerr << "Running C++ Stateful CEGAR Loop..." << std::endl;
        int cegar_calls = 0;
        while (true) {
            cegar_calls++;
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start_time).count();
            if (elapsed >= timeout_seconds) {
                result.status = "TIMEOUT";
                result.time = elapsed;
                break;
            }

            int sat_status = solver.solve();
            now = std::chrono::steady_clock::now();
            elapsed = std::chrono::duration<double>(now - start_time).count();

            if (sat_status == 0) {
                result.status = "TIMEOUT";
                result.time = elapsed;
                break;
            } else if (sat_status == 20) {
                result.status = "UNSAT";
                result.time = elapsed;
                break;
            }

            std::vector<int> model(context.total_vars());
            for (size_t i = 1; i <= context.total_vars(); ++i) {
                model[i - 1] = solver.val((int)i);
            }

            auto subcycles = find_all_cycles(graph, model);
            std::cerr << "[C++ CEGAR] check " << cegar_calls << ": cycles_found=" << subcycles.size() << std::endl;

            if (subcycles.size() == 1 && (int)subcycles[0].size() == graph.v) {
                result.status = "SAT";
                result.model = model;
                result.time = elapsed;
                break;
            }

            int added = 0;
            int K = 0; while ((K + 1) * (K + 1) <= graph.v) K++;
            for (const auto& cycle : subcycles) {
                int cycle_size = cycle.size();
                bool use_exit_clause = true;
                
                if (use_exit_clause) {
                    std::vector<int> exit_clause_cw;
                    std::vector<bool> in_cycle(graph.v + 1, false);
                    for (const auto& edge : cycle) in_cycle[edge.first] = true;
                    
                    for (const auto& edge : cycle) {
                        int su = edge.first;
                        for (int nx : graph.adj_list[su]) {
                            if (!in_cycle[nx]) {
                                int var_id = edge_to_var[{su, nx}];
                                if (var_id > 0) {
                                    solver.add(var_id);
                                    exit_clause_cw.push_back(var_id);
                                }
                            }
                        }
                    }
                    if (!exit_clause_cw.empty()) {
                        solver.add(0);
                        added++;
                    }
                } else {
                    for (const auto& edge : cycle) {
                        int var_id = edge_to_var[{edge.first, edge.second}];
                        solver.add(-var_id);
                    }
                    solver.add(0);
                    added++;

                    if (!graph.is_directed) {
                        for (const auto& edge : cycle) {
                            int var_id = edge_to_var[{edge.second, edge.first}];
                            solver.add(-var_id);
                        }
                        solver.add(0);
                        added++;
                    }
                }
            }

            if (added == 0) {
                // Trivial block clause to avoid loop
                for (const auto& pair : var_to_edge) {
                    if (solver.val(pair.first) > 0) {
                        solver.add(-pair.first);
                    }
                }
                solver.add(0);
            }
        }
    }

    solver_finished = true;
    if (timer_thread.joinable()) {
        timer_thread.join();
    }

    result.nofClauses = context.total_clauses();
    result.nofVariables = context.total_vars();
    return result;
}
