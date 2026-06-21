#pragma once
#include "CardinalityOneEncoder.hpp"
#include <cmath>
#include <algorithm>

class CommanderEncoder : public CardinalityOneEncoder {
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

    void _naive_eo(EncodingContext& context, const std::vector<int>& variables) {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
        _naive_amo(context, variables);
    }

public:
    CommanderEncoder(int group_limit = 6) : limit(group_limit) {}

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

        int g = std::ceil(std::sqrt(n));
        std::vector<std::vector<int>> groups;
        for (int i = 0; i < n; i += g) {
            int end = std::min(i + g, n);
            groups.push_back(std::vector<int>(variables.begin() + i, variables.begin() + end));
        }

        int num_groups = groups.size();
        std::vector<int> commander_vars = context.new_vars(num_groups);

        for (int idx = 0; idx < num_groups; ++idx) {
            int commander = commander_vars[idx];
            const auto& group = groups[idx];

            _naive_amo(context, group);

            for (int x : group) {
                context.add_clause({commander, -x});
            }
        }

        encode_amo(context, commander_vars);
    }

    void encode_eo(EncodingContext& context, const std::vector<int>& variables) override {
        int n = variables.size();
        if (n == 0) {
            context.add_clause({});
            return;
        }
        if (n == 1) {
            context.add_clause({variables[0]});
            return;
        }
        if (n <= limit) {
            _naive_eo(context, variables);
            return;
        }

        int g = std::ceil(std::sqrt(n));
        std::vector<std::vector<int>> groups;
        for (int i = 0; i < n; i += g) {
            int end = std::min(i + g, n);
            groups.push_back(std::vector<int>(variables.begin() + i, variables.begin() + end));
        }

        int num_groups = groups.size();
        std::vector<int> commander_vars = context.new_vars(num_groups);

        for (int idx = 0; idx < num_groups; ++idx) {
            int commander = commander_vars[idx];
            const auto& group = groups[idx];

            _naive_amo(context, group);

            std::vector<int> eo_clause = {-commander};
            eo_clause.insert(eo_clause.end(), group.begin(), group.end());
            context.add_clause(eo_clause);

            for (int x : group) {
                context.add_clause({commander, -x});
            }
        }

        encode_eo(context, commander_vars);
    }
};
