"""
Binary (logarithmic) encoding for cardinality-one constraints
(At-Least-One, At-Most-One, and Exactly-One).

Binary encoding uses ceil(log2(n)) auxiliary variables to assign a unique binary code
to each variable. If a variable is True, it forces the auxiliary variables to match
its binary code.
Number of clauses: n * ceil(log2(n)).
"""

import math
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class BinaryEncoder(CardinalityOneEncoder):
    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def _get_bit(self, mask: int, pos: int) -> int:
        """Returns the bit at the given position in the mask (0 or 1)."""
        return (mask >> pos) & 1

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using binary (logarithmic) encoding.
        """
        n = len(variables)
        if n <= 1:
            return

        num_aux = math.ceil(math.log2(n))
        aux_vars = context.new_vars(num_aux)

        for i, var in enumerate(variables):
            for j, aux_var in enumerate(aux_vars):
                bit = self._get_bit(i, j)
                if bit == 1:
                    context.add_clause([-var, aux_var])
                else:
                    context.add_clause([-var, -aux_var])
