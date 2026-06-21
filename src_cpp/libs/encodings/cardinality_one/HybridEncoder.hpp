#pragma once
#include "CardinalityOneEncoder.hpp"
#include <cmath>
#include <algorithm>

class HybridEncoder : public CardinalityOneEncoder {
private:
    int threshold;

    void _pw(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                context.add_clause({-variables[i], -variables[j]});
            }
        }
    }

    void _bs(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        if (n <= 1) return;

        int m = n / 2;
        int t = context.new_var();

        for (int i = 0; i < n; ++i) {
            int val = (i < m) ? t : -t;
            context.add_clause({-variables[i], val});
        }

        std::vector<int> left(variables.begin(), variables.begin() + m);
        if (m <= 4) {
            _pw(context, left);
        } else {
            _bs(context, left);
        }

        std::vector<int> right(variables.begin() + m, variables.end());
        if (n - m <= 4) {
            _pw(context, right);
        } else {
            _bs(context, right);
        }
    }

    void _pd(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        if (n <= 1) return;

        int m = std::ceil(std::sqrt(n));
        std::vector<int> r = context.new_vars(m);
        std::vector<int> c = context.new_vars(m);

        for (int i = 0; i < n; ++i) {
            context.add_clause({-variables[i], r[i / m]});
            context.add_clause({-variables[i], c[i % m]});
        }

        if (m <= threshold) {
            _bs(context, r);
            _bs(context, c);
        } else {
            _pd(context, r);
            _pd(context, c);
        }
    }

public:
    HybridEncoder(int threshold_limit = 32) : threshold(threshold_limit) {}

    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n <= 1) return;

        if (n > threshold) {
            _pd(context, variables);
        } else if (n > 4) {
            _bs(context, variables);
        } else {
            _pw(context, variables);
        }
    }
};
