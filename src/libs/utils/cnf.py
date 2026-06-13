class CNF:
    def __init__(self):
        self.clauses = []
    
    def add_clause(self, clause):
        self.clauses.append(clause)

    def extend(self, clauses):
        self.clauses.extend(clauses)
    
    def __len__(self):
        return len(self.clauses)
    
    def __iter__(self):
        return iter(self.clauses)
    
    