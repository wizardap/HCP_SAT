#pragma once
#include "CRT.hpp"

class IncCRT : public CRT {
public:
    IncCRT(CardinalityOneEncoder* enc = nullptr, int cycle_len = 12) : CRT(enc, cycle_len) {
        is_incremental = true;
    }
};
