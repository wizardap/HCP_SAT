# Vertex-Centric CRT with Preprocessing (VCCRT Preprocessed)

## 1. Overview
Phương pháp **VCCRT Preprocessed** đại diện cho sự kết hợp mạnh mẽ nhất (Phase 4 kết hợp đầy đủ tiền xử lý) trong dự án HCP-SAT. 

*   **Ý tưởng cốt lõi**: Tận dụng toàn bộ sức mạnh rút gọn cấu trúc đồ thị từ các pha tiền xử lý (**P1: Degree Cascade**, **P2: Path Contraction & Cuts**, **P3: Probing**) để tạo ra đồ thị rút gọn tối đa $G'$ với số đỉnh $N' < N$ và số cạnh $E' < E$. Sau đó, áp dụng mô hình mã hóa hướng đỉnh **VCCRT** trên đồ thị rút gọn này.
*   **Sự kết hợp tối ưu (Synergy)**:
    *   Tiền xử lý đồ thị triệt tiêu các đỉnh bậc thấp, co chuỗi đỉnh bậc 2 và loại bỏ các cạnh không khả thi.
    *   Mô hình VCCRT giảm thiểu số lượng mệnh đề logic chuyển trạng thái trên các cạnh active còn lại của đồ thị rút gọn.
    *   Kết quả thu được là công thức CNF có kích thước tối giản nhất ở cả số lượng biến và số lượng mệnh đề.

---

## 2. Mathematical & Logical Formulation

### 2.1 Tiền xử lý đồ thị
Đồ thị gốc $G = (V, E)$ đi qua bộ tiền xử lý `HCPPreprocessor` để sinh ra kết quả `PreprocessResult`:
$$G' = (V', E') \quad \text{với } |V'| = N' < N, \ |E'| = E' < E$$
Bộ tiền xử lý cũng xác định danh sách các cạnh chắc chắn chọn (`forced_edges`) và các đường đi co đỉnh (`contracted_paths`).

### 2.2 Mã hóa VCCRT trên Đồ thị rút gọn
1.  **Biến vị trí kế tiếp ($S_{i,b}$)**: Chỉ sinh cho các đỉnh hoạt động $i \in V'$ trong đồ thị rút gọn.
2.  **Ràng buộc chuyển vị trí cục bộ**: 
    $$S_i \equiv Succ(P_i) \quad \forall i \in V'$$
    Chỉ cần sinh $N'$ nhóm mệnh đề chuyển đổi (ít hơn nhiều so với $N$ của bản Baseline).
3.  **Ràng buộc sao chép cạnh**: Chỉ sinh cho các cạnh active $E'$ của đồ thị rút gọn:
    $$H_{i,j} \implies (P_{j,b} \leftrightarrow S_{i,b}) \quad \forall (i,j) \in E'$$
4.  **Cố định các cạnh bắt buộc**: Ép buộc trực tiếp các biến tương ứng với các cạnh trong `forced_edges` và `contracted_paths` thành True trong SAT solver dưới dạng mệnh đề đơn vị.

---

## 3. Algorithm Steps
1.  **Tiền xử lý đồ thị**: Chạy lặp Fixpoint (Cascade $\rightarrow$ Cuts $\rightarrow$ Contraction) và Probing trên đồ thị đầu vào.
2.  **Dựng mô hình VCCRT rút gọn**: 
    *   Nếu phát hiện UNSAT sớm $\implies$ Dừng và trả về UNSAT.
    *   Ngược lại, sinh các biến $H_{i,j}$, $P_{i,b}$, $S_{i,b}$ cho đồ thị rút gọn $G'$.
3.  **Xây dựng ràng buộc**:
    *   Áp dụng ràng buộc chuyển vị trí cục bộ trên $N'$ đỉnh của $G'$.
    *   Áp dụng ràng buộc copy trên các cạnh của $G'$.
    *   Áp dụng ràng buộc AMO bằng `PbLibEncoder` trên $G'$.
    *   Nạp các unit clauses cố định các cạnh đã được ép buộc từ bước tiền xử lý.
4.  **Khôi phục lời giải**: Sau khi solver trả về SAT, khôi phục chu trình Hamiltonian gốc bằng cách giải nén các cạnh co đỉnh trong `contracted_paths`.

---

## 4. Evaluation & Insights
*   **Hiệu năng thực nghiệm trên tập `fhcppp`**:
    *   **Tối ưu hóa tổng thể**: `VCCRT Preprocessed` đạt kích thước công thức CNF tối thiểu. Trên `graph223`, số biến giảm còn **876,620** và số mệnh đề giảm chỉ còn **993,940** clauses (so với 2.1M clauses của Baseline).
    *   **Tốc độ giải tối ưu nhất**: Trên `graph223`, thời gian giải tối ưu nhất đạt **18.753s** (nhanh hơn so với VCCRT Baseline là 22.855s, và nhanh hơn rất nhiều so với CRT Baseline là 135.825s).
    *   **Tăng tốc trên `graph255`**: Rút ngắn thời gian giải từ **33.593s** (VCCRT Baseline) xuống còn **5.103s** nhờ cắt giảm một nửa số lượng mệnh đề.
*   **Kết luận**: Đây là cách tiếp cận toàn diện nhất của dự án. VCCRT Preprocessed kết hợp hài hòa giữa việc tối ưu hóa cấu trúc dữ liệu đồ thị (Graph-level) và tối ưu hóa biểu diễn logic Boolean (CNF-level), mang lại tính ổn định và khả năng mở rộng cao nhất cho các bài toán Hamiltonian Cycle quy mô lớn.
