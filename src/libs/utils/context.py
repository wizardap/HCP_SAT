
from libs.utils.cnf import CNF
from libs.utils.var_pool import VarPool


class EncodingContext:
    def __init__(self):
        self.var_pool = VarPool()
        self.cnf = CNF()
    
    def new_var(self):
        return self.var_pool.new_var()
    
    def new_vars(self, n):
        return self.var_pool.new_vars(n)
    
    def add_clause(self, clause):
        self.cnf.add_clause(clause)
    
    def extend(self, clauses):
        self.cnf.extend(clauses)
    
    def total_clauses(self):
        return len(self.cnf)
    
    def total_vars(self):
        return len(self.var_pool)
    
    