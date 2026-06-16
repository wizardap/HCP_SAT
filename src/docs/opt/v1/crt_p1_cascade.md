# CRT Preprocessing Phase 1: Degree Cascade Only

## 1. Overview
Phương pháp **CRT P1** kết hợp mô hình CRT Baseline với giai đoạn tiền xử lý đồ thị ở mức độ đơn giản nhất: **Kiểm tra tính liên thông** và **Lan truyền bậc đỉnh (Degree Cascade)**.

*   **Mục tiêu**: Loại bỏ sớm các trường hợp hiển nhiên không có chu trình (UNSAT) và cố định trước các cạnh bắt buộc phải đi qua, giúp giảm đáng kể kích thước công thức CNF trước khi nạp vào solver.
*   **Mức độ tiền xử lý**: Chỉ kích hoạt Phase 1 (Degree Cascade & 2-Connectivity), tắt toàn bộ các bước co đỉnh (Contraction), 2-cut và Probing.

---

## 2. Mathematical & Logical Formulation

### 2.1 Kiểm tra liên thông 2 chiều (2-Connectivity Check)
Chu trình Hamiltonian đòi hỏi đồ thị phải có liên thông mạnh (đồ thị có hướng) hoặc song liên thông (đồ thị vô hướng):
*   **Có hướng**: $\text{SCC\_Count}(G) = 1$ (sử dụng thuật toán Tarjan hoặc Kosaraju).
*   **Vô hướng**: Đồ thị không chứa đỉnh khớp (articulation point) chia cắt đồ thị (sử dụng thuật toán DFS Tarjan).
*   *Nếu không thỏa mãn*: Trả về `UNSAT` sớm.

### 2.2 Quy tắc Lan truyền bậc đỉnh (Degree Cascade Rules)
Với mỗi đỉnh $u \in V$, gọi $deg_{active}(u)$ là số lượng cạnh kề chưa bị cấm (forbidden):
1.  **Quy tắc UNSAT**: 
    Nếu tồn tại đỉnh $u$ có $deg_{active}(u) < 2$, đồ thị chắc chắn không chứa chu trình Hamiltonian $\implies$ Báo `UNSAT` ngay lập tức.
2.  **Quy tắc Ép buộc (Forced Edges)**:
    Nếu tồn tại đỉnh $u$ có $deg_{active}(u) = 2$, ép buộc cả 2 cạnh kề của $u$ phải nằm trong chu trình:
    $$e_1, e_2 \in Adj(u) \implies \text{Force}(e_1), \text{Force}(e_2)$$
3.  **Quy tắc Cấm (Forbidden Edges)**:
    Nếu một cạnh $e = (u,v)$ được ép buộc thuộc chu trình, thì toàn bộ các cạnh kề khác của $u$ và $v$ đều không được phép chọn (vì bậc vào/ra của mỗi đỉnh trong chu trình chỉ bằng 1):
    $$\text{Force}(e) \implies \text{Forbid}(e') \quad \forall e' \in Adj(u) \cup Adj(v) \setminus \{e\}$$
4.  **Vòng lặp Fixpoint**: 
    Việc cấm các cạnh ở Bước 3 làm giảm bậc $deg_{active}$ của các đỉnh lân cận, có thể kích hoạt các đỉnh mới có bậc bằng 2. Thuật toán chạy lặp liên tục cho đến khi không có thêm cạnh nào bị ép buộc hoặc cấm.

---

## 3. Algorithm Steps
1.  **Kiểm tra tính liên thông**: Chạy giải thuật DFS Tarjan trên đồ thị. Nếu đồ thị bị phân mảnh, dừng thuật toán và báo `UNSAT`.
2.  **Khởi tạo hàng đợi**: Đưa toàn bộ các đỉnh kề có bậc bằng 2 vào hàng đợi xử lý.
3.  **Thực thi Cascade**:
    *   Lấy đỉnh $u$ từ hàng đợi.
    *   Ép buộc 2 cạnh kề của $u$.
    *   Gán nhãn cấm cho các cạnh kề khác của các đỉnh đích.
    *   Cập nhật bậc danh sách kề và đưa các đỉnh bị giảm bậc về 2 vào hàng đợi.
4.  **Sinh CNF**: Xây dựng công thức CRT trên đồ thị hiện tại. Đưa các cạnh đã bị ép buộc vào SAT Solver dưới dạng các mệnh đề đơn vị (unit clauses) có độ dài 1 (ví dụ: $H_{i,j}$).

---

## 4. Evaluation & Insights
*   **Kết quả thực nghiệm**: Trên tập dữ liệu `fhcppp`, Degree Cascade chứng minh hiệu quả cực kỳ mạnh mẽ trên các đồ thị có nhiều cấu trúc dây chuyền cục bộ:
    *   **Giảm 95.5% số lượng mệnh đề trên `graph162`**: Số clauses của `graph162` giảm từ 20.4M xuống còn **912,587**, thời gian giải SAT tương ứng giảm từ **8.198s** xuống chỉ còn **0.002s** (tốc độ tức thời).
    *   **Giảm 50% số mệnh đề trên đồ thị mẫu fhcpcs**: Cascade giúp tối ưu hóa một nửa số lượng mệnh đề trên hầu hết các đồ thị dạng cubic.
*   **Overhead**: Thời gian tiền xử lý mức Phase 1 cực kỳ nhỏ (thường $< 0.01$s), là bước tối ưu hóa thiết yếu và mang lại tỷ suất hiệu năng trên chi phí cao nhất.
