from libs.models.successor.ocrt import OCRT
from libs.encodings.cardinality_one.scl import NSCEncoding


class IncOCRT(OCRT):
    """
    Incremental OCRT (VEE + CRT) Successor Encoding.
    It inherits the hybrid VEE + CRT encoding logic but defaults the cycle limit to 12
    and marks itself as incremental.
    """
    def __init__(self, vee_encoder=None, crt_encoder=None, cycle=12,
                 vee_max_degree=None, vee_heuristic="min-degree",
                 vee_encoder_type="nsc", vee_clause_budget=None,
                 start_node_strategy="lowest-degree"):
        if vee_encoder is None:
            vee_encoder = NSCEncoding()
        super().__init__(vee_encoder=vee_encoder, crt_encoder=crt_encoder, cycle=cycle,
                         vee_max_degree=vee_max_degree, vee_heuristic=vee_heuristic,
                         vee_encoder_type=vee_encoder_type, vee_clause_budget=vee_clause_budget,
                         start_node_strategy=start_node_strategy)
        self.is_incremental = True
