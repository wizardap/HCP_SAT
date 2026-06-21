#include <iostream>
#include <vector>

int main() {
    int N = 5;
    std::vector<int> head(N+1), tail(N+1), next_of(N+1, 0);
    for(int i=1; i<=N; ++i) { head[i] = i; tail[i] = i; }
    
    auto add_edge = [&](int u, int v) {
        int H_u = head[u];
        int T_v = tail[v];
        if (head[u] == head[v]) {
            std::cout << "Cycle detected adding " << u << "->" << v << "\n";
        }
        next_of[u] = v;
        tail[H_u] = T_v;
        head[T_v] = H_u;
    };
    
    add_edge(1, 2);
    add_edge(2, 3);
    add_edge(4, 5);
    add_edge(3, 4); // now 1->2->3->4->5
    add_edge(5, 2); // 2 is intermediate!
    
    return 0;
}
