from abc import ABC, abstractmethod
from libs.utils.context import EncodingContext


class CardinalityOneEncoder(ABC):
    """
    Abstract base class defining the polymorphic interface for cardinality-one encodings
    (At-Least-One, At-Most-One, and Exactly-One).
    """

    @abstractmethod
    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Least-One (ALO) constraint on the given variables.
        Must be implemented by subclasses.
        """
        pass

    @abstractmethod
    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the At-Most-One (AMO) constraint on the given variables.
        Must be implemented by subclasses.
        """
        pass

    def encode_eo(self, context: EncodingContext, variables: list[int]) -> None:
        """
        Encodes the Exactly-One (EO) constraint on the given variables.
        Can be overridden by subclasses if they have optimized implementations.
        """
        n = len(variables)
        if n == 0:
            context.add_clause([])
            return
        elif n == 1:
            context.add_clause([variables[0]])
            return

        # At-Least-One
        self.encode_alo(context, variables)

        # At-Most-One
        self.encode_amo(context, variables)
