# Kế hoạch Tối ưu hóa Chặn chu trình con - V3: Hybrid 1-Hot Bucket

## 1. Cơ sở lý thuyết
Chia $V$ đỉnh thành $K$ khối (buckets), mỗi khối chứa tối đa $M$ đỉnh. (Ví dụ $V=500$, $K=25$, $M=20$).
Mỗi đỉnh $u$ được gán:
1. **Bucket ID ($B_u$)**: Mã hóa Order Encoding (1-Hot cộng dồn) với $K$ biến $O_{u, i} \in \{0, 1\}$. $O_{u, i} = 1 \implies O_{u, i-1} = 1$.
2. **Sub-ID ($S_u$)**: Mã hóa Binary với $\lceil \log_2 M \rceil$ biến (ví dụ 5 bits cho $M=20$).

## 2. Luật chuyển trạng thái
Nếu cạnh $H_{u,v} = 1$:
- Cấm Bucket ID lùi lại: $B_v \ge B_u$. (Mã hóa: $O_{u, i} \implies O_{v, i}$).
- Nếu $B_v == B_u$, thì Sub-ID phải tăng 1: $S_v = (S_u + 1) \bmod M$.
  (Mã hóa: $B_v == B_u$ tương đương $O_{v, i} == O_{u, i}$ với mọi $i$. Khi đó mạch cộng Binary Adder 5-bit được kích hoạt).
- Nếu $B_v > B_u$, Sub-ID có thể nhận giá trị bất kỳ (thường là reset về 0).

## 3. Mục tiêu
- Order Encoding cho phép BCP lan truyền cực mạnh: Bất kỳ gán giá trị nào cho Bucket ID sẽ cắt tỉa toàn bộ các đỉnh phía sau.
- Binary Sub-ID giúp giữ clause count thấp (mỗi cạnh chỉ tốn vài chục mệnh đề).
- Ngăn chặn hoàn toàn Pigeonhole Principle và Clause Explosion.
