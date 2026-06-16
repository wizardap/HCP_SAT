# Vertex-Centric CRT (VCCRT) Baseline

## 1. Overview
Phương pháp **VCCRT Baseline** là cách tiếp cận hướng đỉnh (Vertex-Centric) cải tiến từ mô hình mã hóa CRT truyền thống, tối ưu hóa sâu cách thức biểu diễn logic chuyển vị trí trong SAT.

*   **Ý tưởng cốt lõi**: Trong CRT truyền thống (Edge-Centric), logic chuyển đổi vị trí ($P_j = Succ(P_i)$) được nhân bản trực tiếp cho từng cạnh $i \to j$. Đối với đồ thị có mật độ cạnh trung bình/cao, điều này tạo ra số mệnh đề cực kỳ lớn. 
*   VCCRT tách biệt quá trình này: tính toán trước giá trị vị trí kế tiếp $S_{i}$ (Successor) cục bộ tại mỗi đỉnh $i$ độc lập với các cạnh, sau đó chỉ thực hiện sao chép giá trị vị trí từ đỉnh này sang đỉnh khác nếu cạnh nối giữa chúng được chọn ($H_{i,j} = \text{True}$).
*   **Mức độ tiền xử lý**: Không sử dụng bất kỳ bước tiền xử lý đồ thị nào.

---

## 2. Mathematical & Logical Formulation

### 2.1 Biến quyết định mới
Bên cạnh biến cạnh $H_{i,j}$ và biến vị trí $P_{i,b}$, VCCRT giới thiệu thêm các biến phụ trạng thái kế tiếp:
*   **Biến vị trí kế tiếp ($S_{i,b}$)**: Biểu diễn bit thứ $b$ của giá trị vị trí ngay sau đỉnh $i$ trong chu trình.

### 2.2 Các nhóm ràng buộc

#### Ràng buộc Chuyển đổi trạng thái cục bộ tại Đỉnh (Local Transitions)
Với mỗi đỉnh $i \in V$, định nghĩa giá trị của trạng thái kế tiếp $S_i$ là hàm kế tiếp $Succ$ của vị trí hiện tại $P_i$:
$$S_i \equiv Succ(P_i) \quad \forall i \in V$$
Công thức logic chuyển đổi nhị phân (cộng 1 modulo) này chỉ được sinh đúng 1 lần cho mỗi đỉnh, thay vì sinh cho mỗi cạnh như trước. Số lượng mệnh đề sinh ra cho phần này tối ưu ở mức $O(N \log N)$.

#### Ràng buộc Sao chép trạng thái qua Cạnh (Copy Constraints)
Với mỗi cạnh khả thi $i \to j$ (ngoại trừ đỉnh xuất phát $j \neq first$), nếu cạnh đó được chọn ($H_{i,j} = \text{True}$), ta sao chép trạng thái kế tiếp từ $i$ sang vị trí hiện tại của $j$:
$$H_{i,j} \implies (P_{j,b} \leftrightarrow S_{i,b}) \quad \forall b$$
Ràng buộc này chuyển đổi thành các mệnh đề CNF cực kỳ đơn giản (mệnh đề tương đương nhị phân có độ dài 3):
$$(\neg H_{i,j} \lor P_{j,b} \lor \neg S_{i,b})$$
$$(\neg H_{i,j} \lor \neg P_{j,b} \lor S_{i,b})$$

---

## 3. Algorithm Steps
1.  **Tạo biến**: Sinh biến $H_{i,j}$, biến vị trí hiện tại $P_{i,b}$, và biến vị trí kế tiếp $S_{i,b}$.
2.  **Dựng logic chuyển vị trí cục bộ**: Duyệt qua từng đỉnh $i \in V$ để xây dựng các ràng buộc $S_i = Succ(P_i)$.
3.  **Dựng logic sao chép cạnh**: Duyệt qua từng cung $i \to j$ trong đồ thị, sinh các ràng buộc copy $H_{i,j} \implies (P_j = S_i)$.
4.  **Dựng ràng buộc AMO**: Sử dụng `PbLibEncoder` tương tự như CRT Baseline.

---

## 4. Evaluation & Insights
*   **Ưu điểm**:
    *   **Giảm số lượng mệnh đề CNF vượt trội**: Nhờ chuyển phần logic chuyển trạng thái modulo phức tạp về mức đỉnh ($O(N \log N)$), số lượng mệnh đề trên mỗi cạnh chỉ còn là logic sao chép bit đơn giản ($O(E \log N)$ clauses với hằng số nhân nhỏ hơn rất nhiều).
    *   *Ví dụ*: Trên đồ thị `graph162`, số mệnh đề của VCCRT Baseline chỉ là **12.2 triệu clauses**, giảm tới **40%** so với **20.4 triệu clauses** của CRT Baseline.
    *   **Tốc độ giải đột phá**: Trên đồ thị khó `graph223`, VCCRT Baseline chỉ mất **22.855s** để tìm được chu trình, nhanh gấp **6 lần** so với CRT Baseline (**135.825s**).
*   **Nhược điểm**: Tăng thêm $N \times nBits$ biến phụ nhị phân ($S_{i,b}$). Tuy nhiên, các SAT solver hiện đại xử lý biến phụ rất tốt và việc giảm mệnh đề mang lại lợi ích lớn hơn nhiều.
