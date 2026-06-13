from libs.models.successor.base import SuccessorMethod
from libs.utils.context import EncodingContext

class Vee(SuccessorMethod):
    def __init__(self, encoder=None):
        super().__init__(encoder)
        self.H = {}  # mapping: (u, w) -> var_id (currently active variable)
        self.original_H = {}  # mapping: (u, w) -> var_id (original step 0 variable)
        self.V = {}  # mapping: v -> var_id

    def getH(self, u, w):
        if (u, w) not in self.original_H:
            self.original_H[(u, w)] = self.new_var()
        return self.original_H[(u, w)]

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        
        self.original_H = {}
        self.H = {}

        # 1. Initialize variables for vertices and original edges
        for v in range(1, n + 1):
            self.V[v] = self.new_var()
            self.add_clause([self.V[v]])

        original_edges = set()
        for u in range(1, n + 1):
            for w in graph.graph[u]:
                self.original_H[(u, w)] = self.new_var()
                original_edges.add((u, w))

        # Copy original variables to active variables mapping
        self.H = self.original_H.copy()

        # Handle trivial cases
        if n < 3:
            self.add_clause([])
            return self.context

        # Set up active vertices and active edges
        active_vertices = set(range(1, n + 1))
        active_edges = {u: set(graph.graph[u]) for u in range(1, n + 1)}

        # 2. Sequential Elimination Loop
        for step in range(1, n - 2):
            # Find in-neighbors for all active vertices to compute degree
            in_adj = {u: set() for u in active_vertices}
            for u in active_vertices:
                for w in active_edges[u]:
                    if w in active_vertices:
                        in_adj[w].add(u)

            # Find vertex v with minimum degree (out-degree + in-degree)
            v = None
            min_deg = float('inf')
            for u in active_vertices:
                deg = len(active_edges[u]) + len(in_adj[u])
                if deg < min_deg:
                    min_deg = deg
                    v = u

            if v is None:
                self.add_clause([])
                return self.context

            # If v has in-degree or out-degree 0, no cycle can be formed
            if len(in_adj[v]) == 0 or len(active_edges[v]) == 0:
                self.add_clause([])
                return self.context

            # Outgoing active edges from v:
            outgoing_edges = [(v, w) for w in active_edges[v]]
            incoming_edges = [(u, v) for u in in_adj[v]]

            # VE-1 & VE-2: Exactly One constraint on incoming and outgoing edges of v
            incoming_vars = [self.H[(u, v)] for u, _ in incoming_edges]
            outgoing_vars = [self.H[(v, w)] for _, w in outgoing_edges]

            self.exactly_one_constraint(incoming_vars)
            self.exactly_one_constraint(outgoing_vars)

            # VE-3: No 2-cycle involving v
            for u, _ in incoming_edges:
                if (v, u) in outgoing_edges:
                    b_uv = self.H[(u, v)]
                    b_vu = self.H[(v, u)]
                    self.add_clause([-b_uv, -b_vu])

            # Prepare for step transition
            out_neighbors = list(active_edges[v])
            in_neighbors = list(in_adj[v])

            # Initialize next active edges
            next_active_edges = {u: set() for u in active_vertices if u != v}
            for u in active_vertices:
                if u == v:
                    continue
                for w in active_edges[u]:
                    if w != v:
                        next_active_edges[u].add(w)

            new_arc_vars = []
            updates = []

            # For each pair u in in_neighbors and w in out_neighbors (u != w):
            for u in in_neighbors:
                for w in out_neighbors:
                    if u == w:
                        continue

                    is_new_arc = w not in active_edges[u]
                    b_prime_uw = self.new_var()
                    
                    b_uv = self.H[(u, v)]
                    b_vw = self.H[(v, w)]

                    if is_new_arc:
                        # VE-4: b'_uw -> b_uv & b_vw
                        self.add_clause([-b_prime_uw, b_uv])
                        self.add_clause([-b_prime_uw, b_vw])

                        # VE-6: b_uv & b_vw -> b'_uw
                        self.add_clause([-b_uv, -b_vw, b_prime_uw])

                        new_arc_vars.append(b_prime_uw)
                    else:
                        # Existing arc (affected)
                        b_uw = self.H[(u, w)]

                        # VE-6: b_uv & b_vw -> b'_uw
                        self.add_clause([-b_uv, -b_vw, b_prime_uw])

                        # VE-7: b_uv & b_vw -> -b_uw
                        self.add_clause([-b_uv, -b_vw, -b_uw])

                        # VE-8: -b_uv | -b_vw -> (b_uw == b'_uw)
                        self.add_clause([b_uv, -b_uw, b_prime_uw])
                        self.add_clause([b_vw, -b_uw, b_prime_uw])
                        self.add_clause([b_uv, b_uw, -b_prime_uw])
                        self.add_clause([b_vw, b_uw, -b_prime_uw])

                    updates.append(((u, w), b_prime_uw))
                    next_active_edges[u].add(w)

            # Apply updates to self.H
            for edge, var in updates:
                self.H[edge] = var

            # VE-5: At Most One on new arcs
            if len(new_arc_vars) > 1:
                self.encoder.encode_amo(self.context, new_arc_vars)

            active_vertices.remove(v)
            active_edges = next_active_edges

        # 3. Base Case: len(active_vertices) == 3
        for r in active_vertices:
            incoming = []
            for u in active_vertices:
                if r in active_edges[u]:
                    incoming.append(self.H[(u, r)])

            outgoing = []
            for w in active_edges[r]:
                if w in active_vertices:
                    outgoing.append(self.H[(r, w)])

            self.exactly_one_constraint(incoming)
            self.exactly_one_constraint(outgoing)

        return self.context
