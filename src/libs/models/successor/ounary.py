from libs.models.successor.base import SuccessorMethod
from libs.utils.context import EncodingContext

class OUnary(SuccessorMethod):
    """
    Hybrid VEE + Unary Successor Encoding for Hamiltonian Cycle Problem.
    """
    def __init__(self, vee_encoder=None):
        super().__init__(encoder=None)
        self.vee_encoder = vee_encoder
        self.H = {}          # (u, w) -> active variable
        self.original_H = {} # (u, w) -> original step 0 variable
        self.V = {}          # v -> vertex variable
        self.U = {}          # (i, p) -> unary position variable
        self.is_directed = True
        self._dummy_v = None

    def getH(self, u, w):
        if (u, w) not in self.original_H:
            return self._dummy_var()
        return self.original_H[(u, w)]

    def _dummy_var(self):
        if self._dummy_v is None:
            self._dummy_v = self.new_var()
            self.add_clause([-self._dummy_v])
        return self._dummy_v

    def _vee_eo(self, literals):
        if self.vee_encoder is None:
            raise ValueError("No vee_encoder set for OUnary.")
        self.vee_encoder.encode_eo(self.context, literals)

    def _vee_amo(self, literals):
        if self.vee_encoder is None:
            raise ValueError("No vee_encoder set for OUnary.")
        self.vee_encoder.encode_amo(self.context, literals)

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        self.is_directed = graph.is_directed

        self.original_H = {}
        self.H = {}
        self.V = {}
        self.U = {}
        self._dummy_v = None

        # 1. Initialize variables for vertices and original edges
        for v in range(1, n + 1):
            self.V[v] = self.new_var()
            self.add_clause([self.V[v]])

        for u in range(1, n + 1):
            for w in graph.graph[u]:
                self.original_H[(u, w)] = self.new_var()

        # Copy original variables to active variables mapping
        self.H = self.original_H.copy()

        # Handle trivial cases
        if n < 3:
            self.add_clause([])
            return self.context

        # Set up active vertices and active edges
        active_vertices = set(range(1, n + 1))
        active_edges = {u: set(graph.graph[u]) for u in range(1, n + 1)}

        # Setup encoders
        if self.vee_encoder is None:
            from libs.encodings.cardinality_one.scl import NSCEncoding
            self.vee_encoder = NSCEncoding()

        # 2. Sequential Elimination Loop (Phase 1 VEE)
        sigma = 0
        while True:
            n_prime = len(active_vertices)
            if n_prime == 3:
                for r in active_vertices:
                    incoming = []
                    for u in active_vertices:
                        if r in active_edges[u]:
                            incoming.append(self.H[(u, r)])
                    outgoing = []
                    for w in active_edges[r]:
                        if w in active_vertices:
                            outgoing.append(self.H[(r, w)])
                    self._vee_eo(incoming)
                    self._vee_eo(outgoing)
                return self.context

            if n_prime < 3:
                self.add_clause([])
                return self.context

            in_adj = {u: set() for u in active_vertices}
            for u in active_vertices:
                for w in active_edges[u]:
                    if w in active_vertices:
                        in_adj[w].add(u)

            has_deg_0 = False
            for u in active_vertices:
                if len(active_edges[u]) == 0 or len(in_adj[u]) == 0:
                    has_deg_0 = True
                    break

            if has_deg_0:
                self.add_clause([])
                return self.context

            # Sparsity check
            d_min = min(len(active_edges[u]) for u in active_vertices)
            if not (d_min * sigma <= n):
                break

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

            outgoing_edges = [(v, w) for w in active_edges[v]]
            incoming_edges = [(u, v) for u in in_adj[v]]

            incoming_vars = [self.H[(u, v)] for u, _ in incoming_edges]
            outgoing_vars = [self.H[(v, w)] for _, w in outgoing_edges]

            self._vee_eo(incoming_vars)
            self._vee_eo(outgoing_vars)

            for u, _ in incoming_edges:
                if (v, u) in outgoing_edges:
                    b_uv = self.H[(u, v)]
                    b_vu = self.H[(v, u)]
                    self.add_clause([-b_uv, -b_vu])

            out_neighbors = list(active_edges[v])
            in_neighbors = list(in_adj[v])

            next_active_edges = {u: set() for u in active_vertices if u != v}
            for u in active_vertices:
                if u == v: continue
                for w in active_edges[u]:
                    if w != v: next_active_edges[u].add(w)

            new_arc_vars = []
            updates = []

            for u in in_neighbors:
                for w in out_neighbors:
                    if u == w: continue
                    is_new_arc = w not in active_edges[u]
                    b_prime_uw = self.new_var()
                    b_uv = self.H[(u, v)]
                    b_vw = self.H[(v, w)]

                    if is_new_arc:
                        self.add_clause([-b_prime_uw, b_uv])
                        self.add_clause([-b_prime_uw, b_vw])
                        self.add_clause([-b_uv, -b_vw, b_prime_uw])
                        new_arc_vars.append(b_prime_uw)
                    else:
                        b_uw = self.H[(u, w)]
                        self.add_clause([-b_uv, -b_vw, b_prime_uw])
                        self.add_clause([-b_uv, -b_vw, -b_uw])
                        self.add_clause([b_uv, -b_uw, b_prime_uw])
                        self.add_clause([b_vw, -b_uw, b_prime_uw])
                        self.add_clause([b_uv, b_uw, -b_prime_uw])
                        self.add_clause([b_vw, b_uw, -b_prime_uw])

                    updates.append(((u, w), b_prime_uw))
                    next_active_edges[u].add(w)

            for edge, var in updates:
                self.H[edge] = var

            if len(new_arc_vars) > 1:
                self._vee_amo(new_arc_vars)

            active_vertices.remove(v)
            active_edges = next_active_edges
            sigma += 1

        n_prime = len(active_vertices)
        # print(f"[O-UNARY] VEE Phase 1 complete. Eliminated: {sigma} vertices. Remaining: {n_prime} vertices.")

        if n_prime == 3:
            for r in active_vertices:
                incoming = [self.H[(u, r)] for u in active_vertices if r in active_edges[u]]
                outgoing = [self.H[(r, w)] for w in active_edges[r] if w in active_vertices]
                self._vee_eo(incoming)
                self._vee_eo(outgoing)
            return self.context

        if n_prime < 3:
            self.add_clause([])
            return self.context

        first = min(active_vertices, key=lambda v: len(active_edges[v]))

        # 1. Exactly one outgoing edge
        for i in active_vertices:
            literals = [self.H[(i, j)] for j in active_edges[i]]
            self._vee_eo(literals)

        # 2. Exactly one incoming edge
        in_adj = {u: set() for u in active_vertices}
        for u in active_vertices:
            for w in active_edges[u]:
                if w in active_vertices:
                    in_adj[w].add(u)

        for j in active_vertices:
            literals = [self.H[(i, j)] for i in in_adj[j]]
            self._vee_eo(literals)

        # 3. Connect one neighbor back to start (final edge)
        first_neighbors = [u for u in active_vertices if first in active_edges[u]]
        if not first_neighbors:
            self.add_clause([])
            return self.context

        self.add_clause([self.H[(neighbor, first)] for neighbor in first_neighbors])

        # 4. Symmetry breaking
        if not self.is_directed:
            for i in range(len(first_neighbors)):
                clause = []
                for j in range(i):
                    neighbor_j = first_neighbors[j]
                    if neighbor_j in active_edges[first]:
                        clause.append(self.H[(first, neighbor_j)])
                clause.append(-self.H[(first_neighbors[i], first)])
                self.add_clause(clause)

        # 5. Unary Phase
        def getU(i, p):
            if (i, p) not in self.U:
                self.U[(i, p)] = self.new_var()
            return self.U[(i, p)]

        # start_vertex position is 1
        self.add_clause([getU(first, 1)])

        # Exactly one position per vertex
        for i in active_vertices:
            literals = [getU(i, p) for p in range(1, n_prime + 1)]
            self._vee_eo(literals)

        # Transitions
        for i in active_vertices:
            for j in active_edges[i]:
                if i == first and j != first:
                    self.add_clause([-self.H[(i, j)], getU(j, 2)])
                elif j == first and i != first:
                    self.add_clause([-self.H[(i, j)], getU(i, n_prime)])
                elif i != first and j != first:
                    for p in range(2, n_prime):
                        self.add_clause([-self.H[(i, j)], -getU(i, p), getU(j, p + 1)])

        return self.context
