#pragma once
#include "CardinalityOneEncoder.hpp"

class HeuleEncoder : public CardinalityOneEncoder {
private:
    void _heule_amo(EncodingContext& context, std::vector<int> array) {
        int size = array.size();
        if (size <= 1) return;

        if (size > 1) {
            context.add_clause({-array[0], -array[1]});
        }
        if (size > 2) {
            context.add_clause({-array[0], -array[2]});
            context.add_clause({-array[1], -array[2]});
        }
        if (size == 4) {
            context.add_clause({-array[0], -array[3]});
            context.add_clause({-array[1], -array[3]});
            context.add_clause({-array[2], -array[3]});
        }
        if (size > 4) {
            int max_var = context.new_var();
            context.add_clause({-array[0], max_var});
            context.add_clause({-array[1], max_var});
            context.add_clause({-array[2], max_var});

            std::vector<int> array_copy(array.begin() + 3, array.end());
            array_copy.push_back(max_var);
            _heule_amo(context, array_copy);
        }
    }

public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        if (!variables.empty()) {
            context.add_clause(variables);
        }
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        _heule_amo(context, variables);
    }
};
