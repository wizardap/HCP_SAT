#pragma once
#include <vector>
#include <string>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <queue>
#include <unordered_map>

namespace fs = std::filesystem;

inline std::vector<int> extract_numbers(const std::string& filename) {
    std::vector<int> numbers;
    std::regex re("\\d+");
    auto words_begin = std::sregex_iterator(filename.begin(), filename.end(), re);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        numbers.push_back(std::stoi(i->str()));
    }
    return numbers;
}

inline std::vector<std::string> get_files_from_folder(const std::string& folder_path) {
    std::vector<std::string> files;
    if (!fs::exists(folder_path)) return files;
    
    if (fs::is_regular_file(folder_path)) {
        std::string p = fs::path(folder_path).string();
        std::replace(p.begin(), p.end(), '\\', '/');
        files.push_back(p);
        return files;
    }

    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (entry.is_regular_file()) {
            std::string p = entry.path().string();
            std::replace(p.begin(), p.end(), '\\', '/');
            files.push_back(p);
        }
    }

    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        auto num_a = extract_numbers(fs::path(a).filename().string());
        auto num_b = extract_numbers(fs::path(b).filename().string());
        return num_a < num_b;
    });

    return files;
}

inline std::unordered_map<int, int> find_distance(const std::vector<std::vector<int>>& graph, int start_vertex, int n) {
    std::unordered_map<int, int> visited;
    visited[start_vertex] = 0;
    std::queue<int> q;
    q.push(start_vertex);
    while (!q.empty()) {
        int current_vertex = q.front();
        q.pop();
        int current_distance = visited[current_vertex];
        if (current_vertex < (int)graph.size()) {
            for (int neighbor : graph[current_vertex]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited[neighbor] = current_distance + 1;
                    q.push(neighbor);
                }
            }
        }
    }
    return visited;
}

inline std::unordered_map<int, int> find_distance_to_first(const std::vector<std::vector<int>>& graph, int start_vertex, int n) {
    // Reverse the graph
    std::vector<std::vector<int>> reversed_graph(n + 1);
    for (int v = 1; v <= n; ++v) {
        if (v < (int)graph.size()) {
            for (int u : graph[v]) {
                if (u <= n) {
                    reversed_graph[u].push_back(v);
                }
            }
        }
    }
    return find_distance(reversed_graph, start_vertex, n);
}
