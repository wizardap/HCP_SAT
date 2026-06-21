#pragma once
#include "VarPool.hpp"
#include "CNF.hpp"

class EncodingContext {
public:
    VarPool var_pool;
    CNF cnf;

    int new_var() {
        return var_pool.new_var();
    }

    std::vector<int> new_vars(int n) {
        return var_pool.new_vars(n);
    }

    void add_clause(const std::vector<int>& clause) {
        cnf.add_clause(clause);
    }

    void extend(const std::vector<std::vector<int>>& clauses) {
        cnf.extend(clauses);
    }

    size_t total_clauses() const {
        return cnf.size();
    }

    size_t total_vars() const {
        return var_pool.size();
    }
};
