"""
Bimander encoding for cardinality-one constraints (At-Least-One, At-Most-One, and Exactly-One)
implemented using polymorphism.

Bimander encoding partitions the variables into 'm' groups, where each group has
at most 'g = ceil(n / m)' variables. It enforces:
1. At most one variable in each group is true (using a naive At-Most-One).
2. At most one group is active/has a true variable (by binary encoding the active
   group index using log2(m) auxiliary variables).

Reference:
Alan M. Frisch and Gianluigi Giannaros. "SAT Encodings of the At-Most-One Constraint", 2010.
"""

import math
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class BimanderEncoder(CardinalityOneEncoder):
    def __init__(self, m: int = None):
        """
        Args:
            m: The number of groups to partition the variables into.
               If None, it is dynamically computed as ceil(n / ceil(sqrt(n))).
        """
        self.m = m

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        In Bimander/standard encoding, this is simply a single clause containing all variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def _get_bit(self, mask: int, pos: int) -> int:
        """Returns the bit at the given position in the mask (0 or 1)."""
        return (mask >> pos) & 1

    def _naive_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using the naive (quadratic) method."""
        n = len(variables)
        for i in range(n):
            for j in range(i + 1, n):
                context.add_clause([-variables[i], -variables[j]])

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using Bimander encoding.

        Args:
            context: The EncodingContext to add variables and clauses to.
            variables: A list of SAT variables/literals (represented as integers).
        """
        n = len(variables)
        if n <= 1:
            return

        # Determine group size and number of groups
        if self.m is None:
            g = math.ceil(math.sqrt(n))
            m_groups = math.ceil(n / g)
        else:
            m_groups = max(1, min(self.m, n))
            g = math.ceil(n / m_groups)

        # If only one group, it is equivalent to naive encoding of the whole list
        if m_groups == 1:
            self._naive_amo(context, variables)
            return

        # Partition variables into m_groups groups
        groups = []
        for i in range(m_groups):
            start = i * g
            end = min(start + g, n)
            if start < end:
                groups.append(variables[start:end])

        num_groups = len(groups)

        # 1. At most one variable in each group can be true
        for group in groups:
            self._naive_amo(context, group)

        # 2. At most one group can be active (at most one group has a true variable)
        # We need ceil(log2(num_groups)) auxiliary variables to encode the active group's index
        num_aux = math.ceil(math.log2(num_groups))
        if num_aux > 0:
            aux_vars = context.new_vars(num_aux)

            for i, group in enumerate(groups):
                for var in group:
                    # If variable `var` in group `i` is true, then the auxiliary variables
                    # must match the binary representation of `i`.
                    # var -> (aux_vars[j] == bit)
                    # which translates to:
                    # If bit is 1: -var v aux_var
                    # If bit is 0: -var v -aux_var
                    for j, aux_var in enumerate(aux_vars):
                        bit = self._get_bit(i, j)
                        if bit == 1:
                            context.add_clause([-var, aux_var])
                        else:
                            context.add_clause([-var, -aux_var])
