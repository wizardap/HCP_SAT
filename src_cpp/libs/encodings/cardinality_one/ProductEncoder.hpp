#pragma once
#include "CardinalityOneEncoder.hpp"
#include <cmath>
#include <algorithm>

class ProductEncoder : public CardinalityOneEncoder {
private:
    int limit;

    void _naive_amo(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                context.add_clause({-variables[i], -variables[j]});
            }
        }
    }

    int _get_bit(int mask, int pos) {
        return (mask >> pos) & 1;
    }

    void _binary_amo(EncodingContext& context, const std::vector<int>& variables) {
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

public:
    ProductEncoder(int size_limit = 10) : limit(size_limit) {}

    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;
        if (n <= limit) {
            _naive_amo(context, variables);
            return;
        }

        int p = std::ceil(std::sqrt(n));
        int q = std::ceil((double)n / p);

        std::vector<int> u = context.new_vars(p);
        std::vector<int> v = context.new_vars(q);

        _binary_amo(context, u);
        _binary_amo(context, v);

        int n_count = 0;
        for (int j = 0; j < q; ++j) {
            for (int i = 0; i < p; ++i) {
                context.add_clause({-variables[n_count], u[i]});
                context.add_clause({-variables[n_count], v[j]});
                n_count++;
                if (n_count == n) break;
            }
            if (n_count == n) break;
        }
    }
};
