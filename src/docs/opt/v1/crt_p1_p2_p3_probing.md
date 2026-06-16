# CRT Preprocessing Phase 1+2+3: Graph Probing & Sub-cycle Detection

## 1. Overview
Phương pháp **CRT P1+P2+P3** tích hợp đầy đủ 3 pha tiền xử lý đồ thị, bao gồm cả **Phase 3: Probing & Sub-cycle Detection**.

*   **Mục tiêu**: Khai phá sâu các ràng buộc ẩn của đồ thị thông qua việc thử nghiệm gán nhãn biến (Probing) để kích hoạt Degree Cascade tìm mâu thuẫn logic, đồng thời phát hiện sớm chu trình con khép kín (sub-cycle).
*   **Mức độ tiền xử lý**: Kích hoạt toàn bộ các pha (P1 + P2 + P3). Thiết lập giới hạn số cạnh probing tối đa là 300 (`max_probing_edges = 300`) để tránh bùng nổ thời gian xử lý trên các đồ thị lớn.

---

## 2. Mathematical & Logical Formulation

### 2.1 Thuật toán Dò tìm cạnh sâu (Graph-Level Probing)
Duyệt qua các cạnh $e = (u,v)$ chưa được xác định (chưa bị ép buộc hay bị cấm):

1.  **Dò tìm phủ định (Negative Probing)**:
    *   Giả định cạnh $e$ KHÔNG thuộc chu trình: Gán $e = \text{False}$ (đưa vào tập `forbidden`).
    *   Chạy lan truyền **Degree Cascade**.
    *   *Nếu phát hiện mâu thuẫn* (ví dụ: tồn tại đỉnh có bậc kề $< 2$): Giả định sai $\implies$ Cạnh $e$ bắt buộc phải thuộc chu trình $\implies$ Thực hiện $\text{Force}(e)$.
2.  **Dò tìm khẳng định (Positive Probing)**:
    *   Giả định cạnh $e$ THUỘC chu trình: Gán $e = \text{True}$ (đưa vào tập `forced`).
    *   Chạy lan truyền **Degree Cascade**.
    *   *Nếu phát hiện mâu thuẫn*: Giả định sai $\implies$ Cạnh $e$ không được phép thuộc chu trình $\implies$ Thực hiện $\text{Forbid}(e)$.

### 2.2 Phát hiện chu trình con sớm (Sub-cycle Detection)
Trong quá trình chạy thử nghiệm Cascade ở bước Probing:
*   Gọi $E_{forced}$ là tập hợp tất cả các cạnh đã được ép buộc.
*   Nếu $E_{forced}$ tạo thành một chu trình khép kín kết nối $k$ đỉnh mà $k < N$ (số đỉnh của đồ thị hiện tại):
    *   Đây là một **chu trình con sớm** (sub-cycle) không hợp lệ trong chu trình Hamiltonian đầy đủ.
    *   Đoạn logic này dẫn đến **mâu thuẫn** ngay lập tức, kích hoạt việc gán nhãn ngược lại cho cạnh đang probing.

---

## 3. Algorithm Steps
1.  **Chạy Phase 1 & 2** để rút gọn tối đa đồ thị đầu vào.
2.  **Kiểm tra ngân sách (Budget Check)**: Nếu số lượng cạnh chưa xác định $\le 300$, bắt đầu thực hiện Probing.
3.  **Vòng lặp Probing**:
    *   Với mỗi cạnh active $e$:
        *   Tạo bản sao tạm thời của đồ thị, tập `forced` và `forbidden`.
        *   Thực hiện gán nhãn tạm thời (Positive/Negative).
        *   Chạy **Degree Cascade** kết hợp **Sub-cycle check**.
        *   Nếu phát hiện mâu thuẫn, cập nhật nhãn chính thức của cạnh đó (Force/Forbid) trên đồ thị chính và kích hoạt lại vòng lặp Fixpoint.
4.  **Mã hóa và Giải**: Dựng công thức CRT trên đồ thị đã qua tiền xử lý 3 pha.

---

## 4. Evaluation & Insights
*   **Hiệu năng thực tế trên tập `fhcppp`**:
    *   Trên tập `fhcppp`, kết quả cho thấy số biến và mệnh đề của P1+P2 và P1+P2+P3 là trùng khớp nhau. Điều này chứng tỏ trên tập dữ liệu này, sau khi chạy co đường đi và cascade, giải thuật Probing không tìm thấy thêm cạnh mâu thuẫn nào mới.
    *   **Overhead**: Thời gian build tăng nhẹ (khoảng $0.1s$ đến $0.5s$ tùy kích thước đồ thị) do chi phí của việc nhân bản đồ thị và chạy DFS cascade liên tiếp.
*   **Kết luận**: Probing đặc biệt mạnh mẽ trên các đồ thị có bậc đỉnh đều bằng 3 (cubic graphs như tập `fhcpcs`), nơi chỉ cần cấm 1 cạnh lập tức ép buộc các cạnh còn lại của đỉnh đó $\rightarrow$ tạo ra chuỗi cascade lan truyền dài. Đối với đồ thị có mật độ lớn hoặc thưa ngẫu nhiên của `fhcppp`, Probing mang lại hiệu quả biên thấp hơn.
