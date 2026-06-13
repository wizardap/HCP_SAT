class VarPool:
    def __init__(self):
        self.top = 0
    
    def new_var(self):
        self.top += 1
        return self.top
    
    def reset(self):
        self.top = 0
    
    def new_vars(self, n):
        self.top += n
        # Return variables from self.top - n + 1 up to self.top
        return [self.top - i for i in range(n - 1, -1, -1)]
    
    def __len__(self):
        return self.top
    
