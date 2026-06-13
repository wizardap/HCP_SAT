"""
Sequential encoding for cardinality-one constraints (At-Least-One, At-Most-One, and Exactly-One).

Sequential encoding uses n - 1 auxiliary variables s_0, ..., s_{n-2} to represent
a sequential counter of the number of True variables seen so far.
Number of clauses: 3n - 4.

Reference:
Carsten Sinz. "Towards an Optimal SAT Encoding of Boolean Cardinality Constraints", 2005.
"""

from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class SequentialEncoder(CardinalityOneEncoder):
    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using sequential encoding.
        """
        n = len(variables)
        if n <= 1:
            return

        # Generate n - 1 sequential auxiliary variables
        seq_vars = context.new_vars(n - 1)

        # 1. Activate: if variables[i] is True, seq_vars[i] must be True
        for i in range(n - 1):
            context.add_clause([-variables[i], seq_vars[i]])

        # 2. Propagate: if seq_vars[i-1] is True, seq_vars[i] must be True
        for i in range(1, n - 1):
            context.add_clause([-seq_vars[i - 1], seq_vars[i]])

        # 3. Forbid second True: if seq_vars[i-1] is True, variables[i] must be False
        for i in range(1, n):
            context.add_clause([-seq_vars[i - 1], -variables[i]])
