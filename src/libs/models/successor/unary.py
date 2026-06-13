from libs.models.successor.base import SuccessorMethod
from libs.utils.common import find_distance, find_distance_to_first
from libs.utils.context import EncodingContext


class Unary(SuccessorMethod):
    def __init__(self, encoder=None):
        super().__init__(encoder)
        self.H = {}
        self.U = {}

    def getH(self, i, j):
        if (i, j) not in self.H:
            self.H[(i, j)] = self.new_var()
        return self.H[(i, j)]

    def getU(self, i, j):
        if (i, j) not in self.U:
            self.U[(i, j)] = self.new_var()
        return self.U[(i, j)]

    def preprocessing(self, graph):
        n = graph.v
        start_v = graph.start_vertex
        distance = find_distance(graph.graph, start_v)
        distance_to_first = distance if not graph.is_directed else find_distance_to_first(graph.graph, start_v)

        for vertex in range(1, n + 1):
            if vertex == start_v:
                continue

            # U_it = 0 if the minimum distance from vertex 1 to vertex i is greater than t
            for t in range(1, distance[vertex] + 1):
                self.add_clause([-self.getU(vertex, t)])

            # U_it = 0 if the minimum distance from vertex i to vertex 1 is greater than n-t
            for t in range(n - distance_to_first[vertex] + 2, n + 1):
                self.add_clause([-self.getU(vertex, t)])

    def add_default_variables(self, n, graph):
        # Hij = 0 if the arc (i, j) is not in the graph
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if j not in graph.graph[i]:
                    self.add_clause([-self.getH(i, j)])
        # U_start_1 is initialized to True
        start_v = graph.start_vertex
        self.add_clause([self.getU(start_v, 1)])

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

    def vertex_start(self, n, graph):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            if i == start_v:
                continue
            self.add_clause([-self.getH(start_v, i), self.getU(i, 2)])

    def vertex_end(self, n, graph):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            if i == start_v:
                continue
            self.add_clause([-self.getH(i, start_v), self.getU(i, n)])

    def vertex_positions(self, n, graph):
        start_v = graph.start_vertex
        for i in range(1, n + 1):
            for j in range(1, n + 1):
                if i != start_v and j != start_v and i != j:
                    for p in range(2, n):
                        self.add_clause([-self.getH(i, j), -self.getU(i, p), self.getU(j, p + 1)])

    def vertex_EO_positions(self, n):
        for i in range(1, n + 1):
            literals = [self.getU(i, p) for p in range(1, n + 1)]
            self.exactly_one_constraint(literals)

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        self.context = context
        n = graph.v

        if self.is_preprocessing:
            self.preprocessing(graph)

        self.add_default_variables(n, graph)
        self.vertex_outgoing_arcs(n)
        self.vertex_incoming_arcs(n)
        self.vertex_start(n, graph)
        self.vertex_end(n, graph)
        self.vertex_positions(n, graph)
        self.vertex_EO_positions(n)
        return self.context
