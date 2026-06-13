from libs.models.successor.base import SuccessorMethod
from libs.utils.context import EncodingContext

def lfsr(n, size, xor):
    """
    Linear-feedback shift register (LFSR) step function.
    Given current state 'n', size of shift register, and XOR tap index.
    """
    m = n << 1
    x = m & (1 << size)
    if x:
        m = m - x + 1
    m ^= x >> xor
    return m

class OCRT(SuccessorMethod):
    """
    Hybrid VEE + CRT Successor Encoding for Hamiltonian Cycle Problem.
    """
    def __init__(self, vee_encoder=None, crt_encoder=None, cycle=None):
        super().__init__(encoder=None)
        self.vee_encoder = vee_encoder
        self.crt_encoder = crt_encoder
        self.cycle_override = cycle
        self.H = {}          # (u, w) -> active variable
        self.original_H = {} # (u, w) -> original step 0 variable
        self.V = {}          # v -> vertex variable
        self.P = {}          # (i, bit_idx) -> position bit variable
        self.is_directed = True
        self._dummy_v = None

    def getH(self, u, w):
        """
        Get the original step 0 variable for result extraction.
        If the edge didn't exist, returns a dummy variable forced to False.
        """
        if (u, w) not in self.original_H:
            return self._dummy_var()
        return self.original_H[(u, w)]

    def _dummy_var(self):
        if self._dummy_v is None:
            self._dummy_v = self.new_var()
            self.add_clause([-self._dummy_v])
        return self._dummy_v

    def getP(self, i, bit_idx):
        """Get or create the bit variable index for vertex i at bit_idx"""
        if (i, bit_idx) not in self.P:
            self.P[(i, bit_idx)] = self.new_var()
        return self.P[(i, bit_idx)]

    def get_cycle_and_nbits(self, nNode):
        """
        Determine the cycle size and the number of bits needed to represent the counters.
        Matches the calculation logic in crt.py.
        """
        if self.cycle_override is not None:
            cycle = self.cycle_override
        else:
            cycle = 2
            if cycle <= nNode:
                cycle *= 3
            if cycle <= nNode:
                cycle *= 5
            if cycle <= nNode:
                cycle *= 7
            while cycle <= nNode:
                cycle *= 2

        nBits = 0
        k = 1
        while True:
            if (cycle % (1 << k)) == 0:
                nBits += 1
            else:
                break
            k += 1

        if (cycle % 3) == 0:
            nBits += 2
        if (cycle % 5) == 0:
            nBits += 3
        if (cycle % 7) == 0:
            nBits += 3
        if (cycle % 511) == 0:
            nBits += 9
        if (cycle % 1023) == 0:
            nBits += 10
        if (cycle % 2047) == 0:
            nBits += 11

        return cycle, nBits

    def enforce_non_zero_lfsr(self, active_vertices, cycle, nBits):
        """
        Enforce that the bit-vectors representing the LFSR/modulo counters cannot be all-zero.
        """
        for i in active_vertices:
            b = 0
            k = 1
            # Skip binary part bits (they are allowed to be all-zero)
            while True:
                if (cycle % (1 << k)) == 0:
                    b += 1
                else:
                    break
                k += 1

            if (cycle % 3) == 0:
                self.add_clause([self.getP(i, b), self.getP(i, b + 1)])
                b += 2
            if (cycle % 5) == 0:
                self.add_clause([-self.getP(i, b), -self.getP(i, b + 2)])
                self.add_clause([-self.getP(i, b + 1), -self.getP(i, b + 2)])
                b += 3
            if (cycle % 7) == 0:
                self.add_clause([self.getP(i, b), self.getP(i, b + 1), self.getP(i, b + 2)])
                b += 3
            if (cycle % 511) == 0:
                self.add_clause([self.getP(i, b + j) for j in range(9)])
                b += 9
            if (cycle % 1023) == 0:
                self.add_clause([self.getP(i, b + j) for j in range(10)])
                b += 10
            if (cycle % 2047) == 0:
                self.add_clause([self.getP(i, b + j) for j in range(11)])
                b += 11

    def init_starting_position(self, first, cycle):
        """
        Set up initial counter state for the start vertex (state 0 for binary, LFSR seed states for others).
        """
        b = 0
        k = 1
        while True:
            if (cycle % (1 << k)) == 0:
                self.add_clause([-self.getP(first, b)])
                b += 1
            else:
                break
            k += 1

        if (cycle % 3) == 0:
            self.add_clause([self.getP(first, b)])
            b += 1
            self.add_clause([-self.getP(first, b)])
            b += 1
        if (cycle % 5) == 0:
            self.add_clause([-self.getP(first, b)])
            b += 1
            self.add_clause([-self.getP(first, b)])
            b += 1
            self.add_clause([-self.getP(first, b)])
            b += 1
        if (cycle % 7) == 0:
            self.add_clause([self.getP(first, b)])
            b += 1
            self.add_clause([-self.getP(first, b)])
            b += 1
            self.add_clause([-self.getP(first, b)])
            b += 1

        if (cycle % 511) == 0:
            self.add_clause([self.getP(first, b)])
            b += 1
            for k in range(2, 10):
                self.add_clause([-self.getP(first, b)])
                b += 1
        if (cycle % 1023) == 0:
            self.add_clause([self.getP(first, b)])
            b += 1
            for k in range(2, 11):
                self.add_clause([-self.getP(first, b)])
                b += 1
        if (cycle % 2047) == 0:
            self.add_clause([self.getP(first, b)])
            b += 1
            for k in range(2, 12):
                self.add_clause([-self.getP(first, b)])
                b += 1

    def init_termination_position(self, n_prime, first, first_neighbors, cycle):
        """
        Ensure that if a neighbor is connected to 'first' as the last step in the cycle,
        its counter state matches the representation of step n_prime - 1.
        """
        for neighbor in first_neighbors:
            b = 0
            h_var = self.H[(neighbor, first)]

            k = 1
            while True:
                if (cycle % (1 << k)) == 0:
                    clause = [-h_var]
                    if ((n_prime - 1) & ((1 << k) // 2)) == 0:
                        clause.append(-self.getP(neighbor, b))
                    else:
                        clause.append(self.getP(neighbor, b))
                    self.add_clause(clause)
                    b += 1
                else:
                    break
                k += 1

            if (cycle % 3) == 0:
                mask = 1
                for i in range((n_prime - 1) % 3):
                    mask = lfsr(mask, 2, 1)
                for i in range(2):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 2

            if (cycle % 5) == 0:
                mask = (n_prime + 4) % 5
                for i in range(3):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 3

            if (cycle % 7) == 0:
                mask = 1
                for i in range((n_prime - 1) % 7):
                    mask = lfsr(mask, 3, 1)
                for i in range(3):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 3

            if (cycle % 511) == 0:
                mask = 1
                for i in range((n_prime - 1) % 511):
                    mask = lfsr(mask, 9, 4)
                for i in range(9):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 9

            if (cycle % 1023) == 0:
                mask = 1
                for i in range((n_prime - 1) % 1023):
                    mask = lfsr(mask, 10, 3)
                for i in range(10):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 10

            if (cycle % 2047) == 0:
                mask = 1
                for i in range((n_prime - 1) % 2047):
                    mask = lfsr(mask, 11, 2)
                for i in range(11):
                    clause = [-h_var]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 11

    def add_transitions(self, active_vertices, active_edges, first, cycle):
        """
        Add state transition constraints modulo each coprime factor for active edges.
        """
        for i in active_vertices:
            for j in active_edges[i]:
                if j != first:
                    h_var = self.H[(i, j)]
                    b = 0
                    # Modulo 2 transition: y0 = not x0
                    if (cycle % 2) == 0:
                        self.add_clause([-h_var, self.getP(j, b), self.getP(i, b)])
                        self.add_clause([-h_var, -self.getP(j, b), -self.getP(i, b)])
                        b += 1

                    # Higher powers of 2 transition (binary addition logic)
                    k = 2
                    while True:
                        if (cycle % (1 << k)) == 0:
                            for l in range(1, k):
                                self.add_clause([-h_var, self.getP(i, b - l), self.getP(j, b), -self.getP(i, b)])
                                self.add_clause([-h_var, self.getP(i, b - l), -self.getP(j, b), self.getP(i, b)])
                            clause = [-self.getP(i, b - l) for l in range(1, k)] + [-h_var, self.getP(j, b), self.getP(i, b)]
                            self.add_clause(clause)
                            clause = [-self.getP(i, b - l) for l in range(1, k)] + [-h_var, -self.getP(j, b), -self.getP(i, b)]
                            self.add_clause(clause)
                            b += 1
                        else:
                            break
                        k += 1

                    # Modulo 3 transition: y0 = x1, y1 = x0 ^ x1
                    if (cycle % 3) == 0:
                        self.add_clause([-h_var, self.getP(j, b), -self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b), self.getP(i, b + 1)])
                        self.add_clause([-h_var, self.getP(j, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-h_var, self.getP(j, b + 1), -self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                        b += 2

                    # Modulo 5 transition: y0 = not (x0 or x2), y1 = x0 ^ x1, y2 = x0 and x1
                    if (cycle % 5) == 0:
                        self.add_clause([-h_var, -self.getP(j, b), -self.getP(i, b)])
                        self.add_clause([-h_var, -self.getP(j, b), -self.getP(i, b + 2)])
                        self.add_clause([-h_var, self.getP(j, b), self.getP(i, b), self.getP(i, b + 2)])
                        self.add_clause([-h_var, self.getP(j, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-h_var, self.getP(j, b + 1), -self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-h_var, -self.getP(j, b + 2), self.getP(i, b)])
                        self.add_clause([-h_var, -self.getP(j, b + 2), self.getP(i, b + 1)])
                        self.add_clause([-h_var, self.getP(j, b + 2), -self.getP(i, b), -self.getP(i, b + 1)])
                        b += 3

                    # Modulo 7 transition: y0 = x2, y1 = x0, y2 = x1 ^ x2
                    if (cycle % 7) == 0:
                        self.add_clause([-h_var, self.getP(j, b), -self.getP(i, b + 2)])
                        self.add_clause([-h_var, -self.getP(j, b), self.getP(i, b + 2)])
                        self.add_clause([-h_var, self.getP(j, b + 1), -self.getP(i, b)])
                        self.add_clause([-h_var, -self.getP(j, b + 1), self.getP(i, b)])
                        self.add_clause([-h_var, self.getP(j, b + 2), self.getP(i, b + 1), -self.getP(i, b + 2)])
                        self.add_clause([-h_var, self.getP(j, b + 2), -self.getP(i, b + 1), self.getP(i, b + 2)])
                        self.add_clause([-h_var, -self.getP(j, b + 2), self.getP(i, b + 1), self.getP(i, b + 2)])
                        self.add_clause([-h_var, -self.getP(j, b + 2), -self.getP(i, b + 1), -self.getP(i, b + 2)])
                        b += 3

                    # Modulo 511 transition (LFSR 9 bits, xor 4)
                    if (cycle % 511) == 0:
                        self.add_clause([-h_var, self.getP(j, b), -self.getP(i, b + 8)])
                        self.add_clause([-h_var, -self.getP(j, b), self.getP(i, b + 8)])
                        for idx in range(1, 5):
                            self.add_clause([-h_var, self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-h_var, -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-h_var, self.getP(j, b + 5), self.getP(i, b + 4), -self.getP(i, b + 8)])
                        self.add_clause([-h_var, self.getP(j, b + 5), -self.getP(i, b + 4), self.getP(i, b + 8)])
                        self.add_clause([-h_var, -self.getP(j, b + 5), self.getP(i, b + 4), self.getP(i, b + 8)])
                        self.add_clause([-h_var, -self.getP(j, b + 5), -self.getP(i, b + 4), -self.getP(i, b + 8)])
                        for idx in range(6, 9):
                            self.add_clause([-h_var, self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-h_var, -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        b += 9

                    # Modulo 1023 transition (LFSR 10 bits, xor 3)
                    if (cycle % 1023) == 0:
                        self.add_clause([-h_var, self.getP(j, b), -self.getP(i, b + 9)])
                        self.add_clause([-h_var, -self.getP(j, b), self.getP(i, b + 9)])
                        for idx in range(1, 7):
                            self.add_clause([-h_var, self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-h_var, -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-h_var, self.getP(j, b + 7), self.getP(i, b + 6), -self.getP(i, b + 9)])
                        self.add_clause([-h_var, self.getP(j, b + 7), -self.getP(i, b + 6), self.getP(i, b + 9)])
                        self.add_clause([-h_var, -self.getP(j, b + 7), self.getP(i, b + 6), self.getP(i, b + 9)])
                        self.add_clause([-h_var, -self.getP(j, b + 7), -self.getP(i, b + 6), -self.getP(i, b + 9)])
                        for idx in range(8, 10):
                            self.add_clause([-h_var, self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-h_var, -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        b += 10

                    # Modulo 2047 transition (LFSR 11 bits, xor 2)
                    if (cycle % 2047) == 0:
                        self.add_clause([-h_var, self.getP(j, b), -self.getP(i, b + 10)])
                        self.add_clause([-h_var, -self.getP(j, b), self.getP(i, b + 10)])
                        for idx in range(1, 9):
                            self.add_clause([-h_var, self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-h_var, -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-h_var, self.getP(j, b + 9), self.getP(i, b + 8), -self.getP(i, b + 10)])
                        self.add_clause([-h_var, self.getP(j, b + 9), -self.getP(i, b + 8), self.getP(i, b + 10)])
                        self.add_clause([-h_var, -self.getP(j, b + 9), self.getP(i, b + 8), self.getP(i, b + 10)])
                        self.add_clause([-h_var, -self.getP(j, b + 9), -self.getP(i, b + 8), -self.getP(i, b + 10)])
                        self.add_clause([-h_var, self.getP(j, b + 10), -self.getP(i, b + 9)])
                        self.add_clause([-h_var, -self.getP(j, b + 10), self.getP(i, b + 9)])
                        b += 11

    def _vee_eo(self, literals):
        if self.vee_encoder is None:
            raise ValueError("No vee_encoder set for OCRT.")
        self.vee_encoder.encode_eo(self.context, literals)

    def _vee_amo(self, literals):
        if self.vee_encoder is None:
            raise ValueError("No vee_encoder set for OCRT.")
        self.vee_encoder.encode_amo(self.context, literals)

    def _crt_eo(self, literals):
        if self.crt_encoder is None:
            raise ValueError("No crt_encoder set for OCRT.")
        self.crt_encoder.encode_eo(self.context, literals)

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        self.is_directed = graph.is_directed

        self.original_H = {}
        self.H = {}
        self.V = {}
        self.P = {}
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
            
        if self.crt_encoder is None:
            if self.encoder is not None:
                self.crt_encoder = self.encoder
            else:
                try:
                    from libs.encodings.cardinality_one.pblib import PbLibEncoder
                    self.crt_encoder = PbLibEncoder()
                except (ImportError, Exception):
                    from libs.encodings.cardinality_one.scl import NSCEncoding
                    self.crt_encoder = NSCEncoding()

        # 2. Sequential Elimination Loop (Phase 1 VEE)
        sigma = 0
        while True:
            # First, check if VEE has reduced to 3 vertices
            n_prime = len(active_vertices)
            if n_prime == 3:
                # Base Case VEE
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

            # Find in-neighbors for all active vertices to compute degree and check degree-0
            in_adj = {u: set() for u in active_vertices}
            for u in active_vertices:
                for w in active_edges[u]:
                    if w in active_vertices:
                        in_adj[w].add(u)

            # Degree-0 check
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
                # Sparsity condition false -> break VEE, proceed to Phase 2 CRT
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

            # Outgoing active edges from v:
            outgoing_edges = [(v, w) for w in active_edges[v]]
            incoming_edges = [(u, v) for u in in_adj[v]]

            # VE-1 & VE-2: Exactly One constraint on incoming and outgoing edges of v
            incoming_vars = [self.H[(u, v)] for u, _ in incoming_edges]
            outgoing_vars = [self.H[(v, w)] for _, w in outgoing_edges]

            self._vee_eo(incoming_vars)
            self._vee_eo(outgoing_vars)

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
                self._vee_amo(new_arc_vars)

            active_vertices.remove(v)
            active_edges = next_active_edges
            sigma += 1

        # Phase 2: CRT Encoding on remaining graph
        n_prime = len(active_vertices)
        print(f"[OCRT] VEE Phase 1 complete. Eliminated: {sigma} vertices. Remaining (n_prime): {n_prime} vertices.")

        # 3. Base Case check (if sparsity was violated, but we are at 3 vertices anyway, or less)
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

        # Starting vertex: min out-degree vertex in active_vertices
        first = min(active_vertices, key=lambda v: len(active_edges[v]))

        cycle, nBits = self.get_cycle_and_nbits(n_prime)

        # 1. Exactly one outgoing edge
        for i in active_vertices:
            literals = []
            for j in active_edges[i]:
                literals.append(self.H[(i, j)])
            self._crt_eo(literals)

        # 2. Exactly one incoming edge
        in_adj = {u: set() for u in active_vertices}
        for u in active_vertices:
            for w in active_edges[u]:
                if w in active_vertices:
                    in_adj[w].add(u)

        for j in active_vertices:
            literals = []
            for i in in_adj[j]:
                literals.append(self.H[(i, j)])
            self._crt_eo(literals)

        # 3. Enforce non-zero representation
        self.enforce_non_zero_lfsr(active_vertices, cycle, nBits)

        # 4. Connect one neighbor back to start (final edge)
        first_neighbors = [u for u in active_vertices if first in active_edges[u]]
        if not first_neighbors:
            # No path back to first in remaining graph -> UNSAT
            self.add_clause([])
            return self.context

        self.add_clause([self.H[(neighbor, first)] for neighbor in first_neighbors])

        # 5. Symmetry breaking
        if not self.is_directed:
            for i in range(len(first_neighbors)):
                clause = []
                for j in range(i):
                    neighbor_j = first_neighbors[j]
                    if neighbor_j in active_edges[first]:
                        clause.append(self.H[(first, neighbor_j)])
                clause.append(-self.H[(first_neighbors[i], first)])
                self.add_clause(clause)

        # 6. Initialize starting position
        self.init_starting_position(first, cycle)

        # 7. Initialize termination position
        self.init_termination_position(n_prime, first, first_neighbors, cycle)

        # 8. Add transitions
        self.add_transitions(active_vertices, active_edges, first, cycle)

        return self.context
