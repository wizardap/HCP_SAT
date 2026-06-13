"""
Heule (nested/tree) encoding for cardinality-one constraints
(At-Least-One, At-Most-One, and Exactly-One).

This encoding is based on the recursive nested at-most-one method proposed by Marijn Heule,
which processes variables in groups of 3 and uses auxiliary variables to reduce the size.
Number of clauses: ~ 3n.
Number of variables: ~ n/2 auxiliary variables.
"""

from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.utils.context import EncodingContext


class HeuleEncoder(CardinalityOneEncoder):
    """
    Heule (nested/tree) encoder for cardinality-one constraints.
    Refers to the atmostone implementation in encode.py (from ChienseRemainderConvert).
    """

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        """
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint using Heule's nested method.
        """
        self._heule_amo(context, list(variables))

    def _heule_amo(self, context: EncodingContext, array: list[int]) -> None:
        size = len(array)
        if size <= 1:
            return

        if size > 1:
            context.add_clause([-array[0], -array[1]])
        
        if size > 2:
            context.add_clause([-array[0], -array[2]])
            context.add_clause([-array[1], -array[2]])
        
        if size == 4:
            context.add_clause([-array[0], -array[3]])
            context.add_clause([-array[1], -array[3]])
            context.add_clause([-array[2], -array[3]])
        
        if size > 4:
            # Create an auxiliary variable
            max_var = context.new_var()
            context.add_clause([-array[0], max_var])
            context.add_clause([-array[1], max_var])
            context.add_clause([-array[2], max_var])
            
            # Form the recursive array: replace first 3 variables with the auxiliary variable max_var
            # The remaining elements start from index 3
            array_copy = array[3:] + [max_var]
            
            self._heule_amo(context, array_copy)
