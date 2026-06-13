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


class CRT(SuccessorMethod):
    """
    Successor encoding based on the Chinese Remainder Theorem (CRT).
    It decomposes the cycle counter into coprime moduli (powers of 2, 3, 5, 7, 511, 1023, 2047)
    and enforces state transitions modulo each factor.
    """
    def __init__(self, encoder=None, cycle=None):
        super().__init__(encoder)
        self.cycle_override = cycle
        self.H = {}  # H[(i, j)] represents directed edge variable i -> j
        self.P = {}  # P[(i, bit)] represents position bit variable

    def getH(self, i, j):
        """Get or create the directed edge variable from i to j"""
        if (i, j) not in self.H:
            self.H[(i, j)] = self.new_var()
        return self.H[(i, j)]

    def getP(self, i, bit_idx):
        """Get or create the bit variable index for vertex i at bit_idx"""
        if (i, bit_idx) not in self.P:
            self.P[(i, bit_idx)] = self.new_var()
        return self.P[(i, bit_idx)]

    def get_cycle_and_nbits(self, nNode):
        """
        Determine the cycle size and the number of bits needed to represent the counters.
        Matches the calculation logic in encode.py.
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

    def enforce_non_zero_lfsr(self, n, cycle, nBits):
        """
        Enforce that the bit-vectors representing the LFSR/modulo counters cannot be all-zero.
        """
        for i in range(1, n + 1):
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

    def init_termination_position(self, n, first, first_neighbors, cycle):
        """
        Ensure that if a neighbor is connected to 'first' as the last step in the cycle,
        its counter state matches the representation of step nNode - 1.
        """
        minDegree = len(first_neighbors)
        for j in range(minDegree):
            neighbor = first_neighbors[j]
            b = 0

            k = 1
            while True:
                if (cycle % (1 << k)) == 0:
                    clause = [-self.getH(neighbor, first)]
                    if ((n - 1) & (1 << k) // 2) == 0:
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
                for i in range((n - 1) % 3):
                    mask = lfsr(mask, 2, 1)
                for i in range(2):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 2

            if (cycle % 5) == 0:
                mask = (n + 4) % 5
                for i in range(3):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 3

            if (cycle % 7) == 0:
                mask = 1
                for i in range((n - 1) % 7):
                    mask = lfsr(mask, 3, 1)
                for i in range(3):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 3

            if (cycle % 511) == 0:
                mask = 1
                for i in range((n - 1) % 511):
                    mask = lfsr(mask, 9, 4)
                for i in range(9):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 9

            if (cycle % 1023) == 0:
                mask = 1
                for i in range((n - 1) % 1023):
                    mask = lfsr(mask, 10, 3)
                for i in range(10):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 10

            if (cycle % 2047) == 0:
                mask = 1
                for i in range((n - 1) % 2047):
                    mask = lfsr(mask, 11, 2)
                for i in range(11):
                    clause = [-self.getH(neighbor, first)]
                    if (mask & 1) == 0:
                        clause.append(-self.getP(neighbor, b + i))
                    else:
                        clause.append(self.getP(neighbor, b + i))
                    self.add_clause(clause)
                    mask = mask >> 1
                b += 11

    def add_transitions(self, n, graph, first, cycle):
        """
        For each valid directed edge i -> j (j != first), if the edge is active,
        the state at vertex j must be the successor of the state at vertex i.
        """
        for i in range(1, n + 1):
            for j in graph.graph[i]:
                if j != first:
                    b = 0
                    # Modulo 2 transition: y0 = not x0
                    if (cycle % 2) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), self.getP(i, b)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), -self.getP(i, b)])
                        b += 1

                    # Higher powers of 2 transition (binary addition logic)
                    k = 2
                    while True:
                        if (cycle % (1 << k)) == 0:
                            for l in range(1, k):
                                self.add_clause([-self.getH(i, j), self.getP(i, b - l), self.getP(j, b), -self.getP(i, b)])
                                self.add_clause([-self.getH(i, j), self.getP(i, b - l), -self.getP(j, b), self.getP(i, b)])
                            clause = [-self.getP(i, b - l) for l in range(1, k)] + [-self.getH(i, j), self.getP(j, b), self.getP(i, b)]
                            self.add_clause(clause)
                            clause = [-self.getP(i, b - l) for l in range(1, k)] + [-self.getH(i, j), -self.getP(j, b), -self.getP(i, b)]
                            self.add_clause(clause)
                            b += 1
                        else:
                            break
                        k += 1

                    # Modulo 3 transition: y0 = x1, y1 = x0 ^ x1
                    if (cycle % 3) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 1), -self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                        b += 2

                    # Modulo 5 transition: y0 = not (x0 or x2), y1 = x0 ^ x1, y2 = x0 and x1
                    if (cycle % 5) == 0:
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), -self.getP(i, b)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), -self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b), self.getP(i, b), self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b+1), -self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 2), self.getP(i, b)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 2), self.getP(i, b + 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 2), -self.getP(i, b), -self.getP(i, b + 1)])
                        b += 3

                    # Modulo 7 transition: y0 = x2, y1 = x0, y2 = x1 ^ x2
                    if (cycle % 7) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 1), -self.getP(i, b)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 1), self.getP(i, b)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 2), self.getP(i, b + 1), -self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 2), -self.getP(i, b + 1), self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 2), self.getP(i, b + 1), self.getP(i, b + 2)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 2), -self.getP(i, b + 1), -self.getP(i, b + 2)])
                        b += 3

                    # Modulo 511 transition (LFSR 9 bits, xor 4)
                    if (cycle % 511) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + 8)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + 8)])
                        for idx in range(1, 5):
                            self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 5), self.getP(i, b + 4), -self.getP(i, b + 8)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 5), -self.getP(i, b + 4), self.getP(i, b + 8)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 5), self.getP(i, b + 4), self.getP(i, b + 8)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 5), -self.getP(i, b + 4), -self.getP(i, b + 8)])
                        for idx in range(6, 9):
                            self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        b += 9

                    # Modulo 1023 transition (LFSR 10 bits, xor 3)
                    if (cycle % 1023) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + 9)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + 9)])
                        for idx in range(1, 7):
                            self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 7), self.getP(i, b + 6), -self.getP(i, b + 9)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 7), -self.getP(i, b + 6), self.getP(i, b + 9)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 7), self.getP(i, b + 6), self.getP(i, b + 9)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 7), -self.getP(i, b + 6), -self.getP(i, b + 9)])
                        for idx in range(8, 10):
                            self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        b += 10

                    # Modulo 2047 transition (LFSR 11 bits, xor 2)
                    if (cycle % 2047) == 0:
                        self.add_clause([-self.getH(i, j), self.getP(j, b), -self.getP(i, b + 10)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b), self.getP(i, b + 10)])
                        for idx in range(1, 9):
                            self.add_clause([-self.getH(i, j), self.getP(j, b + idx), -self.getP(i, b + idx - 1)])
                            self.add_clause([-self.getH(i, j), -self.getP(j, b + idx), self.getP(i, b + idx - 1)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 9), self.getP(i, b + 8), -self.getP(i, b + 10)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 9), -self.getP(i, b + 8), self.getP(i, b + 10)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 9), self.getP(i, b + 8), self.getP(i, b + 10)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 9), -self.getP(i, b + 8), -self.getP(i, b + 10)])
                        self.add_clause([-self.getH(i, j), self.getP(j, b + 10), -self.getP(i, b + 9)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b + 10), self.getP(i, b + 9)])
                        b += 11

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        """Main method to construct SAT variables and clauses representing CRT encoding"""
        self.context = context
        n = graph.v
        first = graph.start_vertex

        cycle, nBits = self.get_cycle_and_nbits(n)

        # 1. Non-edges are set to False
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])

        # 2. Exactly one outgoing edge
        for i in range(1, n + 1):
            literals = []
            for j in range(1, n + 1):
                if j in graph.graph[i]:
                    literals.append(self.getH(i, j))
            self.exactly_one_constraint(literals)

        # 3. Exactly one incoming edge
        for j in range(1, n + 1):
            literals = []
            for i in range(1, n + 1):
                if j in graph.graph[i]:
                    literals.append(self.getH(i, j))
            self.exactly_one_constraint(literals)

        # 4. Enforce non-zero representation
        self.enforce_non_zero_lfsr(n, cycle, nBits)

        # 5. Connect one neighbor back to start (final edge)
        first_neighbors = []
        for u in range(1, n + 1):
            if first in graph.graph[u]:
                first_neighbors.append(u)

        self.add_clause([self.getH(neighbor, first) for neighbor in first_neighbors])

        # 6. Symmetry breaking
        if not graph.is_directed:
            for i in range(len(first_neighbors)):
                clause = [self.getH(first, first_neighbors[j]) for j in range(i)]
                clause.append(-self.getH(first_neighbors[i], first))
                self.add_clause(clause)

        # 7. Initialize starting position
        self.init_starting_position(first, cycle)

        # 8. Initialize termination position
        self.init_termination_position(n, first, first_neighbors, cycle)

        # 9. Add transitions
        self.add_transitions(n, graph, first, cycle)

        return self.context
