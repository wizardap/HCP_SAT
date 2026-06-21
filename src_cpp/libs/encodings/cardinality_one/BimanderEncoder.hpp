#pragma once
#include "CardinalityOneEncoder.hpp"
#include <cmath>
#include <algorithm>

class BimanderEncoder : public CardinalityOneEncoder {
public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void naive_amo(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                context.add_clause({-variables[i], -variables[j]});
            }
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;

        int g = std::ceil(std::sqrt(n));
        int m_groups = std::ceil((double)n / g);

        if (m_groups <= 1) {
            naive_amo(context, variables);
            return;
        }

        std::vector<std::vector<int>> groups;
        for (int i = 0; i < m_groups; ++i) {
            int start = i * g;
            int end = std::min(start + g, n);
            if (start < end) {
                groups.push_back(std::vector<int>(variables.begin() + start, variables.begin() + end));
            }
        }

        int num_groups = groups.size();

        for (const auto& group : groups) {
            naive_amo(context, group);
        }

        int num_aux = std::ceil(std::log2(num_groups));
        if (num_aux > 0) {
            std::vector<int> aux_vars = context.new_vars(num_aux);

            for (int i = 0; i < num_groups; ++i) {
                for (int var : groups[i]) {
                    for (int j = 0; j < num_aux; ++j) {
                        int bit = (i >> j) & 1;
                        if (bit == 1) {
                            context.add_clause({-var, aux_vars[j]});
                        } else {
                            context.add_clause({-var, -aux_vars[j]});
                        }
                    }
                }
            }
        }
    }
};
