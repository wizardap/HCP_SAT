"""
Product encoding for cardinality-one constraints (At-Least-One, At-Most-One, and Exactly-One).

Product encoding partitions the variables into a 2D grid of size p x q, where
p = ceil(sqrt(n)) and q = ceil(n/p). It creates two sets of auxiliary variables:
- u of size p (representing rows)
- v of size q (representing columns)

If a variable x_ij is True, it forces u_i and v_j to be True.
It then enforces At-Most-One on u and At-Most-One on v using binary encoding.

Reference:
J. N. Darwiche and Pipatsrisawat. "A Product Encoding for At-Most-One Constraints", 2007.
"""

import math
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class ProductEncoder(CardinalityOneEncoder):
    def __init__(self, limit: int = 10):
        """
        Args:
            limit: The maximum size of variables before applying product encoding (default: 10).
                   If size is below or equal to limit, naive encoding is used.
        """
        self.limit = limit

    def _naive_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using the naive (quadratic) method."""
        n = len(variables)
        for i in range(n):
            for j in range(i + 1, n):
                context.add_clause([-variables[i], -variables[j]])

    def _get_bit(self, mask: int, pos: int) -> int:
        """Returns the bit at the given position in the mask (0 or 1)."""
        return (mask >> pos) & 1

    def _binary_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using binary encoding."""
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

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using Product encoding.
        """
        n = len(variables)
        if n <= 1:
            return
        if n <= self.limit:
            self._naive_amo(context, variables)
            return

        p = math.ceil(math.sqrt(n))
        q = math.ceil(n / p)

        # Generate auxiliary variables u and v
        u = context.new_vars(p)
        v = context.new_vars(q)

        # Enforce at most one on u and v using binary encoding
        self._binary_amo(context, u)
        self._binary_amo(context, v)

        # Map variables to u and v
        n_count = 0
        for j in range(q):
            for i in range(p):
                # variables[n_count] -> u[i] and variables[n_count] -> v[j]
                context.add_clause([-variables[n_count], u[i]])
                context.add_clause([-variables[n_count], v[j]])
                n_count += 1
                if n_count == n:
                    break
            if n_count == n:
                break
