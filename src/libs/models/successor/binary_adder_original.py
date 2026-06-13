import math
from libs.models.successor.base import SuccessorMethod
from libs.utils.common import find_distance, find_distance_to_first
from libs.utils.context import EncodingContext
from libs.encodings.cardinality_one.heule import HeuleEncoder


class BinaryAdderOriginal(SuccessorMethod):
    def __init__(self, encoder=None):
        if encoder is None:
            encoder = HeuleEncoder()
        super().__init__(encoder)
        self.H = {}
        self.P = {}

    def set_encoder(self, encoder):
        from libs.encodings.cardinality_one.pblib import PbLibEncoder
        # If PbLibEncoder is passed, override with HeuleEncoder as it is not used in original
        if encoder is None or isinstance(encoder, PbLibEncoder):
            self.encoder = HeuleEncoder()
        else:
            self.encoder = encoder

    def getH(self, i, j):
        if (i, j) not in self.H:
            self.H[(i, j)] = self.new_var()
        return self.H[(i, j)]

    def getP(self, i, bit):  # P(i, bit) = 1 if the bit-th bit of the Pi is 1
        if (i, bit) not in self.P:
            self.P[(i, bit)] = self.new_var()
        return self.P[(i, bit)]

    def preprocessing(self, graph, m):
        n = graph.v
        start_v = graph.start_vertex
        distance = find_distance(graph.graph, start_v)
        distance_to_first = distance if not graph.is_directed else find_distance_to_first(graph.graph, start_v)

        for vertex in range(1, n + 1):
            if vertex == start_v:
                continue

            # Pi != t if the minimum distance from vertex 1 to vertex i is greater than t
            for t in range(1, distance[vertex] + 1):
                binaryT = bin(t)[2:].zfill(m)[::-1]  # convert t to binary
                self.add_clause([-self.getP(vertex, bit) if binaryT[bit] == '1' else self.getP(vertex, bit) for bit in range(m)])

            # Pi != t if the minimum distance from vertex i to vertex 1 is greater than n-t
            for t in range(n - distance_to_first[vertex] + 2, n + 1):
                binaryT = bin(t)[2:].zfill(m)[::-1]
                self.add_clause([-self.getP(vertex, bit) if binaryT[bit] == '1' else self.getP(vertex, bit) for bit in range(m)])

        # If start vertex of undirected graph has exactly 2 neighbors, neighbor positions can't be 3, 4, ..., n-1
        if len(graph.graph[start_v]) == 2 and not graph.is_directed:
            for neighbor in graph.graph[start_v]:
                for t in range(3, n):
                    binaryT = bin(t)[2:].zfill(m)[::-1]
                    self.add_clause([-self.getP(neighbor, bit) if binaryT[bit] == '1' else self.getP(neighbor, bit) for bit in range(m)])

    def add_default_variables(self, n, m, graph):
        # Hij = 0 if the arc (i, j) is not in the graph
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])

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

    def vertex_start(self, n, m, graph):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            if i == start_v:
                continue
            for bit in range(m):
                if bit == 1:
                    self.add_clause([-self.getH(start_v, i), self.getP(i, bit)])
                else:
                    self.add_clause([-self.getH(start_v, i), -self.getP(i, bit)])

    def vertex_end(self, n, m, graph):
        start_v = graph.start_vertex
        binaryN = bin(n)[2:][::-1]
        for i in range(1, n + 1):
            if i == start_v:
                continue
            for bit in range(m):
                if binaryN[bit] == '1':
                    self.add_clause([-self.getH(i, start_v), self.getP(i, bit)])
                else:
                    self.add_clause([-self.getH(i, start_v), -self.getP(i, bit)])

    def vertex_positions_final(self, n, m, graph):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if i == start_v or j == start_v or i == j:
                    continue

                # bit == 0: # y0 = -x0
                self.add_clause([-self.getH(i, j), self.getP(i, 0), self.getP(j, 0)])
                self.add_clause([-self.getH(i, j), -self.getP(i, 0), -self.getP(j, 0)])
                # bit == 1: # x0 -> y1 = -x1 and -x0 -> y1 = x1
                self.add_clause([-self.getH(i, j), -self.getP(i, 0), self.getP(i, 1), self.getP(j, 1)])
                self.add_clause([-self.getH(i, j), -self.getP(i, 0), -self.getP(i, 1), -self.getP(j, 1)])
                self.add_clause([-self.getH(i, j), self.getP(i, 0), self.getP(i, 1), -self.getP(j, 1)])
                self.add_clause([-self.getH(i, j), self.getP(i, 0), -self.getP(i, 1), self.getP(j, 1)])

                # 2-bit incrementor
                for bit in range(2, m - 4, 2):
                    # ¬Yi−1 ∧ Xi−1 ⇒ Yi = ¬Xi
                    self.add_clause([-self.getH(i, j), self.getP(j, bit - 1), -self.getP(i, bit - 1), self.getP(j, bit), self.getP(i, bit)])

                    # ¬Yi−1 ∧ Xi−1 ∧ Xi ⇒ Yi+1 = ¬Xi+1
                    self.add_clause([-self.getH(i, j), self.getP(j, bit - 1), -self.getP(i, bit - 1), -self.getP(i, bit), self.getP(j, bit + 1), self.getP(i, bit + 1)])
                    self.add_clause([-self.getH(i, j), self.getP(j, bit - 1), -self.getP(i, bit - 1), -self.getP(i, bit), -self.getP(j, bit + 1), -self.getP(i, bit + 1)])

                    # otherwise: Yi-1 -> Yi = Xi
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit - 1), -self.getP(i, bit), self.getP(j, bit)])
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit - 1), self.getP(i, bit), -self.getP(j, bit)])

                    # otherwise: -Xi-1 -> Yi = Xi
                    self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(i, bit), self.getP(j, bit)])
                    self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), self.getP(i, bit), -self.getP(j, bit)])

                    # special: -Xi -> Yi+1 = Xi+1
                    self.add_clause([-self.getH(i, j), self.getP(i, bit), self.getP(j, bit + 1), -self.getP(i, bit + 1)])
                    self.add_clause([-self.getH(i, j), self.getP(i, bit), -self.getP(j, bit + 1), self.getP(i, bit + 1)])

                    # special: Yi -> Yi+1 = Xi+1
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit), self.getP(j, bit + 1), -self.getP(i, bit + 1)])
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit), -self.getP(j, bit + 1), self.getP(i, bit + 1)])

                # the last bit if remain using 1-bit incrementor
                if m % 2 == 1:
                    bit = m - 5
                    # carry = 1, -Yi-1 ^ Xi-1 -> Yi = -Xi
                    self.add_clause([-self.getH(i, j), self.getP(j, bit - 1), -self.getP(i, bit - 1), self.getP(j, bit), self.getP(i, bit)])
                    self.add_clause([-self.getH(i, j), self.getP(j, bit - 1), -self.getP(i, bit - 1), -self.getP(j, bit), -self.getP(i, bit)])
                    # carry = 0, -Xi-1 -> Yi = Xi
                    self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), self.getP(j, bit), -self.getP(i, bit)])
                    self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(j, bit), self.getP(i, bit)])
                    # carry = 0, Yi-1 -> Yi = Xi
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit - 1), self.getP(j, bit), -self.getP(i, bit)])
                    self.add_clause([-self.getH(i, j), -self.getP(j, bit - 1), -self.getP(j, bit), self.getP(i, bit)])

                # top 4 bits
                bit = m
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), self.getP(i, bit - 4), -self.getP(j, bit - 1)])  # 1
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(j, bit - 1), -self.getP(j, bit - 2)])  # 2
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(j, bit - 1), -self.getP(j, bit - 3)])  # 3
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(j, bit - 1), -self.getP(j, bit - 4)])  # 4
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 3), -self.getP(j, bit - 3), -self.getP(j, bit - 4)])  # 5
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 4), -self.getP(j, bit - 5), self.getP(j, bit - 4)])  # 6
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 2), -self.getP(i, bit - 3), self.getP(j, bit - 2), self.getP(j, bit - 3)])  # 7
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 4), self.getP(i, bit - 5), -self.getP(j, bit - 4)])  # 8
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 1), self.getP(j, bit - 1)])  # 9
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 2), -self.getP(i, bit - 3), -self.getP(j, bit - 2), self.getP(j, bit - 3)])  # 10
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 3), -self.getP(i, bit - 4), -self.getP(i, bit - 5), self.getP(j, bit - 5), -self.getP(j, bit - 3)])  # 11
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 2), self.getP(i, bit - 4), -self.getP(j, bit - 2)])  # 12
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 3), self.getP(i, bit - 4), -self.getP(j, bit - 3)])  # 13
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 2), -self.getP(j, bit - 2), -self.getP(j, bit - 3)])  # 14
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 2), -self.getP(j, bit - 2), -self.getP(j, bit - 4)])  # 15
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 4), self.getP(i, bit - 5), self.getP(j, bit - 4)])  # 16
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 1), -self.getP(i, bit - 2), self.getP(j, bit - 1), self.getP(j, bit - 2)])  # 17
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 4), -self.getP(i, bit - 5), self.getP(j, bit - 5), self.getP(j, bit - 4)])  # 18
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 4), -self.getP(j, bit - 5), -self.getP(j, bit - 4)])  # 19
                self.add_clause([-self.getH(i, j), -self.getP(i, bit - 1), -self.getP(i, bit - 2), -self.getP(j, bit - 1), self.getP(j, bit - 2)])  # 20
                self.add_clause([-self.getH(i, j), self.getP(i, bit - 3), -self.getP(i, bit - 4), -self.getP(i, bit - 5), self.getP(j, bit - 5), self.getP(j, bit - 3)])  # 21

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v
        m = math.ceil(math.log2(n)) if math.log2(n) != int(math.log2(n)) else int(math.log2(n)) + 1

        if self.is_preprocessing:
            self.preprocessing(graph, m)

        self.add_default_variables(n, m, graph)
        self.vertex_outgoing_arcs(n)
        self.vertex_incoming_arcs(n)

        # Original version does not have symmetry breaking
        # Original version does not have preprocessing_v2

        self.vertex_start(n, m, graph)
        self.vertex_end(n, m, graph)
        self.vertex_positions_final(n, m, graph)

        return self.context
