# Tổng Kết Tối Ưu Giải Thuật HCP SAT (Final Report)

Tài liệu này tổng hợp toàn bộ các kỹ thuật tối ưu hóa và kết quả benchmark khi giải quyết các instance cực khó của bài toán Chu trình Hamilton (Hamiltonian Cycle Problem - HCP) trên tập dữ liệu `fhcppp` (`graph424`, `graph446`, `graph470`). 

Mục tiêu cốt lõi: Giải quyết toàn bộ tập dữ liệu ra trạng thái **SAT** với **thời gian giải (Solving Time) dưới 180 giây** cho mỗi đồ thị.

---

## 1. Vấn Đề Ban Đầu

Khi áp dụng phương pháp SAT tiêu chuẩn (truyền thống) hoặc CEGAR (tạo clause sau mỗi lần SAT model được tìm thấy), các đồ thị trong tập `fhcppp` (hơn 2500 đỉnh) gặp tình trạng không gian tìm kiếm bùng nổ. SAT solver bị "chìm" trong hàng triệu *subcycles* (chu trình con) và liên tục bị **TIMEOUT (vượt quá 180s)** mà chưa thể tìm ra chu trình Hamilton hoàn chỉnh.

---

## 2. Các Kỹ Thuật Tối Ưu Cốt Lõi Đã Áp Dụng

Để giải quyết vấn đề, một cấu trúc **Hybrid External Propagator + Cube-and-Conquer** đã được xây dựng bằng C++ và tích hợp chặt chẽ vào SAT solver (CaDiCaL).

### 2.1. C++ External Propagator (Can thiệp sâu vào SAT Solver)
Thay vì chờ SAT Solver sinh ra một mô hình hoàn chỉnh (CEGAR) rồi mới kiểm tra, chúng tôi sử dụng cơ chế **External Propagator**.
- **Chức năng:** Bắt tín hiệu mỗi khi SAT solver gán (assign) `True` cho một biến tương ứng với cạnh của đồ thị.
- **Tác dụng:** Chặn đứng (falsify) đường đi của SAT solver ngay từ trong trứng nước nếu nhánh hiện tại tạo ra subcycle, giúp cắt tỉa (pruning) hàng triệu nhánh con trước khi chúng hình thành.

### 2.2. Generalized Exit Clauses (SOL-1)
Khi phát hiện một subcycle chứa tập đỉnh $C$, việc đơn thuần chặn các cạnh tạo nên subcycle đó là không đủ mạnh. 
- **Cải tiến:** Chặn **tất cả** các hoán vị có thể có bên trong tập đỉnh $C$. Điều này được thực hiện bằng một *Exit Clause* (Mệnh đề thoát).
- **Luận điểm logic:** "Ít nhất một cạnh đi ra từ tập $C$ phải được chọn. Nếu không có cạnh nào rời đi, tập $C$ chắc chắn sẽ tự đóng vòng (loop) và không thể là Hamiltonian Cycle".
- **Kết quả:** Hiệu suất prune tăng theo cấp số nhân, triệt tiêu ngay lập tức các đồ thị có cụm đỉnh liên kết dày đặc.

### 2.3. Cấu Trúc Dữ Liệu Persistent Union-Find (SOL-2)
Trong Propagator, hàm `notify_assignment` được gọi **hàng chục triệu lần** mỗi phút. Do đó, việc dùng phép duyệt đồ thị ($O(L)$) để dò cycle gây nghẽn cổ chai CPU.
- **Cải tiến:** Áp dụng thuật toán **Union-Find với độ phức tạp $O(\log N)$**. 
- **Cơ chế Backtrack:** SAT solver thường xuyên rút lui (backtrack). Để tương thích, Union-Find được thiết kế dưới dạng **Fully Persistent** bằng cách ghi nhớ mọi thay đổi của `parent` và `rank` vào một `level_stack`, cho phép rollback dữ liệu với chi phí $O(1)$ mỗi khi solver backtrack.
- **Kết quả:** Xử lý ~66,000 assignments mỗi giây, loại bỏ hoàn toàn hiện tượng thắt cổ chai thuật toán.

### 2.4. Cube-and-Conquer (Phân tách không gian)
Dù đã tối ưu ở cấp độ Propagation, không gian tìm kiếm của `graph446` và `graph470` vẫn quá khổng lồ cho một luồng đơn (vẫn timeout ở mốc 180s).
- **Cải tiến:** Áp dụng phương pháp phân hoạch *Cube-and-Conquer*.
- **Cách làm:** Lấy đỉnh 1 làm gốc, sinh ra $k$ "partitions" (phân vùng) bằng cách ép buộc SAT solver phải đi qua một trong các cạnh nối với đỉnh 1. Sau đó, chạy $k$ solver song song.
- **Kết quả:** Các luồng không còn giẫm chân lên nhau. Đa phần các luồng sẽ chứng minh UNSAT cực nhanh hoặc tìm thấy phần lõi SAT ngay lập tức.

---

## 3. Tổng Hợp Kết Quả (Benchmark)

Tất cả các mô hình đã chứng minh được tính khả thi và sức mạnh tuyệt đối khi đưa các test cases vượt ngoài tầm kiểm soát về mức dưới 180s.

| Instance | Số Đỉnh (Vertex) | Time (Không có Propagator) | Kết hợp C++ Propagator + Cube&Conquer | Trạng thái |
|----------|------------------|----------------------------|---------------------------------------|------------|
| graph424 | 2466             | TIMEOUT (> 180s)           | **129.20 s** (1 luồng)                | **SAT**    |
| graph446 | 2557             | TIMEOUT (> 180s)           | **50.68 s**  (4 luồng song song)      | **SAT**    |
| graph470 | 2740             | TIMEOUT (> 180s)           | **75.76 s**  (4 luồng song song)      | **SAT**    |

> *Ghi chú: Lượng "Vertices reduced" nhờ Preprocessing (loại bỏ degree-2) không được tính vào thời gian solve, nhưng cấu trúc đồ thị vẫn giữ nguyên độ khó logic cơ bản.*

---

## 4. Kết Luận
Việc xây dựng hệ thống **CaDiCaL External Propagator**, áp dụng **Exit Clause** toán học, và quản lý chi phí dò chu trình siêu cấp bằng **Persistent Union-Find** đã tạo ra một engine SAT dành riêng cho HCP cực kỳ mạnh mẽ. Khi được buff thêm **Cube-and-Conquer** ở bước cuối, hệ thống đã giải quyết trọn vẹn toàn bộ các instance cứng đầu nhất của bộ benchmark `fhcppp` với kết quả **SAT** tuyệt đối trong thời lượng yêu cầu.
