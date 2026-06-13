import math
from libs.models.successor.base import SuccessorMethod
from libs.utils.common import find_distance, find_distance_to_first
from libs.utils.context import EncodingContext


class BinaryAdder(SuccessorMethod):
    def __init__(self, encoder=None):
        super().__init__(encoder)
        self.H = {}
        self.P = {}

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

    def preprocessing_v2(self, graph, m):
        n = graph.v
        start_v = graph.start_vertex
        distance = find_distance(graph.graph, start_v)
        distance_to_first = distance if not graph.is_directed else find_distance_to_first(graph.graph, start_v)

        for vertex in range(1, n + 1):
            if vertex == start_v:
                continue

            # Pi >= t+1 if the minimum distance from vertex 1 to vertex i is t
            d_v = distance[vertex]
            binaryD = bin(d_v + 1)[2:].zfill(m)[::-1]  # convert d to binary
            for bit in range(m):
                if binaryD[bit] == '1':
                    self.add_clause([self.getP(vertex, bit)] +
                                     [self.getP(vertex, i) if binaryD[i] == '0' else -self.getP(vertex, i) for i in range(bit + 1, m)])

            # Pi <= n-t+1 if the minimun distance from vertex i to vertex 1 is t
            d_v_to_first = distance_to_first[vertex]
            binaryD = bin(n - d_v_to_first + 1)[2:].zfill(m)[::-1]
            for bit in range(m):
                if binaryD[bit] == '0':
                    self.add_clause([-self.getP(vertex, bit)] +
                                     [self.getP(vertex, i) if binaryD[i] == '0' else -self.getP(vertex, i) for i in range(bit + 1, m)])

        # If start vertex of undirected graph has exactly 2 neighbors, neighbor positions can't be 3, 4, ..., n-1
        if len(graph.graph[start_v]) == 2 and not graph.is_directed:
            # Get the two neighbors
            neighbors = list(graph.graph[start_v])
            n1, n2 = neighbors[0], neighbors[1]

            binary_two = bin(2)[2:].zfill(m)[::-1]
            binary_n = bin(n)[2:].zfill(m)[::-1]

            # Encode that either:
            # (n1 is at position 2 AND n2 is at position n) OR (n1 is at position n AND n2 is at position 2)
            # (case1_n1 AND case1_n2) OR (case2_n1 AND case2_n2)

            # Case 1: n1 is at position 2
            case1_n1 = []
            for bit in range(m):
                if binary_two[bit] == '1':
                    case1_n1.append(self.getP(n1, bit))
                else:
                    case1_n1.append(-self.getP(n1, bit))
            # Case 1: n2 is at position n
            case1_n2 = []
            for bit in range(m):
                if binary_n[bit] == '1':
                    case1_n2.append(self.getP(n2, bit))
                else:
                    case1_n2.append(-self.getP(n2, bit))

            # Case 2: n2 is at position 2
            case2_n2 = []
            for bit in range(m):
                if binary_two[bit] == '1':
                    case2_n2.append(self.getP(n2, bit))
                else:
                    case2_n2.append(-self.getP(n2, bit))
            # Case 2: n1 is at position n
            case2_n1 = []
            for bit in range(m):
                if binary_n[bit] == '1':
                    case2_n1.append(self.getP(n1, bit))
                else:
                    case2_n1.append(-self.getP(n1, bit))

            self.add_clause(case1_n1 + case2_n1)
            self.add_clause(case1_n1 + case2_n2)
            self.add_clause(case1_n2 + case2_n1)
            self.add_clause(case1_n2 + case2_n2)

    def add_default_variables(self, n, m, graph):
        # Hij = 0 if the arc (i, j) is not in the graph
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])

        # P_start = 1 (00..01)
        start_v = graph.start_vertex
        for bit in range(m):
            if bit == 0:
                self.add_clause([self.getP(start_v, bit)])
            else:
                self.add_clause([-self.getP(start_v, bit)])

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

    def symmetry_breaking(self, graph, n):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            clause = [-self.getH(i, start_v)]
            for j in range(1, i):
                clause.append(self.getH(start_v, j))
            self.add_clause(clause)

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
            self.preprocessing_v2(graph, m)

        self.add_default_variables(n, m, graph)
        self.vertex_outgoing_arcs(n)
        self.vertex_incoming_arcs(n)

        if not graph.is_directed:
            self.symmetry_breaking(graph, n)

        self.vertex_start(n, m, graph)
        self.vertex_end(n, m, graph)
        self.vertex_positions_final(n, m, graph)

        return self.context
