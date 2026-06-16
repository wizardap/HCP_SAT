# HCP-SAT CRT Preprocessing Pipeline — Design Document

## 1. Overview

Thiết kế tầng preprocessing cho CRT encoding trong project HCP-SAT, hoạt động ở cả **graph-level** (trước khi sinh CNF) và **CNF-level** (CaDiCaL built-in).

### Goals
- Giảm thời gian giải tổng thể (preprocessing + solving)
- Mở rộng khả năng giải cho đồ thị lớn hơn
- Giảm kích thước CNF (biến + clauses)
- Khảo sát/so sánh hiệu quả các kỹ thuật

### Non-Goals
- Không thay đổi CRT encoding logic (moduli, LFSR transitions)
- Không modify VEE (đã có riêng trong `ocrt.py`)

### Constraints
- Preprocessing là bước **tùy chọn** (bật/tắt để benchmark)
- Thời gian preprocessing tính vào tổng thời gian giải
- **Correctness bắt buộc** — không được mất solution hoặc tạo false UNSAT
- Linh hoạt implementation (Python + CaDiCaL tuning)

---

## 2. Architecture

### Pipeline Flow

```
Load Graph (n vertices)
    │
    ▼
┌─────────────────────────────────────┐
│      PREPROCESSING PIPELINE         │
│                                     │
│  ① 2-connectivity check ──→ UNSAT?  │
│  ④a Bridge detection ──────→ UNSAT? │
│                                     │
│  ┌─── Fixpoint Loop ────────────┐   │
│  │ ④b 2-edge-cut → force edges │   │
│  │ ② Degree cascade            │   │
│  │ ③ Path contraction          │   │
│  │ (repeat until no change)    │   │
│  └──────────────────────────────┘   │
│                                     │
│  ⑤ Graph-level probing             │
│  ⑥ Forbidden edge (sub-cycle)      │
│                                     │
│  Output: reduced_graph (n' < n),    │
│          forced_edges[],            │
│          contracted_paths[]         │
└─────────────────────────────────────┘
    │
    ▼
CRT build_clauses(reduced_graph)
  + inject forced edges as unit clauses
    │
    ▼
CaDiCaL (with tuned preprocessing)
    │
    ▼
Solution Expansion (un-contract paths)
```

### Data Structures

```python
@dataclass
class PreprocessResult:
    graph: Graph              # Đồ thị đã rút gọn
    forced_edges: list        # [(u,v), ...] chắc chắn trong HC
    forbidden_edges: list     # [(u,v), ...] chắc chắn KHÔNG trong HC
    contracted_paths: list    # Để expand solution sau
    is_unsat: bool            # True nếu phát hiện UNSAT sớm
    stats: dict               # Thống kê cho benchmark
```

---

## 3. Techniques Analysis

### ① 2-Connectivity Check
- **Undirected**: Phải biconnected (Tarjan, O(V+E))
- **Directed**: Phải strongly connected (Kosaraju/Tarjan, O(V+E))
- **Impact**: Low on designed benchmarks (most are HC-solvable), but free safety net

### ② Degree Cascade
- deg(v) < 2 → UNSAT
- deg(v) == 2 → force both edges → cascade to neighbors
- **Undirected**: track remaining_degree per vertex
- **Directed**: separate in_degree / out_degree tracking
- **Impact**: ⭐⭐⭐ Extremely high on cubic graphs (deg reduction 3→2 triggers cascade)

### ③ Path Contraction
- Chain of degree-2 vertices → contract into super-edge
- Directly reduces n → smaller CRT cycle → fewer P variables → fewer transition clauses
- Need path_map for solution expansion
- **Impact**: ⭐⭐⭐ Very high — quadratic reduction in CRT clause count

### ④ Bridge / 2-Edge-Cut Detection
- **④a Bridge** (O(V+E) Tarjan): Bridge exists → UNSAT
- **④b 2-edge-cut**: Remove each edge, run bridge detection on remainder
  - Complexity: O(E × (V+E)), acceptable for V < 500
  - For large graphs: heuristic (only try edges at low-degree vertices)
- **Impact**: ⭐⭐ High on cubic/sparse graphs

### ⑤ Graph-Level Probing
- **Negative probe**: Remove edge → cascade → contradiction? → Force edge
- **Positive probe**: Force edge → cascade → contradiction? → Forbid edge
- Extremely powerful on cubic graphs (removing 1 edge → deg=2 → cascade)
- **Budget**: O(E × (V+E)) per iteration; limit iterations for V > 1000
- **Impact**: ⭐⭐⭐ Extremely high on cubic, ⭐ Medium on dense

