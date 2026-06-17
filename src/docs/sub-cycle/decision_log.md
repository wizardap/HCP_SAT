# Decision Log: Sub-cycle Breaking (MTZ Comparator) -> Exact Binary Incrementer

## Phase 1: Initial Design
- **Decision:** Use MTZ (Miller-Tucker-Zemlin) Comparator instead of CRT/LFSR exact counting for sub-cycle breaking.
- **Rationale:** Exact transition equality causes "Clause Explosion". MTZ uses inequality ($U_v > U_u$) to reduce CNF size.

## Phase 2: Structured Review

### Reviewer 1: Skeptic / Challenger
- **Objection:** Pigeonhole Principle makes MTZ inequality fail catastrophically. The SAT solver will not infer global bounds and will instead explore an exponentially large space of invalid assignments before backtracking.
- **Resolution:** ACCEPTED. MTZ rejected. Revised to Single LFSR.

### Reviewer 2: Constraint Guardian
- **Objection:** Single LFSR relies on pseudo-random XOR chains. CDCL solvers cannot learn distance bounds from XOR chains. Monolithic LFSR destroys the "early local conflict detection" that CRT had.
- **Resolution:** ACCEPTED. Single LFSR rejected. Revised to **Exact Binary Incrementer with Carry-Chain**. This provides small structural sub-periods (bit 0 flips every step, bit 1 every 2 steps) for early BCP pruning, while keeping clauses low ($\sim 30$ per edge).

### Reviewer 3: User Advocate
- **Objection:** The Exact Binary Incrementer is great for debugging since $U_v$ directly encodes the path index. However, hardcoding 9-bits is a silent trap that will cause false UNSAT on graphs where $V \ge 512$.
- **Resolution:** ACCEPTED. The bit-width must be calculated dynamically as $\lceil \log_2(V) \rceil$ based on the input graph, and runtime validation must be added to prevent overflow.

## Phase 3: Integration & Arbitration
- **Final Decision:** All reviewer objections have been ACCEPTED. The initial MTZ design has been entirely replaced by a far more robust **Dynamic Exact Binary Incrementer**. 
- **Integrator Sign-off:** The final design solves the original Clause Explosion problem, retains early CDCL conflict pruning, avoids Pigeonhole Principle backtracking, provides excellent developer debuggability, and includes safeguards against overflow false-UNSATs.
- **Status:** APPROVED.
