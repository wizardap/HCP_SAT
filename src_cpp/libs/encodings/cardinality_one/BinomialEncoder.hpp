#pragma once
#include "CardinalityOneEncoder.hpp"

class BinomialEncoder : public CardinalityOneEncoder {
public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                context.add_clause({-variables[i], -variables[j]});
            }
        }
    }
};
