"""Normalized Sequential Counter (NSC) encoding for cardinality-one constraints.

Optimized for At-Least-One (ALO), At-Most-One (AMO), and Exactly-One (EO).
"""

from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class NSCEncoding(CardinalityOneEncoder):
    """Normalized Sequential Counter encoder for cardinality-one constraints."""

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Least-One constraint using a single clause."""
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint."""
        n = len(variables)
        if n <= 1:
            return

        # Tối ưu cho n nhỏ bằng cách dùng Pairwise (Binomial) encoding để không tốn biến phụ
        if n <= 3:
            for i in range(n):
                for j in range(i + 1, n):
                    context.add_clause([-variables[i], -variables[j]])
            return

        # Dùng Sequential Counter (NSC với k=1) cho At-Most-One
        R = [context.new_var() for _ in range(n - 1)]

        # (1) X_i -> R_{i,1}
        for i in range(n - 1):
            context.add_clause([-variables[i], R[i]])

        # (2) R_{i-1,1} -> R_{i,1}
        for i in range(1, n - 1):
            context.add_clause([-R[i - 1], R[i]])

        # (8) X_i -> -R_{i-1,1}
        for i in range(1, n):
            context.add_clause([-variables[i], -R[i - 1]])


# Giữ các alias này để các cell cũ dùng NSCEncoder hoặc SCLEncoder vẫn chạy được.
NSCEncoder = NSCEncoding
SCLEncoder = NSCEncoding
