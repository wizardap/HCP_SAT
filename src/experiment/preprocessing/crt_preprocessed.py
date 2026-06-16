from libs.models.successor.base import SuccessorMethod
from libs.models.successor.crt import CRT
from libs.utils.context import EncodingContext
from experiment.preprocessing.preprocessor import HCPPreprocessor

class CRTPreprocessed(SuccessorMethod):
    """
    Experimental preprocessed CRT successor encoding.
    It runs the HCPPreprocessor graph-level preprocessing before building the CRT clauses.
    """
    def __init__(self, encoder=None, cycle=None, enable_probing=True, max_probing_edges=300, enable_contraction=True, enable_2cut=True, crt_class=None):
        super().__init__(encoder)
        self.cycle_override = cycle
        self.enable_probing = enable_probing
        self.max_probing_edges = max_probing_edges
        self.enable_contraction = enable_contraction
        self.enable_2cut = enable_2cut
        if crt_class is None:
            from libs.models.successor.crt import CRT
            self.crt_class = CRT
        else:
            self.crt_class = crt_class
        
        self.H = {}  # maps original (i, j) -> SAT variable
        self._forbidden_var = None
        self._forced_var = None
        self.preprocess_result = None
        self.reduced_crt = None
        self.edge_map = {}
        self.original_graph = None

    def get_forbidden_var(self):
        if self._forbidden_var is None:
            self._forbidden_var = self.new_var()
            self.add_clause([-self._forbidden_var])
        return self._forbidden_var

    def get_forced_var(self):
        if self._forced_var is None:
            self._forced_var = self.new_var()
            self.add_clause([self._forced_var])
        return self._forced_var

    def getH(self, i, j):
        if (i, j) in self.H:
            return self.H[(i, j)]

        # If preprocessor declared UNSAT, all getH return forbidden_var
        if self.preprocess_result is None or self.preprocess_result.is_unsat:
            return self.get_forbidden_var()

        # Check if it's the direct endpoints of a contracted path
        # (This is a virtual edge in the reduced graph, not an edge in the original cycle)
        norm_ij = (i, j) if self.original_graph.is_directed else (min(i, j), max(i, j))
        if norm_ij in self.preprocess_result.contracted_paths:
            var = self.get_forbidden_var()
            self.H[(i, j)] = var
            return var

        # Check contracted path map
        if (i, j) in self.edge_map:
            type_, u_red, v_red = self.edge_map[(i, j)]
            var = self.reduced_crt.getH(u_red, v_red)
            self.H[(i, j)] = var
            return var

        # Check if both vertices are active in the reduced graph
        original_to_reduced = self.preprocess_result.original_to_reduced
        if i in original_to_reduced and j in original_to_reduced:
            i_red = original_to_reduced[i]
            j_red = original_to_reduced[j]
            var = self.reduced_crt.getH(i_red, j_red)
            self.H[(i, j)] = var
            return var

        # Otherwise it's a forbidden/non-existent edge
        var = self.get_forbidden_var()
        self.H[(i, j)] = var
        return var

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        if self.reduced_crt is not None:
            return self.context
            
        self.context = context
        self.original_graph = graph
        
        # 1. Run Preprocessor
        preprocessor = HCPPreprocessor(
            enable_probing=self.enable_probing,
            max_probing_edges=self.max_probing_edges,
            enable_contraction=self.enable_contraction,
            enable_2cut=self.enable_2cut
        )
        self.preprocess_result = preprocessor.preprocess(graph)
        
        print(f"[Preprocessor] Stats: {self.preprocess_result.stats}")
        
        # 2. Check for early UNSAT
        if self.preprocess_result.is_unsat:
            print("[Preprocessor] Detected UNSAT early!")
            self.add_clause([])  # Force UNSAT in solver
            return self.context

        # 3. Initialize edge_map for contracted paths
        original_to_reduced = self.preprocess_result.original_to_reduced
        self.edge_map = {}
        for edge, path in self.preprocess_result.contracted_paths.items():
            u, v = path[0], path[-1]
            u_red = original_to_reduced[u]
            v_red = original_to_reduced[v]
            
            # Map forward edges along the path
            for r in range(len(path) - 1):
                self.edge_map[(path[r], path[r+1])] = ('reduced', u_red, v_red)
                
            # If undirected, map backward edges along the path
            if not graph.is_directed:
                for r in range(len(path) - 1):
                    self.edge_map[(path[r+1], path[r])] = ('reduced', v_red, u_red)

        # 4. Build CRT clauses on the reduced graph
        self.reduced_crt = self.crt_class(encoder=self.encoder, cycle=self.cycle_override)
        self.reduced_crt.context = self.context
        
        # Build clauses on the reduced graph
        self.reduced_crt.build_clauses(self.context, self.preprocess_result.graph)

        # 5. Enforce unit clauses for forced edges in the reduced representation
        # (This forces the variables of forced original edges and contracted edges)
        for u, v in self.preprocess_result.forced_edges:
            if u in original_to_reduced and v in original_to_reduced:
                u_red = original_to_reduced[u]
                v_red = original_to_reduced[v]
                if graph.is_directed:
                    self.add_clause([self.reduced_crt.getH(u_red, v_red)])
                else:
                    self.add_clause([self.reduced_crt.getH(u_red, v_red), self.reduced_crt.getH(v_red, u_red)])

        for edge in self.preprocess_result.contracted_paths:
            u, v = edge
            u_red = original_to_reduced[u]
            v_red = original_to_reduced[v]
            if graph.is_directed:
                self.add_clause([self.reduced_crt.getH(u_red, v_red)])
            else:
                self.add_clause([self.reduced_crt.getH(u_red, v_red), self.reduced_crt.getH(v_red, u_red)])

        return self.context
