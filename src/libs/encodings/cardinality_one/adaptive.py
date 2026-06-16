from libs.encodings.cardinality_one.base import CardinalityOneEncoder
from libs.encodings.cardinality_one.binomial import BinomialEncoder
from libs.encodings.cardinality_one.scl import NSCEncoding
from libs.utils.context import EncodingContext

class AdaptiveEncoder(CardinalityOneEncoder):
    """
    Adaptive cardinality encoder. Use Pairwise (Binomial) encoding for small sets
    (number of variables <= threshold) to avoid introducing auxiliary variables,
    and NSC (Normalized Sequential Counter) encoding for larger sets.
    """
    def __init__(self, threshold: int = 5):
        self.threshold = threshold
        self.pairwise_encoder = BinomialEncoder()
        self.nsc_encoder = NSCEncoding()

    def encode_alo(self, context: EncodingContext, variables: list[int]) -> None:
        # At-least-one is always a single clause containing all variables
        if len(variables) > 0:
            context.add_clause(list(variables))

    def encode_amo(self, context: EncodingContext, variables: list[int]) -> None:
        n = len(variables)
        if n <= self.threshold:
            self.pairwise_encoder.encode_amo(context, variables)
        else:
            self.nsc_encoder.encode_amo(context, variables)
