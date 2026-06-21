#pragma once
#include "../utils/EncodingContext.hpp"
#include "../encodings/cardinality_one/CardinalityOneEncoder.hpp"
#include <vector>

class NQueensModel {
public:
    int n;
    CardinalityOneEncoder* encoder;
    EncodingContext context;
    std::vector<std::vector<int>> grid;

    NQueensModel(int size, CardinalityOneEncoder* enc) : n(size), encoder(enc) {
        _build_grid();
    }

    void _build_grid() {
        std::vector<int> vars_list = context.new_vars(n * n);
        grid.resize(n, std::vector<int>(n));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                grid[i][j] = vars_list[i * n + j];
            }
        }
    }

    EncodingContext& encode() {
        // 1. Exactly one queen in each row
        for (int row = 0; row < n; ++row) {
            encoder->encode_eo(context, grid[row]);
        }

        // 2. Exactly one queen in each column
        for (int col = 0; col < n; ++col) {
            std::vector<int> column(n);
            for (int row = 0; row < n; ++row) {
                column[row] = grid[row][col];
            }
            encoder->encode_eo(context, column);
        }

        // 3. At most one queen in each main diagonal
        for (int d = 0; d < 2 * n - 1; ++d) {
            std::vector<int> diag;
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (i - j == d - n + 1) {
                        diag.push_back(grid[i][j]);
                    }
                }
            }
            encoder->encode_amo(context, diag);
        }

        // 4. At most one queen in each anti-diagonal
        for (int d = 0; d < 2 * n - 1; ++d) {
            std::vector<int> diag;
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (i + j == d) {
                        diag.push_back(grid[i][j]);
                    }
                }
            }
            encoder->encode_amo(context, diag);
        }

        return context;
    }
};
