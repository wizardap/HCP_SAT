import math
from libs.models.successor.base import SuccessorMethod
from libs.utils.context import EncodingContext

LFSR_TAPS = {
    2: [1, 0],              # x^2 + x + 1
    3: [2, 0],              # x^3 + x + 1
    4: [3, 0],              # x^4 + x + 1
    5: [4, 1],              # x^5 + x^2 + 1
    6: [5, 0],              # x^6 + x + 1
    7: [6, 2],              # x^7 + x^3 + 1
    8: [7, 3, 2, 1],        # x^8 + x^4 + x^3 + x^2 + 1
    9: [8, 3],              # x^9 + x^4 + 1
    10: [9, 2],             # x^10 + x^3 + 1
    11: [10, 1],            # x^11 + x^2 + 1
    12: [11, 5, 3, 0],      # x^12 + x^6 + x^4 + x + 1
    13: [12, 3, 2, 0],      # x^13 + x^4 + x^3 + x + 1
    14: [13, 9, 5, 0],      # x^14 + x^10 + x^6 + x + 1
    15: [14, 0],            # x^15 + x + 1
    16: [15, 11, 2, 0]      # x^16 + x^12 + x^3 + x + 1
}


def lfsr_next(state, m):
    taps = LFSR_TAPS[m]
    feedback = 0
    for tap in taps:
        feedback ^= state[tap]
    return (feedback,) + state[:-1]


def get_target_state(m, step):
    state = tuple(1 if i == m - 1 else 0 for i in range(m))
    for _ in range(step):
        state = lfsr_next(state, m)
    return state


class LFSR(SuccessorMethod):
    def __init__(self, encoder=None):
        super().__init__(encoder)
        self.H = {}
        self.P = {}

    def getH(self, i, j):
        if (i, j) not in self.H:
            self.H[(i, j)] = self.new_var()
        return self.H[(i, j)]

    def getP(self, i, bit):  # P(i, bit) = 1 if the i-th bit of the Pi is 1
        if (i, bit) not in self.P:
            self.P[(i, bit)] = self.new_var()
        return self.P[(i, bit)]

    def add_default_variables(self, n, m, graph):
        # Hij = 0 if the arc (i, j) is not in the graph
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])
        # P1 = 1 (00..01)
        for bit in range(m):
            if bit == m - 1:
                self.add_clause([self.getP(1, bit)])
            else:
                self.add_clause([-self.getP(1, bit)])

    def vertex_outgoing_arcs(self, n):
        for i in range(1, n + 1):
            literals = []
            for j in range(1, n + 1):
                literals.append(self.getH(i, j))
            self.exactly_one_constraint(literals)

    def vertex_incoming_arcs(self, n):
        for j in range(1, n + 1):
            literals = []
            for i in range(1, n + 1):
                literals.append(self.getH(i, j))
            self.exactly_one_constraint(literals)

    def vertex_start(self, n, m):
        # s(1) = [1, 0, ..., 0] (only bit 0 is 1)
        for i in range(2, n + 1):
            for bit in range(m):
                if bit == 0:
                    self.add_clause([-self.getH(1, i), self.getP(i, bit)])
                else:
                    self.add_clause([-self.getH(1, i), -self.getP(i, bit)])

    def vertex_end(self, n, m):
        # s^(n-1)(1)
        target_state = get_target_state(m, n - 1)
        for i in range(2, n + 1):
            for bit in range(m):
                if target_state[bit] == 1:
                    self.add_clause([-self.getH(i, 1), self.getP(i, bit)])
                else:
                    self.add_clause([-self.getH(i, 1), -self.getP(i, bit)])

    def vertex_positions(self, n, m):
        taps = LFSR_TAPS[m]
        for i in range(2, n + 1):
            for j in range(2, n + 1):
                # Shift constraint: X_bit = Y_bit+1
                for bit in range(m - 1):
                    self.add_clause([-self.getH(i, j), -self.getP(i, bit), self.getP(j, bit + 1)])
                    self.add_clause([-self.getH(i, j), self.getP(i, bit), -self.getP(j, bit + 1)])
                
                # XOR feedback constraint: Y_0 = XOR(X_tap for tap in taps)
                # We forbid odd parity combinations of {Y_0} union {X_tap for tap in taps}
                num_vars = len(taps) + 1
                for comb in range(1 << num_vars):
                    if bin(comb).count('1') % 2 == 1:
                        clause = [-self.getH(i, j)]
                        # First variable is P(j, 0)
                        if (comb & 1):
                            clause.append(-self.getP(j, 0))
                        else:
                            clause.append(self.getP(j, 0))
                        # Remaining variables are P(i, tap)
                        for idx, tap in enumerate(taps):
                            if (comb & (1 << (idx + 1))):
                                clause.append(-self.getP(i, tap))
                            else:
                                clause.append(self.getP(i, tap))
                        self.add_clause(clause)

    def prevent_zero_state(self, n, m):
        # Forbid P_i = 0 for all i >= 2
        for i in range(2, n + 1):
            self.add_clause([self.getP(i, bit) for bit in range(m)])

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        m = math.ceil(math.log2(n)) if math.log2(n) != int(math.log2(n)) else int(math.log2(n)) + 1

        if m > 16:
            raise ValueError(f"LFSR currently only supports graphs up to 65,535 vertices (m <= 16). Got m={m} for n={n}.")

        self.add_default_variables(n, m, graph)
        self.vertex_outgoing_arcs(n)
        self.vertex_incoming_arcs(n)
        self.vertex_start(n, m)
        self.vertex_end(n, m)
        self.vertex_positions(n, m)
        self.prevent_zero_state(n, m)
        return self.context
