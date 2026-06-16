from libs.models.successor.crt import CRT


class VCCRT(CRT):
    """
    Vertex-Centric Chinese Remainder Theorem (VCCRT) successor encoding.
    Instead of writing modular transition logic for each edge, it precomputes 
    successor state variables S(i, b) locally for each vertex i, and then copies 
    the successor state from i to j if the edge i -> j is active (i.e. H(i, j) is True).
    """
    def __init__(self, encoder=None, cycle=None):
        super().__init__(encoder, cycle)
        self.S = {}  # S[(i, bit)] represents successor position bit variable

    def getS(self, i, bit_idx):
        """Get or create the successor bit variable index for vertex i at bit_idx"""
        if (i, bit_idx) not in self.S:
            self.S[(i, bit_idx)] = self.new_var()
        return self.S[(i, bit_idx)]

    def add_transitions(self, n, graph, first, cycle):
        """
        Define local successor state variables for each vertex, and copy the 
        successor state across active edges.
        """
        # 1. Define S_i = successor(P_i) locally for each vertex i
        for i in range(1, n + 1):
            b = 0
            # Modulo 2 transition: S0 = not P0
            if (cycle % 2) == 0:
                self.add_clause([self.getS(i, b), self.getP(i, b)])
                self.add_clause([-self.getS(i, b), -self.getP(i, b)])
                b += 1

            # Higher powers of 2 transition (binary addition logic)
            k = 2
            while True:
                if (cycle % (1 << k)) == 0:
                    # S_{i, b} <-> P_{i, b} ^ (P_{i, b-1} & ... & P_{i, b-k+1})
                    # If all previous bits are 1, S_{i, b} <-> ~P_{i, b}
                    self.add_clause([-self.getP(i, b - l) for l in range(1, k)] + [self.getS(i, b), self.getP(i, b)])
                    self.add_clause([-self.getP(i, b - l) for l in range(1, k)] + [-self.getS(i, b), -self.getP(i, b)])
                    
                    # If any previous bit is 0, S_{i, b} <-> P_{i, b}
                    for l in range(1, k):
                        self.add_clause([self.getP(i, b - l), self.getS(i, b), -self.getP(i, b)])
                        self.add_clause([self.getP(i, b - l), -self.getS(i, b), self.getP(i, b)])
                    b += 1
                else:
                    break
                k += 1

            # Modulo 3 transition: S0 = P1, S1 = P0 ^ P1
            if (cycle % 3) == 0:
                self.add_clause([self.getS(i, b), -self.getP(i, b + 1)])
                self.add_clause([-self.getS(i, b), self.getP(i, b + 1)])
                
                self.add_clause([self.getS(i, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                self.add_clause([self.getS(i, b + 1), -self.getP(i, b), self.getP(i, b + 1)])
                self.add_clause([-self.getS(i, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                self.add_clause([-self.getS(i, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                b += 2

            # Modulo 5 transition: S0 = not (P0 or P2), S1 = P0 ^ P1, S2 = P0 and P1
            if (cycle % 5) == 0:
                self.add_clause([-self.getS(i, b), -self.getP(i, b)])
                self.add_clause([-self.getS(i, b), -self.getP(i, b + 2)])
                self.add_clause([self.getS(i, b), self.getP(i, b), self.getP(i, b + 2)])
                
                self.add_clause([self.getS(i, b + 1), self.getP(i, b), -self.getP(i, b + 1)])
                self.add_clause([self.getS(i, b + 1), -self.getP(i, b), self.getP(i, b + 1)])
                self.add_clause([-self.getS(i, b + 1), self.getP(i, b), self.getP(i, b + 1)])
                self.add_clause([-self.getS(i, b + 1), -self.getP(i, b), -self.getP(i, b + 1)])
                
                self.add_clause([-self.getS(i, b + 2), self.getP(i, b)])
                self.add_clause([-self.getS(i, b + 2), self.getP(i, b + 1)])
                self.add_clause([self.getS(i, b + 2), -self.getP(i, b), -self.getP(i, b + 1)])
                b += 3

            # Modulo 7 transition: S0 = P2, S1 = P0, S2 = P1 ^ P2
            if (cycle % 7) == 0:
                self.add_clause([self.getS(i, b), -self.getP(i, b + 2)])
                self.add_clause([-self.getS(i, b), self.getP(i, b + 2)])
                
                self.add_clause([self.getS(i, b + 1), -self.getP(i, b)])
                self.add_clause([-self.getS(i, b + 1), self.getP(i, b)])
                
                self.add_clause([self.getS(i, b + 2), self.getP(i, b + 1), -self.getP(i, b + 2)])
                self.add_clause([self.getS(i, b + 2), -self.getP(i, b + 1), self.getP(i, b + 2)])
                self.add_clause([-self.getS(i, b + 2), self.getP(i, b + 1), self.getP(i, b + 2)])
                self.add_clause([-self.getS(i, b + 2), -self.getP(i, b + 1), -self.getP(i, b + 2)])
                b += 3

            # Modulo 511 transition (LFSR 9 bits, xor 4)
            if (cycle % 511) == 0:
                self.add_clause([self.getS(i, b), -self.getP(i, b + 8)])
                self.add_clause([-self.getS(i, b), self.getP(i, b + 8)])
                for idx in range(1, 5):
                    self.add_clause([self.getS(i, b + idx), -self.getP(i, b + idx - 1)])
                    self.add_clause([-self.getS(i, b + idx), self.getP(i, b + idx - 1)])
                self.add_clause([self.getS(i, b + 5), self.getP(i, b + 4), -self.getP(i, b + 8)])
                self.add_clause([self.getS(i, b + 5), -self.getP(i, b + 4), self.getP(i, b + 8)])
                self.add_clause([-self.getS(i, b + 5), self.getP(i, b + 4), self.getP(i, b + 8)])
                self.add_clause([-self.getS(i, b + 5), -self.getP(i, b + 4), -self.getP(i, b + 8)])
                for idx in range(6, 9):
                    self.add_clause([self.getS(i, b + idx), -self.getP(i, b + idx - 1)])
                    self.add_clause([-self.getS(i, b + idx), self.getP(i, b + idx - 1)])
                b += 9

            # Modulo 1023 transition (LFSR 10 bits, xor 3)
            if (cycle % 1023) == 0:
                self.add_clause([self.getS(i, b), -self.getP(i, b + 9)])
                self.add_clause([-self.getS(i, b), self.getP(i, b + 9)])
                for idx in range(1, 7):
                    self.add_clause([self.getS(i, b + idx), -self.getP(i, b + idx - 1)])
                    self.add_clause([-self.getS(i, b + idx), self.getP(i, b + idx - 1)])
                self.add_clause([self.getS(i, b + 7), self.getP(i, b + 6), -self.getP(i, b + 9)])
                self.add_clause([self.getS(i, b + 7), -self.getP(i, b + 6), self.getP(i, b + 9)])
                self.add_clause([-self.getS(i, b + 7), self.getP(i, b + 6), self.getP(i, b + 9)])
                self.add_clause([-self.getS(i, b + 7), -self.getP(i, b + 6), -self.getP(i, b + 9)])
                for idx in range(8, 10):
                    self.add_clause([self.getS(i, b + idx), -self.getP(i, b + idx - 1)])
                    self.add_clause([-self.getS(i, b + idx), self.getP(i, b + idx - 1)])
                b += 10

            # Modulo 2047 transition (LFSR 11 bits, xor 2)
            if (cycle % 2047) == 0:
                self.add_clause([self.getS(i, b), -self.getP(i, b + 10)])
                self.add_clause([-self.getS(i, b), self.getP(i, b + 10)])
                for idx in range(1, 9):
                    self.add_clause([self.getS(i, b + idx), -self.getP(i, b + idx - 1)])
                    self.add_clause([-self.getS(i, b + idx), self.getP(i, b + idx - 1)])
                self.add_clause([self.getS(i, b + 9), self.getP(i, b + 8), -self.getP(i, b + 10)])
                self.add_clause([self.getS(i, b + 9), -self.getP(i, b + 8), self.getP(i, b + 10)])
                self.add_clause([-self.getS(i, b + 9), self.getP(i, b + 8), self.getP(i, b + 10)])
                self.add_clause([-self.getS(i, b + 9), -self.getP(i, b + 8), -self.getP(i, b + 10)])
                self.add_clause([self.getS(i, b + 10), -self.getP(i, b + 9)])
                self.add_clause([-self.getS(i, b + 10), self.getP(i, b + 9)])
                b += 11

        # 2. Copy clauses: if H(i, j) is True (for j != first), then P_j <-> S_i
        # Compute cycle and nBits to know how many bits to copy
        _, nBits = self.get_cycle_and_nbits(n)
        for i in range(1, n + 1):
            for j in graph.graph[i]:
                if j != first:
                    for b_idx in range(nBits):
                        self.add_clause([-self.getH(i, j), self.getP(j, b_idx), -self.getS(i, b_idx)])
                        self.add_clause([-self.getH(i, j), -self.getP(j, b_idx), self.getS(i, b_idx)])
