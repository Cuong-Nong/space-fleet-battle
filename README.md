# Phi đội không gian (Space Fleet Battle)

## 1. Giới thiệu
Game chiến thuật "Phi đội không gian" chạy trên nền tảng Linux (CLI):
- Giao thức truyền thông: TCP Socket.
- Kiến trúc Server: I/O multiplexing với `select()`.
- Kiến trúc Client: CLI đa luồng (`pthread`) - 1 luồng nhận dữ liệu, 1 luồng gửi/xử lý nhập liệu người dùng.

## 2. Môi trường phát triển
- Hệ điều hành: Linux / Ubuntu 22.04 LTS (WSL).
- Trình biên dịch: `gcc` (flags: `-Wall -Wextra -g -pthread`).
- Công cụ build: GNU Make.

## 3. Cấu trúc thư mục
- `src/`: Mã nguồn của dự án (header, logic mạng, logger, client, server).
- `logs/`: Lưu trữ các file log khi chạy ứng dụng.
- `Makefile`: Kịch bản biên dịch và kiểm thử tự động.

## 4. Hướng dẫn biên dịch & chạy
- Biên dịch toàn bộ (Server & Client):
  ```bash
  make all
  ```
- Biên dịch bài kiểm thử giao thức (Test Protocol):
  ```bash
  make test
  ```
- Dọn dẹp các file nhị phân và object files:
  ```bash
  make clean
  ```
