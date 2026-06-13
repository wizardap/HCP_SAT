"""
Commander encoding for cardinality-one constraints (At-Least-One, At-Most-One, and Exactly-One).

Commander encoding partitions the variables into groups and assigns a "commander"
variable to each group. It recursively enforces:
1. At most one variable in each group is True.
2. The commander of a group is True if and only if exactly one variable in the group is True.
3. Recursively, exactly one (or at most one) of the commander variables is True.

Reference:
Will Klieber and Gihwon Kwon. "Efficient CNF Encoding for Selecting 1 from N Objects", 2007.
"""

import math
from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class CommanderEncoder(CardinalityOneEncoder):
    def __init__(self, limit: int = 6):
        """
        Args:
            limit: The maximum size of a group before recursive partitioning is applied (default: 6).
        """
        self.limit = limit

    def _naive_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes At-Most-One constraint using the naive (quadratic) method."""
        n = len(variables)
        for i in range(n):
            for j in range(i + 1, n):
                context.add_clause([-variables[i], -variables[j]])

    def _naive_eo(self, context: EncodingContext, variables: list[int]) -> None:
        """Encodes Exactly-One constraint using the naive method."""
        if len(variables) > 0:
            context.add_clause(list(variables))
        self._naive_amo(context, variables)

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using recursive Commander encoding.
        """
        n = len(variables)
        if n <= 1:
            return
        if n <= self.limit:
            self._naive_amo(context, variables)
            return

        # Determine group size g = ceil(sqrt(n))
        g = math.ceil(math.sqrt(n))

        # Partition variables into groups of size at most g
        groups = [variables[i : i + g] for i in range(0, n, g)]
        num_groups = len(groups)

        # Allocate commander variables for the groups
        commander_vars = context.new_vars(num_groups)

        for idx, group in enumerate(groups):
            commander = commander_vars[idx]

            # 1. At most one variable in the group can be True
            self._naive_amo(context, group)

            # 2. If the commander is False, none of the variables in the group can be True (x -> c)
            for x in group:
                context.add_clause([commander, -x])

        # 3. Recursively enforce At-Most-One on the commander variables
        self.encode_amo(context, commander_vars)

    def encode_eo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the Exactly-One (EO) constraint using recursive Commander encoding.
        """
        n = len(variables)
        if n == 0:
            context.add_clause([])
            return
        if n == 1:
            context.add_clause([variables[0]])
            return
        if n <= self.limit:
            self._naive_eo(context, variables)
            return

        # Determine group size g = ceil(sqrt(n))
        g = math.ceil(math.sqrt(n))

        # Partition variables into groups of size at most g
        groups = [variables[i : i + g] for i in range(0, n, g)]
        num_groups = len(groups)

        # Allocate commander variables for the groups
        commander_vars = context.new_vars(num_groups)

        for idx, group in enumerate(groups):
            commander = commander_vars[idx]

            # 1. At most one variable in the group can be True
            self._naive_amo(context, group)

            # 2. If the commander is True, at least one variable in the group must be True (c -> v1 v v2 v ...)
            context.add_clause([-commander] + list(group))

            # 3. If the commander is False, none of the variables in the group can be True (x -> c)
            for x in group:
                context.add_clause([commander, -x])

        # 4. Recursively enforce Exactly-One on the commander variables
        self.encode_eo(context, commander_vars)
