#pragma once
#include "CardinalityOneEncoder.hpp"
#include "BimanderEncoder.hpp"

class PbLibEncoder : public CardinalityOneEncoder {
private:
    BimanderEncoder fallback;

public:
    void encode_alo(EncodingContext& context, const std::vector<int>& variables) override {
        fallback.encode_alo(context, variables);
    }

    void encode_amo(EncodingContext& context, const std::vector<int>& variables) override {
        fallback.encode_amo(context, variables);
    }

    void encode_eo(EncodingContext& context, const std::vector<int>& variables) override {
        fallback.encode_eo(context, variables);
    }
};
