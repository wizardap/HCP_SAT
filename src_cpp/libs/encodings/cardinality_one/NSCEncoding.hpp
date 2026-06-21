#pragma once
#include "CardinalityOneEncoder.hpp"

class NSCEncoding : public CardinalityOneEncoder {
public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;

        if (n <= 3) {
            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    context.add_clause({-variables[i], -variables[j]});
                }
            }
            return;
        }

        std::vector<int> R = context.new_vars(n - 1);

        for (int i = 0; i < n - 1; ++i) {
            context.add_clause({-variables[i], R[i]});
        }

        for (int i = 1; i < n - 1; ++i) {
            context.add_clause({-R[i - 1], R[i]});
        }

        for (int i = 1; i < n; ++i) {
            context.add_clause({-variables[i], -R[i - 1]});
        }
    }
};

using NSCEncoder = NSCEncoding;
using SCLEncoder = NSCEncoding;
