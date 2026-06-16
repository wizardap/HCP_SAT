# CRT Preprocessing Phase 1+2: Path Contraction & Cuts

## 1. Overview
Phương pháp **CRT P1+P2** nâng cấp quá trình tiền xử lý bằng việc tích hợp **Phase 2**, tập trung vào việc rút gọn kích thước thực tế của đồ thị thông qua thuật toán **Co đường đi (Path Contraction)** và phát hiện lát cắt nhỏ (**2-Edge-Cut & Bridge Detection**).

*   **Mục tiêu**: Giảm số lượng đỉnh thực tế $N$ cần giải. Nhờ việc giảm $N$, số lượng biến trạng thái CRT ($O(N \log N)$) và số bit modulo cần thiết sẽ giảm đi, đem lại khả năng thu gọn mạnh mẽ không gian tìm kiếm của SAT solver.
*   **Mức độ tiền xử lý**: Kích hoạt Phase 1 (Cascade) + Phase 2 (Contraction, Bridge, 2-Cut). Tắt Phase 3 (Probing).

---

## 2. Mathematical & Logical Formulation

### 2.1 Co đường đi (Path Contraction)
*   **Định nghĩa**: Một chuỗi các đỉnh liên tiếp có bậc bằng 2 dạng $u_1 - u_2 - \dots - u_k$ có thể được co lại thành một siêu cạnh (super-edge) duy nhất nối giữa $u_1$ và $u_k$, loại bỏ hoàn toàn các đỉnh trung gian khỏi đồ thị mã hóa SAT.
*   **Cơ chế hoạt động**:
    *   Giả sử có đường đi $u - v - w$ mà $\text{deg}(v) = 2$. Trong chu trình Hamiltonian, bắt buộc ta phải đi qua $u \to v \to w$ (hoặc ngược lại).
    *   Ta có thể xóa đỉnh $v$ và thay thế bằng cạnh ảo $(u, w)$.
    *   Kích thước đồ thị giảm: $N' = N - 1$.
*   **Khôi phục lời giải (Solution Expansion)**: Khi bộ giải SAT trả về chu trình chứa cạnh ảo $(u, w)$, thuật toán hậu xử lý sẽ tự động chèn lại đỉnh $v$ vào giữa $u$ và $w$ để khôi phục chu trình Hamiltonian gốc.

### 2.2 Phát hiện Cầu (Bridge) & Vết cắt 2 cạnh (2-Edge-Cut)
*   **Cầu (Bridge)**: Cạnh mà nếu loại bỏ sẽ làm tăng số thành phần liên thông của đồ thị.
    *   *Quy tắc*: Nếu đồ thị vô hướng chứa bất kỳ một cầu nào $\implies$ Báo `UNSAT` ngay lập tức (do chu trình không thể đi qua cầu sang phân mảnh khác rồi quay lại mà không lặp đỉnh).
*   **Vết cắt 2 cạnh (2-Edge-Cut)**: Cặp cạnh $\{e_1, e_2\}$ mà nếu loại bỏ sẽ chia cắt đồ thị làm 2 thành phần $G_1, G_2$.
    *   *Quy tắc*: Chu trình Hamiltonian bắt buộc phải đi qua cả hai cạnh này (1 cạnh đi sang $G_2$, 1 cạnh đi về $G_1$) $\implies$ Ép buộc cả $e_1$ và $e_2$ vào chu trình:
        $$\{e_1, e_2\} \text{ là 2-edge-cut} \implies \text{Force}(e_1) \land \text{Force}(e_2)$$

---

## 3. Algorithm Steps
1.  **Chạy Phase 1 (Degree Cascade)** để ổn định bậc của các đỉnh.
2.  **Duyệt tìm Cầu và Lát cắt**:
    *   Sử dụng giải thuật DFS Tarjan để tìm cầu. Nếu có cầu $\implies$ dừng và trả về `UNSAT`.
    *   Duyệt qua các cạnh kề để tìm vết cắt 2 cạnh (loại bỏ thử từng cạnh và tìm cầu trên đồ thị còn lại). Ép buộc các cạnh thuộc vết cắt.
3.  **Thực hiện Co đỉnh (Path Contraction)**:
    *   Tìm các chuỗi đỉnh có bậc bằng 2 kề nhau.
    *   Co chuỗi đỉnh thành siêu cạnh ảo, lưu trữ ánh xạ đường đi `contracted_paths`.
    *   Cập nhật đồ thị rút gọn $G'$ với số đỉnh $N' < N$.
4.  **Lặp Fixpoint**: Chạy lặp lại chuỗi `Cascade` $\rightarrow$ `Cuts` $\rightarrow$ `Contraction` cho đến khi đồ thị hội tụ (không thể rút gọn thêm).
5.  **Mã hóa CRT**: Sinh các mệnh đề CRT và AMO dựa trên đồ thị rút gọn $G'$ và số đỉnh $N'$.

---

## 4. Evaluation & Insights
*   **Kết quả thực nghiệm trên tập `fhcppp`**:
    *   **Giảm số biến vượt trội**: Trên `graph223`, số biến giảm từ **1,938,300** xuống còn **865,532** (giảm **55.3%**). Số mệnh đề giảm tương ứng từ **2.1M** xuống còn **1.02M** clauses.
    *   **Đồ thị `graph255`**: Giảm số lượng biến từ **2.52M** xuống còn **1.12M** biến (giảm **55.5%**).
*   **Đánh giá**: Phase 2 mang lại hiệu quả cực kỳ rõ rệt trong việc nén không gian biến trạng thái nhị phân. Mặc dù chi phí tính toán lát cắt 2 cạnh lớn hơn ($O(E \times (V+E))$), nhưng hiệu quả thu nhỏ bài toán SAT bù đắp lại hoàn toàn chi phí này trên các đồ thị lớn.
