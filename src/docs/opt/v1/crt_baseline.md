# CRT Baseline Approach

## 1. Overview
Phương pháp **CRT Baseline** là mô hình mã hóa Hamiltonian Cycle gốc sử dụng Định lý số dư Trung Hoa (Chinese Remainder Theorem) để biểu diễn vị trí thứ tự của các đỉnh trong chu trình.

*   **Ý tưởng cốt lõi**: Thay vì mã hóa trực tiếp vị trí của mỗi đỉnh bằng một số nguyên $1 \ldots N$ (đòi hỏi $N$ biến nhị phân ở mã hóa Unary), CRT Baseline phân rã vị trí này thành các số dư theo một tập các modulo đôi một nguyên tố cùng nhau $m_1, m_2, \dots, m_k$ có tích $M = \prod_{i=1}^k m_i \ge N$.
*   **Mức độ tiền xử lý**: Không sử dụng bất kỳ bước tiền xử lý đồ thị nào.
*   **Phương pháp mã hóa AMO**: Sử dụng `PbLibEncoder` để biểu diễn ràng buộc mỗi đỉnh có đúng 1 cung đi vào và 1 cung đi ra.

---

## 2. Mathematical Formulation

### 2.1 Biến quyết định (Decision Variables)
1.  **Biến cạnh ($H_{i,j}$)**: 
    $$H_{i,j} = \text{True} \iff \text{Cạnh đi từ } i \text{ sang } j \text{ nằm trong chu trình Hamiltonian.}$$
2.  **Biến vị trí ($P_{i,b}$)**:
    Biểu diễn bit thứ $b$ trong mã hóa nhị phân của số dư của vị trí đỉnh $i$ theo các moduli.
    Tổng số bit vị trí cho mỗi đỉnh là $B = \sum \lceil \log_2 m_i \rceil$.

### 2.2 Các nhóm ràng buộc (Constraints)

#### Ràng buộc Chu trình (Hamiltonian Constraints - AMO)
Mỗi đỉnh $i$ phải có đúng một cạnh đi ra và một cạnh đi vào hoạt động:
$$\sum_{j \in Adj_{out}(i)} H_{i,j} = 1 \quad \forall i \in V$$
$$\sum_{j \in Adj_{in}(i)} H_{j,i} = 1 \quad \forall i \in V$$

#### Ràng buộc Chuyển trạng thái Vị trí (Transition Constraints)
Với mỗi cạnh khả thi $i \to j$ (ngoại trừ đỉnh xuất phát $j \neq first$), nếu cạnh đó được chọn ($H_{i,j} = \text{True}$), thì vị trí của $j$ phải là giá trị kế tiếp của vị trí $i$ theo toán tử kế tiếp cục bộ $Succ$:
$$H_{i,j} \implies (P_{j} \equiv Succ(P_{i}))$$

Logic chuyển đổi vị trí nhị phân kế tiếp cho mỗi modulo $m$ được viết trực tiếp trên từng cạnh dưới dạng các mệnh đề CNF liên kết giữa các biến $H_{i,j}$, $P_{i,b}$ và $P_{j,b}$.

---

## 3. Algorithm Steps
1.  **Lựa chọn Moduli**: Tìm tập các số moduli nguyên tố cùng nhau $\{m_1, m_2, \dots\}$ sao cho tích của chúng lớn hơn hoặc bằng số đỉnh $N$.
2.  **Tạo biến**: Sinh các biến $H_{i,j}$ cho toàn bộ các cung trong đồ thị và biến vị trí $P_{i,b}$ cho toàn bộ các đỉnh.
3.  **Dựng ràng buộc AMO**: Áp dụng bộ mã hóa `PbLibEncoder` để sinh công thức SAT giới hạn bậc vào/ra bằng 1 cho toàn bộ đồ thị.
4.  **Dựng ràng buộc kế tiếp (Edge-centric Transitions)**: Duyệt qua từng cung $i \to j$ trong đồ thị để sinh các mệnh đề chuyển trạng thái modulo tương ứng.

---

## 4. Evaluation & Insights
*   **Ưu điểm**: Giảm số lượng biến từ $O(N^2)$ xuống $O(N \log N)$ so với mã hóa Unary, giúp tối ưu hóa không gian bộ nhớ cho các đồ thị trung bình và lớn.
*   **Nhược điểm**: 
    *   Do các ràng buộc modulo chuyển trạng thái được nhân bản trực tiếp trên từng cạnh, số lượng mệnh đề CNF tăng tỉ lệ thuận với số lượng cạnh $E$ của đồ thị ($O(E \log N)$ clauses). Đối với các đồ thị dày, điều này dẫn đến bùng nổ số lượng mệnh đề khổng lồ (Ví dụ: `graph162` có hơn 20.4 triệu mệnh đề).
    *   Không khai thác các đỉnh bậc thấp để tối ưu cấu trúc đồ thị trước khi giải.
