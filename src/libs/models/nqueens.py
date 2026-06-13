from libs.utils.context import EncodingContext
from libs.encodings.cardinality_one.base import CardinalityOneEncoder


class NQueensModel:
    def __init__(self, n: int, encoder: CardinalityOneEncoder):
        """
        Args:
            n: Size of the board (N x N) and the number of queens.
            encoder: A polymorphic CardinalityOneEncoder implementation.
        """
        self.n = n
        self.encoder = encoder
        self.context = EncodingContext()
        self.grid = []
        self._build_grid()

    def _build_grid(self) -> None:
        """Generates the grid of SAT variables representing the board cells."""
        vars_list = self.context.new_vars(self.n * self.n)
        self.grid = [vars_list[i * self.n : (i + 1) * self.n] for i in range(self.n)]

    def encode(self) -> EncodingContext:
        """
        Encodes the N-Queens problem constraints:
        1. Exactly one queen in each row.
        2. Exactly one queen in each column.
        3. At most one queen in each main diagonal.
        4. At most one queen in each anti-diagonal.
        """
        n = self.n

        # 1. Exactly one queen in each row
        for row in range(n):
            self.encoder.encode_eo(self.context, self.grid[row])

        # 2. Exactly one queen in each column
        for col in range(n):
            self.encoder.encode_eo(self.context, [self.grid[row][col] for row in range(n)])

        # 3. At most one queen in each main diagonal
        for d in range(2 * n - 1):
            diag = [
                self.grid[i][j]
                for i in range(n)
                for j in range(n)
                if i - j == d - n + 1
            ]
            self.encoder.encode_amo(self.context, diag)

        # 4. At most one queen in each anti-diagonal
        for d in range(2 * n - 1):
            diag = [
                self.grid[i][j]
                for i in range(n)
                for j in range(n)
                if i + j == d
            ]
            self.encoder.encode_amo(self.context, diag)

        return self.context
