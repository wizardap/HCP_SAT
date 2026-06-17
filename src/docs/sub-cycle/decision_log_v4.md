# Decision Log: V4 Positional Encoding

## Phase 1: Initial Design
- **Decision:** Use Positional Encoding (Time-Unrolled) instead of Successor Encodings.
- **Alternatives considered:** Pure CEGAR (too slow in Python), MTZ/Binary Adder (Thrashing), Reachability (Memory Bloat).
- **Rationale:** 
  1. Dense graphs have very few NON-edges, drastically reducing the number of Transition clauses (which scale with $V \times \bar{E}$). 
  2. Pure 2-SAT transition constraints are processed optimally by CDCL solvers.
  3. Cycle elimination is structurally guaranteed without explicit counting logic or CEGAR loops.

## Phase 2: Structured Review
*(Waiting for agents...)*
