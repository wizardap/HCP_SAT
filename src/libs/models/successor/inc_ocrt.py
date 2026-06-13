from libs.models.successor.ocrt import OCRT
from libs.encodings.cardinality_one.scl import NSCEncoding


class IncOCRT(OCRT):
    """
    Incremental OCRT (VEE + CRT) Successor Encoding.
    It inherits the hybrid VEE + CRT encoding logic but defaults the cycle limit to 12
    and marks itself as incremental.
    """
    def __init__(self, vee_encoder=None, crt_encoder=None, cycle=12):
        if vee_encoder is None:
            vee_encoder = NSCEncoding()
        super().__init__(vee_encoder=vee_encoder, crt_encoder=crt_encoder, cycle=cycle)
        self.is_incremental = True
