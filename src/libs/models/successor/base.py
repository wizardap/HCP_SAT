from libs.utils.context import EncodingContext
from libs.encodings.cardinality_one.base import CardinalityOneEncoder


class SuccessorMethod:
    def __init__(self, encoder: CardinalityOneEncoder = None):
        self.encoder = encoder
        self.is_preprocessing = True
        self.context = None

    def set_encoder(self, encoder: CardinalityOneEncoder):
        self.encoder = encoder

    def set_is_preprocessing(self, is_preprocessing):
        self.is_preprocessing = is_preprocessing

    def new_var(self):
        return self.context.new_var()

    def add_clause(self, clause):
        self.context.add_clause(clause)

    def exactly_one_constraint(self, literals):
        if self.encoder is None:
            raise ValueError("No CardinalityOneEncoder set for SuccessorMethod.")
        self.encoder.encode_eo(self.context, literals)

    def build_clauses(self, context: EncodingContext, graph) -> EncodingContext:
        raise NotImplementedError("Subclasses must implement build_clauses.")
