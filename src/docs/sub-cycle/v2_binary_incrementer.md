# Kế hoạch Tối ưu hóa Chặn chu trình con (Sub-cycle Breaking) - V2 (Đã qua Review)

## 1. Bối cảnh và Mục tiêu
- **Vấn đề hiện tại:** Phương pháp Chinese Remainder Theorem (CRT) và LFSR hoạt động rất tốt trên đồ thị thưa, nhưng gặp điểm nghẽn "Clause Explosion" (Bùng nổ mệnh đề) trên tập đồ thị dày `fhcppp`. Việc mã hóa đẳng thức chuyển trạng thái ($P_v \equiv P_u + 1$) qua nhiều hệ modulo đòi hỏi hàng triệu mệnh đề, làm SAT Solver (CaDiCaL) bị quá tải.
- **Mục tiêu:** Phát triển một phương pháp mã hóa nhẹ hơn về mặt CNF clauses để lách qua điểm nghẽn tài nguyên trên đồ thị dày, đồng thời không đánh đổi khả năng cắt tỉa nhánh (pruning) sớm của BCP.

## 2. Hướng tiếp cận (Mới): Dynamic Exact Binary Incrementer
Phương pháp ban đầu (MTZ Comparator) đã bị loại bỏ qua quá trình **Multi-Agent Brainstorming** do vướng phải bẫy Pigeonhole Principle và mất khả năng lan truyền ràng buộc (BCP). Giải pháp được chốt là sử dụng một **Bộ đếm nhị phân chính xác với mạch nhớ (Carry-Chain)**.

- **Lý thuyết:** Ta cấp cho mỗi đỉnh một số nguyên nhị phân $U_v$. Nếu cạnh $(u, v)$ được chọn, ta ép buộc $U_v = U_u + 1$. Bằng cách dùng mạch cộng có Carry, ta mô phỏng lại các chu kỳ cục bộ nhỏ (bit 0 lật mỗi bước, bit 1 lật mỗi 2 bước) giúp CDCL solver nhận diện xung đột sớm như CRT, nhưng chỉ cần số lượng clause cực nhỏ.
- **Chi tiết kỹ thuật:**
  1. Tự động tính toán số bit cần thiết $K = \lceil \log_2 V \rceil$ để tránh lỗi tràn số sinh ra kết quả UNSAT sai (đề xuất từ User Advocate).
  2. Mỗi đỉnh $v$ được cấp $K$ biến nhị phân đại diện cho $U_v$.
  3. Với mỗi cạnh $u \to v$ (v $\neq$ Start Node), tạo mạch cộng:
     - Dùng các biến phụ (auxiliary variables) $C_k$ cho carry: $C_k = C_{k-1} \wedge U_{u, k}$
     - Mệnh đề cộng: $U_{v, k} = U_{u, k} \oplus C_{k-1}$
     - Chi phí cực rẻ: khoảng $\sim 30$ clauses cho mỗi cạnh.

## 3. Ưu điểm mong đợi (So với CRT & MTZ)
- **Giảm dung lượng CNF (Vs CRT):** Mạch cộng nhị phân loại bỏ sự trùng lặp của nhiều hệ modulo. Tổng số mệnh đề dự kiến giảm từ $\sim 10$ triệu xuống khoảng $\sim 2-3$ triệu.
- **Không vướng Pigeonhole (Vs MTZ):** Dùng phương trình chính xác ($=$) thay vì bất đẳng thức ($>$) đảm bảo solver không bị lạc trong không gian tìm kiếm khổng lồ do đoán sai khoảng cách.
- **Pruning hiệu quả (Vs Single LFSR):** Không dùng XOR-chain giả ngẫu nhiên, mạch cộng bảo toàn cấu trúc chu kỳ của từng bit, giúp BCP cắt nhánh tốt hơn rất nhiều.
- **Dễ debug (Vs CRT):** Giá trị $U_v$ khi giải ra chính là số thứ tự của đỉnh trên đường đi Hamiltonian!

## 4. Các bước triển khai (Implementation Steps)
1. **[X] Brainstorming & Review:** Đã chốt hướng thiết kế qua Multi-Agent Review.
2. **[ ] Tạo class encoding mới:** Viết file `src/libs/models/successor/binary_incrementer.py` kế thừa từ `SuccessorMethod`.
3. **[ ] Xây dựng mạch Adder:** Cài đặt hàm tạo SAT clauses cho phép toán $U_v = U_u + 1$ bằng mạch Carry-chain.
4. **[ ] Tính toán Bit-width động:** Đảm bảo $K = \lceil \log_2 V \rceil$.
5. **[ ] Tích hợp vào Pipeline:** Thêm encoding mới vào logic chạy benchmark.
6. **[ ] Đánh giá:** Chạy thử trên `graph424`, so sánh trực tiếp số Variables, Clauses và Runtime với `crt_preprocessed`.
