#pragma once
#include "../../utils/EncodingContext.hpp"
#include <vector>

class CardinalityOneEncoder {
public:
    virtual ~CardinalityOneEncoder() = default;

    virtual void encode_alo(EncodingContext& context, const std::vector<int>& variables) = 0;
    virtual void encode_amo(EncodingContext& context, const std::vector<int>& variables) = 0;

    virtual void encode_eo(EncodingContext& context, const std::vector<int>& variables) {
        int n = variables.size();
        if (n == 0) {
            context.add_clause({});
            return;
        } else if (n == 1) {
            context.add_clause({variables[0]});
            return;
        }

        // At-Least-One
        encode_alo(context, variables);

        // At-Most-One
        encode_amo(context, variables);
    }
};
