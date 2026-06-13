"""
Hybrid encoding for cardinality-one constraints (At-Least-One, At-Most-One, and Exactly-One).

It dynamically selects the best At-Most-One encoding strategy based on input size:
- n <= 4: Pairwise (PW) encoding.
- 4 < n <= threshold: Binary Split (BS) encoding.
- n > threshold: Product (PD) encoding.

This implementation adapts EOHybrid from reference into the project's polymorphic OOP design.
"""

import math
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class HybridEncoder(CardinalityOneEncoder):
    def __init__(self, threshold: int = 32):
        """
        Args:
            threshold: Size threshold above which Product (PD) encoding is used (default: 32).
        """
        self.threshold = threshold

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def _pw(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using Pairwise (binomial) method."""
        n = len(variables)
        for i in range(n):
            for j in range(i + 1, n):
                context.add_clause([-variables[i], -variables[j]])

    def _bs(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using Binary Split (BS) method."""
        n = len(variables)
        if n <= 1:
            return
        
        m = n // 2
        t = context.new_var()
        
        for i in range(n):
            val = t if i < m else -t
            context.add_clause([-variables[i], val])

        # Recurse left partition
        left = variables[:m]
        if m <= 4:
            self._pw(context, left)
        else:
            self._bs(context, left)

        # Recurse right partition
        right = variables[m:]
        if n - m <= 4:
            self._pw(context, right)
        else:
            self._bs(context, right)

    def _pd(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using Product (PD) method."""
        n = len(variables)
        if n <= 1:
            return

        m = math.ceil(math.sqrt(n))
        r = context.new_vars(m)
        c = context.new_vars(m)

        # Map variables onto grid representation
        for i in range(n):
            context.add_clause([-variables[i], r[i // m]])
            context.add_clause([-variables[i], c[i % m]])

        # Enforce At-Most-One on rows and columns
        if m <= self.threshold:
            self._bs(context, r)
            self._bs(context, c)
        else:
            self._pd(context, r)
            self._pd(context, c)

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint on the given variables using the Hybrid strategy.
        """
        n = len(variables)
        if n <= 1:
            return

        if n > self.threshold:
            self._pd(context, variables)
        elif n > 4:
            self._bs(context, variables)
        else:
            self._pw(context, variables)
