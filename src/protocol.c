#include "protocol.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>

/*
 * Hàm phụ trợ: Gửi chính xác len byte dữ liệu qua socket.
 * Do TCP là luồng byte, send() có thể chỉ gửi được một phần dữ liệu (partial send)
 * tùy thuộc vào bộ đệm của hệ điều hành, nên cần vòng lặp để gửi nốt phần còn lại.
 */
static int send_all(int fd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char *)buf;

    while (total_sent < len) {
        /* MSG_NOSIGNAL ngăn hệ điều hành phát tín hiệu SIGPIPE làm sập tiến trình nếu bên kia ngắt kết nối */
        ssize_t bytes_sent = send(fd, ptr + total_sent, len - total_sent, MSG_NOSIGNAL);

        if (bytes_sent < 0) {
            /* Nếu bị ngắt bởi tín hiệu hệ thống (signal interrupt), thử lại */
            if (errno == EINTR) {
                continue;
            }
            /* Lỗi socket thực sự */
            return -1;
        }

        if (bytes_sent == 0) {
            /* Đối phương đã đóng kết nối */
            return -1;
        }

        total_sent += (size_t)bytes_sent;
    }

    return (int)total_sent;
}

/*
 * Gửi một gói tin hoàn chỉnh gồm Header (8 byte) và Payload.
 */
int send_packet(int fd, const Packet *pkt) {
    if (pkt == NULL) {
        return -1;
    }

    /* Kiểm tra độ dài payload hợp lệ trong khoảng [0, MAX_LEN] */
    if (pkt->header.payload_len < 0 || pkt->header.payload_len > MAX_LEN) {
        return -1;
    }

    /*
     * Chuyển đổi các trường trong header sang Network Byte Order (Big-Endian)
     * vào một biến tạm để tránh làm thay đổi dữ liệu trong gói tin gốc (*pkt).
     */
    PacketHeader net_header;
    net_header.payload_len = htonl(pkt->header.payload_len);
    net_header.command_type = htonl(pkt->header.command_type);

    /* 1. Gửi Header 8 byte */
    if (send_all(fd, &net_header, sizeof(net_header)) < 0) {
        return -1;
    }

    /* 2. Gửi Payload nếu có dữ liệu */
    if (pkt->header.payload_len > 0) {
        if (send_all(fd, pkt->payload, (size_t)pkt->header.payload_len) < 0) {
            return -1;
        }
    }

    /* Trả về tổng số byte thực tế đã gửi qua mạng */
    return (int)(sizeof(net_header) + (size_t)pkt->header.payload_len);
}
