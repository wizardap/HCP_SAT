import math
from itertools import combinations
from libs.models.successor.base import SuccessorMethod
from libs.utils.context import EncodingContext

def lfsr(n, size, xor):
    m = n << 1
    x = m & (1 << size)
    if x:
        m = m - x + 1
    m ^= x >> xor
    return m

class OptimizedCRT(SuccessorMethod):
    def __init__(self, encoder=None, cycle=None):
        super().__init__(encoder)
        self.H = {}
        self.P = {}
        
        # (bits, clauses_per_edge, period, name, xor_tap)
        # Mod2 is special: 1 bit, period 2. No XOR tap needed (y = not x)
        self.blocks = [
            (1, 2, 2, "Mod2", None),
            (2, 6, 3, "Size2", 1),
            (3, 8, 7, "Size3", 1),
            (5, 12, 31, "Size5", 2),
            (7, 16, 127, "Size7", 1),
            (11, 24, 2047, "Size11", 2),
        ]

    def getH(self, i, j):
        if (i, j) not in self.H:
            self.H[(i, j)] = self.new_var()
        return self.H[(i, j)]

    def getP(self, i, bit_idx):
        if (i, bit_idx) not in self.P:
            self.P[(i, bit_idx)] = self.new_var()
        return self.P[(i, bit_idx)]

    def get_optimal_combo(self, nNode):
        best_bits = 999
        best_clauses = 999
        best_combo = None
        
        for r in range(1, len(self.blocks)+1):
            for combo in combinations(self.blocks, r):
                prod = 1
                bits = 0
                clauses = 0
                for c in combo:
                    prod *= c[2]
                    bits += c[0]
                    clauses += c[1]
                    
                if prod >= nNode:
                    if bits < best_bits or (bits == best_bits and clauses < best_clauses):
                        best_bits = bits
                        best_clauses = clauses
                        best_combo = combo
        return best_combo

    def enforce_non_zero_lfsr(self, n, combo):
        for i in range(1, n + 1):
            b = 0
            for c in combo:
                bits = c[0]
                name = c[3]
                if name == "Mod2":
                    # Mod2 can be 0 or 1, no restriction
                    b += 1
                else:
                    # LFSR must not be all zero
                    self.add_clause([self.getP(i, b + j) for j in range(bits)])
                    b += bits

    def init_starting_position(self, first, combo):
        b = 0
        for c in combo:
            bits = c[0]
            name = c[3]
            if name == "Mod2":
                self.add_clause([-self.getP(first, b)])
                b += 1
            else:
                self.add_clause([self.getP(first, b)])
                for j in range(1, bits):
                    self.add_clause([-self.getP(first, b + j)])
                b += bits

    def init_termination_position(self, n, first, first_neighbors, combo):
        for neighbor in first_neighbors:
            b = 0
            for c in combo:
                bits = c[0]
                period = c[2]
                name = c[3]
                xor_tap = c[4]
                
                if name == "Mod2":
                    clause = [-self.getH(neighbor, first)]
                    if ((n - 1) % 2) == 0:
                        clause.append(-self.getP(neighbor, b))
                    else:
                        clause.append(self.getP(neighbor, b))
                    self.add_clause(clause)
                    b += 1
                else:
                    mask = 1
                    for _ in range((n - 1) % period):
                        mask = lfsr(mask, bits, xor_tap)
                        
                    for i in range(bits):
                        clause = [-self.getH(neighbor, first)]
                        if (mask & 1) == 0:
                            clause.append(-self.getP(neighbor, b + i))
                        else:
                            clause.append(self.getP(neighbor, b + i))
                        self.add_clause(clause)
                        mask = mask >> 1
                    b += bits

    def add_transitions(self, n, graph, first, combo):
        for i in range(1, n + 1):
            for j in graph.graph[i]:
                if j != first:
                    b = 0
                    for c in combo:
                        bits = c[0]
                        name = c[3]
                        xor_tap = c[4]
                        
                        if name == "Mod2":
                            # y0 = not x0
                            self.add_clause([-self.getH(i, j), self.getP(j, b), self.getP(i, b)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b), -self.getP(i, b)])
                            b += 1
                        else:
                            # Galois LFSR transition
                            # y[0] = x[bits-1]
                            self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + bits - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + bits - 1)])
                            
                            for idx in range(1, bits):
                                if idx == bits - xor_tap:
                                    # y[idx] = x[idx-1] ^ x[bits-1]
                                    self.add_clause([-self.getH(i, j), self.getP(j, b + idx), self.getP(i, b + idx - 1), -self.getP(i, b + bits - 1)])
                                    self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1), self.getP(i, b + bits - 1)])
                                    self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1), self.getP(i, b + bits - 1)])
                                    self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), -self.getP(i, b + idx - 1), -self.getP(i, b + bits - 1)])
                                else:
                                    # y[idx] = x[idx-1]
                                    self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                                    self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                            b += bits

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        first = graph.start_vertex

        combo = self.get_optimal_combo(n)
        if combo is None:
            raise ValueError("Graph too large, max period is 169M")

        # 1. Non-edges are set to False
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])

        # 2. Exactly one outgoing edge
        for i in range(1, n + 1):
            literals = [self.getH(i, j) for j in graph.graph[i]]
            self.exactly_one_constraint(literals)

        # 3. Exactly one incoming edge
        for j in range(1, n + 1):
            literals = [self.getH(i, j) for i in range(1, n + 1) if j in graph.graph[i]]
            self.exactly_one_constraint(literals)

        # 4. Enforce non-zero representation
        self.enforce_non_zero_lfsr(n, combo)

        # 5. Connect one neighbor back to start (final edge)
        first_neighbors = [u for u in range(1, n + 1) if first in graph.graph[u]]
        self.add_clause([self.getH(neighbor, first) for neighbor in first_neighbors])

        # 6. Symmetry breaking
        if not graph.is_directed:
            for i in range(len(first_neighbors)):
                clause = [self.getH(first, first_neighbors[j]) for j in range(i)]
                clause.append(-self.getH(first_neighbors[i], first))
                self.add_clause(clause)

        # 7. Initialize starting position
        self.init_starting_position(first, combo)

        # 8. Initialize termination position
        self.init_termination_position(n, first, first_neighbors, combo)

        # 9. Add transitions
        self.add_transitions(n, graph, first, combo)

        return self.context
