#pragma once
#include "CardinalityOneEncoder.hpp"
#include <cmath>

class BinaryEncoder : public CardinalityOneEncoder {
private:
    int _get_bit(int mask, int pos) {
        return (mask >> pos) & 1;
    }

public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;

        int num_aux = std::ceil(std::log2(n));
        std::vector<int> aux_vars = context.new_vars(num_aux);

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < num_aux; ++j) {
                int bit = _get_bit(i, j);
                if (bit == 1) {
                    context.add_clause({-variables[i], aux_vars[j]});
                } else {
                    context.add_clause({-variables[i], -aux_vars[j]});
                }
            }
        }
    }
};
