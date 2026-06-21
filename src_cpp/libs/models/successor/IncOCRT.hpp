#pragma once
#include "OCRT.hpp"

class IncOCRT : public OCRT {
public:
    IncOCRT(CardinalityOneEncoder* vee_enc = nullptr, CardinalityOneEncoder* crt_enc = nullptr, int cycle_len = 12)
        : OCRT(vee_enc, crt_enc, cycle_len) {
        is_incremental = true;
    }
};