### ⑥ Forbidden Edge (Sub-Cycle Detection)
- During probing cascade, check if forced edges form premature cycle (< n vertices)
- If premature cycle detected → contradiction → edge is forbidden/forced
- **Impact**: ⭐⭐ Medium-high

### ⑧ CaDiCaL Preprocessing Tuning
- CaDiCaL has built-in FLP, BVE, subsumption, vivification
- Tune flags: --preprocessing, --probeinit, --subsume, --vivify
- **Impact**: ⭐ Medium — already active by default

---

## 4. Impact Assessment by Dataset

| Technique | fhcpcs (Cubic, deg=3) | fhcppp (Large) | vset (deg 5-7) |
|---|---|---|---|
| ① 2-Connectivity | 🔴 Low | 🟡 Medium | 🔴 Low |
| ② Degree Cascade | ⭐ Very High (with probing) | ⭐ High | 🔴 Low |
| ③ Path Contraction | ⭐ Very High | ⭐ Very High | 🟡 Medium |
| ④ Bridge/2-Cut | ⭐ High | ⭐ High | 🔴 Low |
| ⑤ Graph Probing | ⭐ Very High | 🟡 Medium (overhead) | 🟡 Medium |
| ⑥ Forbidden Edge | 🟡 Medium | 🟡 Medium | 🟡 Medium |
| ⑧ CaDiCaL Tuning | 🟡 Medium | 🟡 Medium | 🟡 Medium |

Key insight: Probing + Cascade + Contraction = "tam tài" trên cubic graphs.

---

## 5. Implementation Plan (Approach A — Incremental)

### Phase 1: Baseline (Low effort, fast impact)
| Task | Description |
|---|---|
| 1.1 | `HCPPreprocessor` class + `PreprocessResult` dataclass |
| 1.2 | 2-connectivity check (Tarjan biconnected / Kosaraju SCC) |
| 1.3 | Degree cascade (fixpoint, directed/undirected support) |
| 1.4 | CaDiCaL preprocessing tuning |
| 1.5 | Integration into solver + benchmark |

### Phase 2: Reduce n (Highest CRT impact)
| Task | Description |
|---|---|
| 2.1 | Bridge detection (Tarjan) → UNSAT |
| 2.2 | 2-edge-cut detection + force edges |
| 2.3 | Path contraction + solution expansion |
| 2.4 | Fixpoint loop: 2-cut → cascade → contraction → repeat |
| 2.5 | Benchmark Phase 1+2 vs Phase 1 only |

### Phase 3: Deep probing
| Task | Description |
|---|---|
| 3.1 | Graph-level negative probing |
| 3.2 | Graph-level positive probing |
| 3.3 | Sub-cycle detection |
| 3.4 | Probe budget/heuristic for large graphs |
| 3.5 | Final comprehensive benchmark |

---

## 6. Decision Log

| # | Decision | Alternatives | Rationale |
|---|---|---|---|
| D1 | Both graph-level and CNF-level preprocessing | Graph only / CNF only | Graph reduces CNF before generation; CaDiCaL handles CNF |
| D2 | Skip custom CNF-level FLP | Implement FLP in Python | CaDiCaL already has FLP with proper time budget |
| D3 | Implement Graph-level Probing | Skip probing entirely | Same spirit as FLP but reduces CNF at source |
| D4 | Approach A (Incremental) | B (full upfront), C (external) | Measure impact per step, fits research goals |
| D5 | Prioritize cubic graphs (fhcpcs) | vset or fhcppp | Cubic graphs benefit most from probing + cascade |
| D6 | Fixpoint loop: 2-cut→cascade→contraction→repeat | Single pass | Force edges from 2-cut create new cascades |
| D7 | Probe budget for large graphs | Probe all edges | O(E×(V+E)) too expensive for V > 1000 |

---

## 7. Risks

| Risk | Mitigation |
|---|---|
| Preprocessing overhead > solving speedup | Each technique is toggleable; benchmark incrementally |
| Python performance for probing on large graphs | Budget/heuristic limits; consider C extension if needed |
| Low impact on dense graphs (vset) | Expected — focus research on cubic/sparse graphs |
| Correctness bugs in contraction/expansion | Test with known HC instances; verify expanded solution |

---

## 8. Assumptions

- Preprocessing is optional (toggle on/off for benchmarking)
- Preprocessing time counts toward total solve time
- Correctness is mandatory — no lost solutions or false UNSAT
- CaDiCaL is the primary solver backend
- Benchmark datasets are primarily from FHCP challenge sets
