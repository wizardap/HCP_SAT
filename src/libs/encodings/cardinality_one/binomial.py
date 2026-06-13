"""
Binomial (naive/quadratic) encoding for cardinality-one constraints
(At-Least-One, At-Most-One, and Exactly-One).

Binomial encoding does not introduce any auxiliary variables. For At-Most-One, it
adds a clause for every pair of variables to prevent them from being simultaneously True.
Number of clauses: n * (n - 1) / 2.
"""

from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class BinomialEncoder(CardinalityOneEncoder):
    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using pairwise (binomial) encoding.
        """
        n = len(variables)
        for i in range(n):
            for j in range(i + 1, n):
                context.add_clause([-variables[i], -variables[j]])
