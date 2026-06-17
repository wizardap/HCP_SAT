# Thiết kế V4: Positional Encoding (Time-Unrolled)

## 1. Biến (Variables)
Sử dụng các biến $X_{v, t}$ với ý nghĩa: "Đỉnh $v$ nằm ở vị trí thứ $t$ trên đường đi".
Với $V=500$, có $500 \times 500 = 250,000$ biến Boolean.

## 2. Ràng buộc (Clauses)
1. **AMO trên hàng (Mỗi đỉnh xuất hiện đúng 1 lần):** $\sum_t X_{v, t} = 1$ cho mọi đỉnh $v$.
2. **AMO trên cột (Mỗi thời điểm có đúng 1 đỉnh):** $\sum_v X_{v, t} = 1$ cho mọi thời điểm $t$.
3. **Ràng buộc nối cạnh (Transition):**
   Thay vì ràng buộc các cạnh "có mặt", ta loại trừ các cạnh "không tồn tại".
   Với mỗi cặp $(u, v) \notin E$ (không có cạnh nối từ $u$ tới $v$), và với mọi thời điểm $t \in \{1 \dots V-1\}$:
   $\neg X_{u, t} \vee \neg X_{v, t+1}$

## 3. Lý do áp dụng cho Đồ thị đặc (Dense Graph)
Trên đồ thị đặc (ví dụ $V=500, E=100,000$), số cạnh không tồn tại (non-edges) $\bar{E}$ là rất nhỏ: $125,000 - 100,000 = 25,000$.
Tổng số clause cho bước 3 là: $25,000 \times 499 \approx 12.5$ triệu clauses.
Toàn bộ là 2-SAT (mệnh đề độ dài 2), không chứa logic số học (cộng/XOR), giúp SAT solver lan truyền (BCP) cực nhanh. Lại hoàn toàn miễn nhiễm với chu trình con.
