# Decision Log: Hybrid 1-Hot Bucket

## Phase 1: Initial Design
- **Decision:** Use a Hybrid 1-Hot Bucket encoding (Mixed-Radix with Order Encoding for the MSB and Binary Adder for the LSB).
- **Rationale:** Order Encoding on the high bits ($K=25$) provides strong global BCP without clause explosion, while the Binary Adder on the low bits ($M=20$) keeps the total clause count small ($< 5M$ clauses).

## Phase 2: Structured Review

### Review 1: Skeptic / Challenger
- **Objection:** Pigeonhole Principle still applies. The solver can "skip" buckets (e.g., jump from bucket 1 to 10), wasting capacity. Eventually it hits bucket 25 too early, running out of space. Proving that "skipping buckets causes starvation" is a classic Pigeonhole problem that causes exponential backtracking.
- **Resolution (Designer):** ACCEPTED. To fix this, we must enforce dense packing: $v$ can only move to bucket $B+1$ if $u$ is exactly at the end of bucket $B$. This forces a strict $+1$ increment across the entire 500 nodes.

### Review 2: Constraint Guardian
- **Objection:** By forcing a STRICT $+1$ increment to fix the Pigeonhole issue, the Order Encoding becomes a massive liability. Testing $B_v == B_u + 1$ exactly requires comparing all 25 Order variables. This generates $\approx 80-120$ clauses per edge. For $E=100,000$, this explodes to **10 million clauses** just for the transition logic. Furthermore, the global BCP advantage of Order Encoding (the $\ge$ bound) is completely lost when restricted to an exact equality. We'd be better off using a pure 9-bit binary counter ($\approx 30$ clauses/edge).
- **Resolution (Designer):** ACCEPTED. The hybrid approach fundamentally conflicts with itself. If we allow loose bounds, it thrashes (Pigeonhole). If we enforce exact bounds, it explodes in clauses and loses the Order Encoding benefit.

## Phase 3: Integration & Arbitration (Arbiter)
- **Final Decision:** REJECT V3.
- **Rationale:** The Multi-Agent review successfully identified a fatal contradiction. The V3 Hybrid Bucket is mathematically equivalent to an overly-complicated Binary Adder that takes 3x more memory and clauses without providing any additional BCP benefits.
