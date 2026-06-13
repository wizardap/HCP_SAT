from libs.models.successor.crt import CRT


class IncCRT(CRT):
    """
    Incremental Successor encoding based on the Chinese Remainder Theorem (CRT).
    It inherits the base encoding logic from CRT but uses a small default cycle length (m = 12)
    to serve as the starting point for incremental SAT solving (CEGAR loop).
    """
    def __init__(self, encoder=None, cycle=12):
        super().__init__(encoder, cycle=cycle)
        self.is_incremental = True
