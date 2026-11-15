# Phi Đội Không Gian (Space Fleet Battle)

## 1. Môi trường phát triển
- Hệ điều hành: **Ubuntu 22.04 LTS / WSL**
- Trình biên dịch: **gcc** (chuẩn gnu11)
- Biên dịch flags: `-Wall -Wextra -g -pthread`
- Thư viện: Chỉ sử dụng thư viện chuẩn C và POSIX (không dùng thư viện ngoài).

---

## 2. Cấu trúc thư mục
- `src/`: Thư mục chứa toàn bộ mã nguồn (`.c`, `.h`).
- `logs/`: Thư mục lưu trữ file log khi ứng dụng chạy.
- `Makefile`: Script tự động hóa biên dịch, dọn dẹp và chạy kiểm thử.

---

## 3. Hướng dẫn biên dịch
- Biên dịch toàn bộ ứng dụng (server và client):
  ```bash
  make all
  ```
- Biên dịch và chạy bộ kiểm thử tự động (test_protocol, test_logger):
  ```bash
  make test
  ```
- Dọn dẹp các file đối tượng (`.o`) và file thực thi:
  ```bash
  make clean
  ```

---

## 4. Cách chạy dự kiến
- Khởi chạy Server:
  ```bash
  ./server <port>
  # Ví dụ: ./server 8080
  ```
- Khởi chạy Client:
  ```bash
  ./client <IP> <port>
  # Ví dụ: ./client 127.0.0.1 8080
  ```
