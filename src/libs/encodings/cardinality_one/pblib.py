"""
PbLib (Pseudo-Boolean Library) encoding wrapper for cardinality-one constraints
(At-Least-One, At-Most-One, and Exactly-One).

This uses the pypblib binding to access optimized C++ AMK/ALK card encodings.
"""

from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext

try:
    from pypblib import pblib
    from pypblib.pblib import PBConfig, Pb2cnf
    PYPBLIB_AVAILABLE = True
except ImportError:
    PYPBLIB_AVAILABLE = False


class PbLibEncoder(CardinalityOneEncoder):
    def __init__(self):
        """
        Initializes the PbLibEncoder.
        Raises ImportError if the pypblib package is not installed.
        """
        if not PYPBLIB_AVAILABLE:
            raise ImportError(
                "pypblib is not installed in the current Python environment. "
                "Please install it using: pip install pypblib"
            )
        
        # Optimize: Instantiate config and encoder once to avoid overhead of C++ object creation on each call
        self.config = PBConfig()
        self.config.set_AMK_Encoder(pblib.AMK_CARD)
        self.pb2 = Pb2cnf(self.config)

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint using a single clause (optimal).
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using pypblib.
        """
        n = len(variables)
        if n <= 1:
            return
        elif n == 2:
            context.add_clause([-variables[0], -variables[1]])
            return
        
        formula = []
        max_var = self.pb2.encode_at_most_k(variables, 1, formula, context.var_pool.top + 1)
        context.extend(formula)
        context.var_pool.top = max_var

    def encode_eo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the Exactly-One (EO) constraint using a single ALO clause and pypblib for AMO.
        """
        n = len(variables)
        if n == 0:
            context.add_clause([])
            return
        elif n == 1:
            context.add_clause([variables[0]])
            return
        elif n == 2:
            context.add_clause([variables[0], variables[1]])
            context.add_clause([-variables[0], -variables[1]])
            return
        
        # 1. At-Least-One (simple single clause)
        context.add_clause(list(variables))
        
        # 2. At-Most-One (via pypblib)
        formula = []
        max_var = self.pb2.encode_at_most_k(variables, 1, formula, context.var_pool.top + 1)
        context.extend(formula)
        context.var_pool.top = max_var
