#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <map>
#include <utility>

struct Graph {
    int v = 0;
    int e = 0;
    int start_vertex = 1;
    bool is_directed = false;
    std::vector<std::vector<int>> adj_list; // 1-indexed adjacency list
    std::string filename = "";

    void load_graph_from_file(const std::string &url) {
        filename = url;
        std::string norm_name = url;
        std::replace(norm_name.begin(), norm_name.end(), '\\', '/');
        size_t last_slash = norm_name.find_last_of('/');
        std::string test_set = "";
        if (last_slash != std::string::npos) {
            size_t second_last = norm_name.find_last_of('/', last_slash - 1);
            if (second_last != std::string::npos) {
                test_set = norm_name.substr(second_last + 1, last_slash - second_last - 1);
            }
        }

        std::ifstream infile(url);
        if (!infile.is_open()) {
            std::cerr << "Error opening file: " << url << std::endl;
            exit(1);
        }

        adj_list.clear();
        v = 0;
        e = 0;

        std::string line;
        if (test_set == "vset" || test_set == "graphs") {
            is_directed = (test_set == "vset");
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                if (line[0] == 'p') {
                    std::stringstream ss(line);
                    std::string p, cnf;
                    int n;
                    ss >> p >> cnf >> n;
                    v = n;
                    adj_list.resize(v + 1);
                } else if (line[0] == 'e') {
                    std::stringstream ss(line);
                    char e_char;
                    int u, w;
                    ss >> e_char >> u >> w;
                    if (std::find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (!is_directed) {
                        if (std::find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                            adj_list[w].push_back(u);
                        }
                    }
                } else if (isdigit(line[0])) {
                    std::stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (std::find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (!is_directed) {
                        if (std::find(adj_list[w].begin(), adj_list[w].end(), u) == std::find(adj_list[w].begin(), adj_list[w].end(), u)) {
                            if (std::find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
                                adj_list[w].push_back(u);
                            }
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
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = std::stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    std::stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (std::find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                        adj_list[u].push_back(w);
                    }
                    if (std::find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
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
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = std::stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    std::stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (u != -1 && w != -1) {
                        if (std::find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
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
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                if (line.rfind("DIMENSION", 0) == 0) {
                    size_t colon = line.find(':');
                    v = std::stoi(line.substr(colon + 1));
                    adj_list.resize(v + 1);
                } else if (isdigit(line[0])) {
                    std::stringstream ss(line);
                    int u, w;
                    ss >> u >> w;
                    if (u > 0 && w > 0) {
                        if (v == 0) {
                            v = std::max(v, std::max(u, w));
                            adj_list.resize(v + 1);
                        }
                        if (std::find(adj_list[u].begin(), adj_list[u].end(), w) == adj_list[u].end()) {
                            adj_list[u].push_back(w);
                        }
                        if (!is_directed) {
                            if (std::find(adj_list[w].begin(), adj_list[w].end(), u) == adj_list[w].end()) {
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

    std::vector<int> orig_labels; // maps new_label -> old_label (1-indexed)
    std::map<std::pair<int, int>, std::vector<int>> edge_expansion; // old_label -> old_label

    void preprocess() {
        if (is_directed) return; // Preprocessing is only implemented for undirected graphs

        bool changed = true;
        std::vector<bool> active(v + 1, true);
        
        while (changed) {
            changed = false;
            for (int i = 1; i <= v; ++i) {
                if (!active[i]) continue;
                
                std::vector<int> neighbors;
                for (int nx : adj_list[i]) {
                    if (active[nx]) neighbors.push_back(nx);
                }
                
                if (neighbors.size() == 2) {
                    int a = neighbors[0];
                    int b = neighbors[1];
                    
                    if (a == b) continue;
                    
                    // Contract i
                    active[i] = false;
                    changed = true;
                    
                    std::vector<int> internal_path;
                    if (edge_expansion.count({a, i})) {
                        auto p = edge_expansion[{a, i}];
                        internal_path.insert(internal_path.end(), p.begin(), p.end());
                    }
                    internal_path.push_back(i);
                    if (edge_expansion.count({i, b})) {
                        auto p = edge_expansion[{i, b}];
                        internal_path.insert(internal_path.end(), p.begin(), p.end());
                    }
                    
                    // Remove i from a and b's adj lists
                    adj_list[a].erase(std::remove(adj_list[a].begin(), adj_list[a].end(), i), adj_list[a].end());
                    adj_list[b].erase(std::remove(adj_list[b].begin(), adj_list[b].end(), i), adj_list[b].end());
                    
                    // Remove existing a-b edge (if any) since the path MUST go through i
                    adj_list[a].erase(std::remove(adj_list[a].begin(), adj_list[a].end(), b), adj_list[a].end());
                    adj_list[b].erase(std::remove(adj_list[b].begin(), adj_list[b].end(), a), adj_list[b].end());
                    
                    // Add new a-b edge
                    adj_list[a].push_back(b);
                    adj_list[b].push_back(a);
                    
                    edge_expansion[{a, b}] = internal_path;
                    
                    std::vector<int> rev_path = internal_path;
                    std::reverse(rev_path.begin(), rev_path.end());
                    edge_expansion[{b, a}] = rev_path;
                }
            }
        }
        
        int new_v = 0;
        orig_labels.resize(1); // 1-indexed, orig_labels[0] is unused
        std::vector<int> old_to_new(v + 1, 0);
        
        for (int i = 1; i <= v; ++i) {
            if (active[i]) {
                new_v++;
                orig_labels.push_back(i);
                old_to_new[i] = new_v;
            }
        }
        
        std::vector<std::vector<int>> new_adj_list(new_v + 1);
        int new_e = 0;
        for (int i = 1; i <= v; ++i) {
            if (active[i]) {
                int u = old_to_new[i];
                for (int nx : adj_list[i]) {
                    if (active[nx]) {
                        new_adj_list[u].push_back(old_to_new[nx]);
                        new_e++;
                    }
                }
            }
        }
        
        v = new_v;
        e = new_e / 2;
        adj_list = std::move(new_adj_list);
        
        int min_deg = 1e9;
        start_vertex = 1;
        for (int i = 1; i <= v; ++i) {
            if ((int)adj_list[i].size() < min_deg) {
                min_deg = adj_list[i].size();
                start_vertex = i;
            }
        }
    }

    std::vector<int> expand_edge(int u, int w) const {
        if (orig_labels.empty()) return {};
        int old_u = orig_labels[u];
        int old_w = orig_labels[w];
        if (edge_expansion.count({old_u, old_w})) {
            return edge_expansion.at({old_u, old_w});
        }
        return {};
    }
};
