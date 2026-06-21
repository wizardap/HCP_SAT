#pragma once
#include "../../utils/EncodingContext.hpp"
#include "../../utils/Graph.hpp"
#include "../../encodings/cardinality_one/CardinalityOneEncoder.hpp"
#include <memory>
#include <stdexcept>

class SuccessorMethod {
public:
    CardinalityOneEncoder* encoder = nullptr;
    bool is_preprocessing = true;
    EncodingContext* context = nullptr;
    bool is_incremental = false;
    int cycle_override = -1;
    int cycle = -1;

    SuccessorMethod(CardinalityOneEncoder* enc = nullptr) : encoder(enc) {}
    virtual ~SuccessorMethod() = default;

    void set_encoder(CardinalityOneEncoder* enc) {
        encoder = enc;
    }

    void set_is_preprocessing(bool prep) {
        is_preprocessing = prep;
    }

    int new_var() {
        if (!context) throw std::runtime_error("Context not set");
        return context->new_var();
    }

    void add_clause(const std::vector<int>& clause) {
        if (!context) throw std::runtime_error("Context not set");
        context->add_clause(clause);
    }

    void exactly_one_constraint(const std::vector<int>& literals) {
        if (encoder == nullptr) {
            throw std::runtime_error("No CardinalityOneEncoder set for SuccessorMethod.");
        }
        encoder->encode_eo(*context, literals);
    }

    virtual EncodingContext* build_clauses(EncodingContext* ctx, const Graph& graph) = 0;
    virtual int getH(int i, int j) = 0;
};
