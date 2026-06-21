#pragma once
#include <vector>

class CNF {
public:
    std::vector<std::vector<int>> clauses;

    void add_clause(const std::vector<int>& clause) {
        clauses.push_back(clause);
    }

    void extend(const std::vector<std::vector<int>>& new_clauses) {
        clauses.insert(clauses.end(), new_clauses.begin(), new_clauses.end());
    }

    size_t size() const {
        return clauses.size();
    }
};
