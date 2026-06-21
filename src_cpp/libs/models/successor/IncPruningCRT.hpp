#pragma once
#include "PruningCRT.hpp"

class IncPruningCRT : public PruningCRT {
public:
    IncPruningCRT(CardinalityOneEncoder* enc = nullptr, int cycle_len = 12) : PruningCRT(enc, cycle_len) {
        is_incremental = true;
    }
};
