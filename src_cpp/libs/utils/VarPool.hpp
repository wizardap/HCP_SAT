#pragma once
#include <vector>

class VarPool {
public:
    int top = 0;

    int new_var() {
        top++;
        return top;
    }

    void reset() {
        top = 0;
    }

    std::vector<int> new_vars(int n) {
        top += n;
        std::vector<int> vars(n);
        for (int i = 0; i < n; ++i) {
            vars[i] = top - n + 1 + i;
        }
        return vars;
    }

    size_t size() const {
        return top;
    }
};
