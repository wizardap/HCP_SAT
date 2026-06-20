class Graph:
    def __init__(self):
        self.v = None
        self.graph = None
        self.is_directed = False
        self.start_vertex = 1   # default start vertex

    def add_edge(self, u, v):
        self.graph[u].append(v)

    def set_graph(self, graph):
        self.graph = graph

    def set_is_directed(self, is_directed):
        self.is_directed = is_directed

    def set_start_vertex(self, start_vertex):
        self.start_vertex = start_vertex

    def load_graph_from_file(self, url):
        self.filename = url
        # Normalize slashes to handle cross-platform file paths
        normalized_url = url.replace("\\", "/")
        parts = normalized_url.split('/')
        test_set = parts[-2] if len(parts) >= 2 else ""

        if test_set in ["vset", "graphs"]:
            graph = {}
            with open(url) as f:
                for line in f:
                    if line.startswith("p"):
                        n = int(line.split()[2])
                        graph = {i: [] for i in range(1, n + 1)}
                    else:
                        if line.startswith("e"):
                            u, v = map(int, line.split()[1:])
                        else:
                            u, v = map(int, line.split())
                        if v not in graph[u]:
                            graph[u].append(v)
                        if test_set == "graphs":
                            if u not in graph[v]:
                                graph[v].append(u)
            self.graph = graph
            self.v = len(graph)
            if test_set == "graphs":
                self.is_directed = False
                self.start_vertex = min(graph, key=lambda vertex: len(graph[vertex]))
            else:
                self.is_directed = True

        if test_set in ['fhcpcs', 'fhcpsl', 'fhcppp']:
            graph = {}
            with open(url) as f:
                for line in f:
                    if line.startswith("DIMENSION"):
                        n = int(line.split(':')[1])
                        graph = {i: [] for i in range(1, n + 1)}
                    else:
                        # if line start with number differ -1
                        if line[0].isdigit():
                            u, v = map(int, line.split())
                            if v not in graph[u]:
                                graph[u].append(v)
                            if u not in graph[v]:
                                graph[v].append(u)
            self.graph = graph
            self.v = len(graph)
            # set the first vertex with the minimum degree
            self.start_vertex = min(graph, key=lambda vertex: len(graph[vertex]))

        if test_set == 'tsphcp':
            graph = {}
            with open(url) as f:
                for line in f:
                    if line.startswith("D"):
                        n = int(line.split(':')[1])
                        graph = {i: [] for i in range(1, n + 1)}
                    else:
                        if line.strip()[0].isdigit() and int(line.strip()[0]) != -1:
                            u, v = map(int, line.split())
                            if v not in graph[u]:
                                graph[u].append(v)
            self.graph = graph
            self.v = len(graph)
            self.start_vertex = min(graph, key=lambda vertex: len(graph[vertex]))
