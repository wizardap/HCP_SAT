#pragma once
#include "CardinalityOneEncoder.hpp"

class SequentialEncoder : public CardinalityOneEncoder {
public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;

        std::vector<int> seq_vars = context.new_vars(n - 1);

        for (int i = 0; i < n - 1; ++i) {
            context.add_clause({-variables[i], seq_vars[i]});
        }

        for (int i = 1; i < n - 1; ++i) {
            context.add_clause({-seq_vars[i - 1], seq_vars[i]});
        }

        for (int i = 1; i < n; ++i) {
            context.add_clause({-seq_vars[i - 1], -variables[i]});
        }
    }
};
