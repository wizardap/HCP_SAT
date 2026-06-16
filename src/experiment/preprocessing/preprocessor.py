import time
from dataclasses import dataclass, field
from libs.utils.graph import Graph

@dataclass
class PreprocessResult:
    graph: Graph
    forced_edges: set = field(default_factory=set)      # set of (u, v)
    forbidden_edges: set = field(default_factory=set)   # set of (u, v)
    contracted_paths: dict = field(default_factory=dict) # (u, v) -> [u, v1, ..., v]
    original_to_reduced: dict = field(default_factory=dict)
    reduced_to_original: dict = field(default_factory=dict)
    is_unsat: bool = False
    stats: dict = field(default_factory=dict)


class HCPPreprocessor:
    def __init__(self, enable_probing=True, max_probing_edges=300, enable_contraction=True, enable_2cut=True):
        self.enable_probing = enable_probing
        self.max_probing_edges = max_probing_edges
        self.enable_contraction = enable_contraction
        self.enable_2cut = enable_2cut

    def preprocess(self, original_graph: Graph) -> PreprocessResult:
        start_time = time.time()
        n = original_graph.v
        is_directed = original_graph.is_directed
        
        # Build local representation of active adjacency list
        # Undirected: adj[u] = set of neighbors
        # Directed: adj_out[u] = set, adj_in[u] = set
        adj = {i: set(original_graph.graph[i]) for i in range(1, n + 1)}
        
        adj_in = None
        if is_directed:
            adj_in = {i: set() for i in range(1, n + 1)}
            for u in range(1, n + 1):
                for v in adj[u]:
                    adj_in[v].add(u)
                    
        forced = set()
        forbidden = set()
        
        # Helper: Normalize edge representation
        def norm_edge(u, v):
            return (u, v) if is_directed else (min(u, v), max(u, v))

        # 1. Base Connectivity Check
        if is_directed:
            if not self._is_strongly_connected(n, adj, forbidden):
                return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})
        else:
            if not self._is_biconnected(n, adj, forbidden):
                return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})

        # 2. Degree Cascade & Bridge / 2-Edge-Cut Loop (Fixpoint)
        changed = True
        while changed:
            changed = False
            
            # Run Degree Cascade
            if is_directed:
                cascade_ok = self._cascade_directed(n, adj, adj_in, forced, forbidden)
            else:
                cascade_ok = self._cascade_undirected(n, adj, forced, forbidden)
                
            if not cascade_ok:
                return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})
                
            # Bridge Detection (undirected only)
            if not is_directed:
                bridges = self._find_bridges(n, adj, forbidden)
                for b in bridges:
                    if b not in forced:
                        # In HC, a bridge is UNSAT since we cannot traverse back
                        return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})
                        
                # 2-Edge-Cut Detection (if graph is small enough, e.g. V < 300)
                if self.enable_2cut and n < 300:
                    cuts = self._find_2_edge_cuts(n, adj, forced, forbidden)
                    if cuts:
                        for u, v in cuts:
                            edge = norm_edge(u, v)
                            if edge not in forced:
                                forced.add(edge)
                                changed = True

        # 3. Graph Probing (if enabled and within edge budget)
        active_edges = []
        for u in range(1, n + 1):
            for v in adj[u]:
                if is_directed or u < v:
                    edge = norm_edge(u, v)
                    if edge not in forced and edge not in forbidden:
                        active_edges.append(edge)
                        
        if self.enable_probing and len(active_edges) <= self.max_probing_edges:
            probe_changed = True
            while probe_changed:
                probe_changed = False
                for edge in list(active_edges):
                    if edge in forced or edge in forbidden:
                        continue
                        
                    # Probe negative: assume edge is forbidden
                    trial_forbidden = forbidden.copy() | {edge}
                    trial_forced = forced.copy()
                    if is_directed:
                        ok = self._cascade_directed(n, adj, adj_in, trial_forced, trial_forbidden)
                    else:
                        ok = self._cascade_undirected(n, adj, trial_forced, trial_forbidden)
                        if ok:
                            # Additional check: does it disconnect?
                            ok = self._is_biconnected(n, adj, trial_forbidden)
                            
                    if not ok:
                        # Forbidding this edge leads to UNSAT -> edge MUST be forced
                        forced.add(edge)
                        probe_changed = True
                        changed = True
                        break
                        
                    # Probe positive: assume edge is forced
                    trial_forced = forced.copy() | {edge}
                    trial_forbidden = forbidden.copy()
                    if is_directed:
                        ok = self._cascade_directed(n, adj, adj_in, trial_forced, trial_forbidden)
                    else:
                        ok = self._cascade_undirected(n, adj, trial_forced, trial_forbidden)
                        if ok:
                            ok = self._is_biconnected(n, adj, trial_forbidden)
                            
                    if not ok:
                        # Forcing this edge leads to UNSAT -> edge MUST be forbidden
                        forbidden.add(edge)
                        probe_changed = True
                        changed = True
                        break
                
                # Re-run main cascade if probing found new forced/forbidden edges
                if probe_changed:
                    if is_directed:
                        cascade_ok = self._cascade_directed(n, adj, adj_in, forced, forbidden)
                    else:
                        cascade_ok = self._cascade_undirected(n, adj, forced, forbidden)
                    if not cascade_ok:
                        return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})

        # 4. Path Contraction
        contracted_paths = {}
        removed_vertices = set()
        
        # Build forced adj list
        forced_adj = {i: set() for i in range(1, n + 1)}
        for u, v in forced:
            forced_adj[u].add(v)
            if not is_directed:
                forced_adj[v].add(u)
                
        if self.enable_contraction:
            # Find chains
            visited = set()
            for start in range(1, n + 1):
                if start in visited or start in removed_vertices:
                    continue
                    
                # We look for a vertex with forced degree == 1 or 2 (to trace a path)
                # A vertex is a good endpoint for a path contraction if it has degree > 2 (or in-deg/out-deg > 1)
                # but connects to a degree 2 vertex.
                active_neighbors = {v for v in adj[start] if norm_edge(start, v) not in forbidden}
                if len(active_neighbors) <= 2:
                    # This vertex is part of a chain, we will visit it when we start from an endpoint,
                    # or it's a isolated cycle (which we will handle)
                    continue
                    
                # For each neighbor of start, if it has forced degree == 2 (meaning it is a middle node of a path)
                for neigh in list(forced_adj[start]):
                    if neigh in visited or neigh in removed_vertices:
                        continue
                    neigh_active = {v for v in adj[neigh] if norm_edge(neigh, v) not in forbidden}
                    if len(neigh_active) == 2:
                        # Trace path starting: start -> neigh -> ...
                        path = [start, neigh]
                        visited.add(neigh)
                        curr = neigh
                        prev = start
                        
                        while True:
                            # Find next node in forced path
                            next_nodes = [v for v in forced_adj[curr] if v != prev]
                            if not next_nodes:
                                break
                            next_node = next_nodes[0]
                            next_active = {v for v in adj[next_node] if norm_edge(next_node, v) not in forbidden}
                            
                            if len(next_active) == 2:
                                path.append(next_node)
                                visited.add(next_node)
                                prev = curr
                                curr = next_node
                            else:
                                path.append(next_node)
                                break
                                
                        if len(path) >= 3:
                            # Contract path: path[0] -> path[-1]
                            u, v = path[0], path[-1]
                            contracted_paths[norm_edge(u, v)] = path
                            for w in path[1:-1]:
                                removed_vertices.add(w)

        # Check for sub-cycle in forced edges
        # If any forced path forms a cycle of size < n, then it's UNSAT
        # Let's trace forced components to check for cycles
        forced_visited = set()
        for i in range(1, n + 1):
            if i in forced_visited or i in removed_vertices:
                continue
            if len(forced_adj[i]) > 0:
                # Trace component
                comp = []
                curr = i
                prev = None
                is_cycle = False
                while curr not in forced_visited:
                    forced_visited.add(curr)
                    comp.append(curr)
                    next_nodes = [v for v in forced_adj[curr] if v != prev]
                    if not next_nodes:
                        break
                    next_node = next_nodes[0]
                    if next_node == i:
                        is_cycle = True
                        break
                    prev = curr
                    curr = next_node
                if is_cycle and len(comp) < n:
                    return PreprocessResult(graph=original_graph, is_unsat=True, stats={"time": time.time() - start_time})

        # 5. Reindex remaining vertices
        remaining_vertices = sorted(list(set(range(1, n + 1)) - removed_vertices))
        original_to_reduced = {v: idx + 1 for idx, v in enumerate(remaining_vertices)}
        reduced_to_original = {idx + 1: v for idx, v in enumerate(remaining_vertices)}
        
        # Build reduced graph
        reduced_graph = Graph()
        reduced_graph.set_is_directed(is_directed)
        reduced_graph.v = len(remaining_vertices)
        
        reduced_adj = {original_to_reduced[v]: [] for v in remaining_vertices}
        for u in remaining_vertices:
            u_red = original_to_reduced[u]
            for v in adj[u]:
                edge = norm_edge(u, v)
                if edge not in forbidden and v in original_to_reduced:
                    v_red = original_to_reduced[v]
                    reduced_adj[u_red].append(v_red)
                    
        # Add contracted edges as forced edges in the reduced graph
        reduced_forced = set()
        reduced_forbidden = set()
        
        for edge in forced:
            u, v = edge
            if u in original_to_reduced and v in original_to_reduced:
                reduced_forced.add(norm_edge(original_to_reduced[u], original_to_reduced[v]))
                
        for edge in forbidden:
            u, v = edge
            if u in original_to_reduced and v in original_to_reduced:
                reduced_forbidden.add(norm_edge(original_to_reduced[u], original_to_reduced[v]))
                
        for edge in contracted_paths:
            u, v = edge
            u_red, v_red = original_to_reduced[u], original_to_reduced[v]
            reduced_forced.add(norm_edge(u_red, v_red))
            # Make sure this contracted edge exists in the reduced graph
            if v_red not in reduced_adj[u_red]:
                reduced_adj[u_red].append(v_red)
            if not is_directed and u_red not in reduced_adj[v_red]:
                reduced_adj[v_red].append(u_red)

        reduced_graph.set_graph(reduced_adj)
        
        # Adjust start vertex
        start_v = original_graph.start_vertex
        if start_v in original_to_reduced:
            reduced_graph.set_start_vertex(original_to_reduced[start_v])
        else:
            # start_v was removed/contracted. Choose start of the path it was on.
            found = False
            for edge, path in contracted_paths.items():
                if start_v in path:
                    reduced_graph.set_start_vertex(original_to_reduced[path[0]])
                    found = True
                    break
            if not found:
                reduced_graph.set_start_vertex(1)

        # Map back forced/forbidden edges for validation/reference
        mapped_forced = {norm_edge(reduced_to_original[u], reduced_to_original[v]) for u, v in reduced_forced}
        mapped_forbidden = {norm_edge(reduced_to_original[u], reduced_to_original[v]) for u, v in reduced_forbidden}

        # Build stats
        elapsed = time.time() - start_time
        stats = {
            "time": elapsed,
            "orig_v": n,
            "reduced_v": reduced_graph.v,
            "forced_count": len(forced),
            "forbidden_count": len(forbidden),
            "contracted_count": len(contracted_paths),
        }
        
        return PreprocessResult(
            graph=reduced_graph,
            forced_edges=forced,
            forbidden_edges=forbidden,
            contracted_paths=contracted_paths,
            original_to_reduced=original_to_reduced,
            reduced_to_original=reduced_to_original,
            is_unsat=False,
            stats=stats
        )

    # --- HELPER ALGORITHMS ---

    def _cascade_undirected(self, n, adj, forced, forbidden) -> bool:
        changed = True
        while changed:
            changed = False
            for u in range(1, n + 1):
                active = [v for v in adj[u] if (min(u, v), max(u, v)) not in forbidden]
                forced_inc = [v for v in active if (min(u, v), max(u, v)) in forced]
                
                if len(active) < 2:
                    return False  # UNSAT
                    
                if len(forced_inc) > 2:
                    return False  # UNSAT
                    
                if len(forced_inc) == 2:
                    # forbid all other active edges
                    for v in active:
                        if v not in forced_inc:
                            edge = (min(u, v), max(u, v))
                            if edge not in forbidden:
                                forbidden.add(edge)
                                changed = True
                                
                if len(active) == 2:
                    # force both active edges
                    for v in active:
                        edge = (min(u, v), max(u, v))
                        if edge not in forced:
                            forced.add(edge)
                            changed = True
        return True

    def _cascade_directed(self, n, adj_out, adj_in, forced, forbidden) -> bool:
        changed = True
        while changed:
            changed = False
            for u in range(1, n + 1):
                active_out = [v for v in adj_out[u] if (u, v) not in forbidden]
                active_in = [v for v in adj_in[u] if (v, u) not in forbidden]
                
                forced_out = [v for v in active_out if (u, v) in forced]
                forced_in = [v for v in active_in if (v, u) in forced]
                
                if len(active_out) < 1 or len(active_in) < 1:
                    return False
                    
                if len(forced_out) > 1 or len(forced_in) > 1:
                    return False
                    
                if len(forced_out) == 1:
                    v = forced_out[0]
                    for w in active_out:
                        if w != v:
                            edge = (u, w)
                            if edge not in forbidden:
                                forbidden.add(edge)
                                changed = True
                                
                if len(forced_in) == 1:
                    v = forced_in[0]
                    for w in active_in:
                        if w != v:
                            edge = (w, u)
                            if edge not in forbidden:
                                forbidden.add(edge)
                                changed = True
                                
                if len(active_out) == 1:
                    v = active_out[0]
                    edge = (u, v)
                    if edge not in forced:
                        forced.add(edge)
                        changed = True
                        
                if len(active_in) == 1:
                    v = active_in[0]
                    edge = (v, u)
                    if edge not in forced:
                        forced.add(edge)
                        changed = True
        return True

    def _is_biconnected(self, n, adj, forbidden) -> bool:
        visited = [False] * (n + 1)
        depth = [0] * (n + 1)
        low = [0] * (n + 1)
        parent = [0] * (n + 1)
        is_art = [False] * (n + 1)
        
        start_node = None
        for i in range(1, n + 1):
            active_neigh = [v for v in adj[i] if (min(i, v), max(i, v)) not in forbidden]
            if active_neigh:
                start_node = i
                break
                
        if start_node is None:
            return False
            
        dfs_count = 0
        root_children = 0
        
        def dfs(u, d):
            nonlocal dfs_count, root_children
            visited[u] = True
            dfs_count += 1
            depth[u] = d
            low[u] = d
            
            for v in adj[u]:
                if (min(u, v), max(u, v)) in forbidden:
                    continue
                if not visited[v]:
                    parent[v] = u
                    if u == start_node:
                        root_children += 1
                    dfs(v, d + 1)
                    low[u] = min(low[u], low[v])
                    if u != start_node and low[v] >= depth[u]:
                        is_art[u] = True
                elif v != parent[u]:
                    low[u] = min(low[u], depth[v])

        dfs(start_node, 1)
        
        # Check if connected
        for i in range(1, n + 1):
            active_neigh = [v for v in adj[i] if (min(i, v), max(i, v)) not in forbidden]
            if active_neigh and not visited[i]:
                return False
                
        if root_children > 1:
            is_art[start_node] = True
            
        return not any(is_art)

    def _is_strongly_connected(self, n, adj_out, forbidden) -> bool:
        index = [0] * (n + 1)
        lowlink = [0] * (n + 1)
        on_stack = [False] * (n + 1)
        stack = []
        current_index = 1
        scc_count = 0
        
        def strongconnect(u):
            nonlocal current_index, scc_count
            index[u] = current_index
            lowlink[u] = current_index
            current_index += 1
            stack.append(u)
            on_stack[u] = True
            
            for v in adj_out[u]:
                if (u, v) in forbidden:
                    continue
                if index[v] == 0:
                    strongconnect(v)
                    lowlink[u] = min(lowlink[u], lowlink[v])
                elif on_stack[v]:
                    lowlink[u] = min(lowlink[u], index[v])
                    
            if lowlink[u] == index[u]:
                while True:
                    w = stack.pop()
                    on_stack[w] = False
                    if w == u:
                        break
                scc_count += 1
                
        # Start Tarjan from first vertex
        for i in range(1, n + 1):
            # check if it has active edges
            active_neigh = [v for v in adj_out[i] if (i, v) not in forbidden]
            if active_neigh and index[i] == 0:
                strongconnect(i)
                
        return scc_count == 1

    def _find_bridges(self, n, adj, forbidden) -> list:
        visited = [False] * (n + 1)
        depth = [0] * (n + 1)
        low = [0] * (n + 1)
        parent = [0] * (n + 1)
        bridges = []
        
        start_node = None
        for i in range(1, n + 1):
            active_neigh = [v for v in adj[i] if (min(i, v), max(i, v)) not in forbidden]
            if active_neigh:
                start_node = i
                break
                
        if start_node is None:
            return []
            
        def dfs(u, d):
            visited[u] = True
            depth[u] = d
            low[u] = d
            for v in adj[u]:
                if (min(u, v), max(u, v)) in forbidden:
                    continue
                if not visited[v]:
                    parent[v] = u
                    dfs(v, d + 1)
                    low[u] = min(low[u], low[v])
                    if low[v] > depth[u]:
                        bridges.append((min(u, v), max(u, v)))
                elif v != parent[u]:
                    low[u] = min(low[u], depth[v])
                    
        dfs(start_node, 1)
        return bridges

    def _find_2_edge_cuts(self, n, adj, forced, forbidden) -> list:
        # Find all active edges
        active_edges = []
        for u in range(1, n + 1):
            for v in adj[u]:
                if u < v and (u, v) not in forbidden:
                    active_edges.append((u, v))
                    
        cuts = []
        # Try removing each edge and check for new bridges
        for edge in active_edges:
            trial_forbidden = forbidden | {edge}
            bridges = self._find_bridges(n, adj, trial_forbidden)
            for b in bridges:
                if b != edge:
                    cuts.append(edge)
                    cuts.append(b)
        return list(set(cuts))
