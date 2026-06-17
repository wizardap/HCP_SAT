import math
from itertools import combinations

# (bits, clauses_per_edge, period, name)
blocks = [
    (1, 2, 2, "Mod2"),
    (2, 6, 3, "Size2"),
    (3, 8, 7, "Size3"),
    (5, 12, 31, "Size5"),
    (7, 16, 127, "Size7"),
    (11, 24, 2047, "Size11"),
]

def solve(N):
    best_bits = 999
    best_clauses = 999
    best_combo = None
    
    # Try all subsets
    for r in range(1, len(blocks)+1):
        for combo in combinations(blocks, r):
            # Check if mutually coprime
            coprime = True
            for i in range(len(combo)):
                for j in range(i+1, len(combo)):
                    if math.gcd(combo[i][2], combo[j][2]) != 1:
                        coprime = False
            if not coprime: continue
            
            prod = 1
            bits = 0
            clauses = 0
            for c in combo:
                prod *= c[2]
                bits += c[0]
                clauses += c[1]
                
            if prod >= N:
                # We want to minimize bits, then clauses
                if bits < best_bits or (bits == best_bits and clauses < best_clauses):
                    best_bits = bits
                    best_clauses = clauses
                    best_combo = combo
                    
    print(f"N={N:5} -> Prod={math.prod(c[2] for c in best_combo):7} | Bits={best_bits:2} | Clauses={best_clauses:2} | Combo={[c[3] for c in best_combo]}")

solve(142)
solve(150)
solve(214)
solve(318)
solve(1000)
solve(5000)
solve(10000)
