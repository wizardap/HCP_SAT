#include <iostream>
#include <vector>

int main() {
    int N = 5;
    std::vector<int> parent(N+1), rank(N+1, 0);
    for(int i=1; i<=N; ++i) parent[i] = i;
    
    auto find_root = [&](int i) {
        while (parent[i] != i) i = parent[i];
        return i;
    };
    
    auto add_edge = [&](int u, int v) {
        int root_u = find_root(u);
        int root_v = find_root(v);
        if (root_u == root_v) {
            std::cout << "Cycle detected adding " << u << "->" << v << "\n";
            return; // Don't union
        }
        if (rank[root_u] < rank[root_v]) {
            parent[root_u] = root_v;
        } else if (rank[root_u] > rank[root_v]) {
            parent[root_v] = root_u;
        } else {
            parent[root_v] = root_u;
            rank[root_u]++;
        }
    };
    
    add_edge(2, 3);
    add_edge(3, 4);
    add_edge(1, 2);
    add_edge(4, 3); // 4->3 inside the same set!
    
    return 0;
}
