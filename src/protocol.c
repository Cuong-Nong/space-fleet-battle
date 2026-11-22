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

/*
 * Hàm phụ trợ: Nhận chính xác len byte dữ liệu từ socket.
 * - got_any: Cờ báo hiệu đã nhận được byte nào chưa.
 * Trả về:
 *   +  1: Nhận đủ len byte thành công.
 *   +  0: Bên kia đóng kết nối khi chưa nhận byte nào của lần gọi này.
 *   + -1: Lỗi socket hoặc bên kia đóng kết nối khi đang nhận dở dang.
 */
static int recv_all(int fd, void *buf, size_t len, int *got_any) {
    size_t total_recv = 0;
    char *ptr = (char *)buf;

    if (got_any != NULL) {
        *got_any = 0;
    }

    while (total_recv < len) {
        ssize_t n = recv(fd, ptr + total_recv, len - total_recv, 0);

        if (n < 0) {
            /* Bị ngắt bởi tín hiệu hệ thống, thử lại */
            if (errno == EINTR) {
                continue;
            }
            /* Lỗi socket thực sự */
            return -1;
        }

        if (n == 0) {
            /* Bên kia đã đóng kết nối (EOF) */
            if (total_recv == 0) {
                return 0; /* Chưa nhận byte nào thì đóng bình thường */
            }
            return -1;    /* Đã nhận dở dang thì coi là lỗi đứt kết nối */
        }

        total_recv += (size_t)n;
        if (got_any != NULL) {
            *got_any = 1;
        }
    }

    return 1; /* Nhận đủ len byte */
}

/*
 * Nhận một gói tin hoàn chỉnh từ socket.
 * Trả về:
 *   +  1: Thành công.
 *   +  0: Bên kia đóng kết nối bình thường trước khi truyền gói mới.
 *   + -1: Lỗi socket, payload_len sai hoặc đóng kết nối giữa chừng.
 */
int recv_packet(int fd, Packet *pkt) {
    if (pkt == NULL) {
        return -1;
    }

    /* 1. Đọc đủ 8 byte header */
    PacketHeader net_header;
    int got_any = 0;
    int ret = recv_all(fd, &net_header, sizeof(net_header), &got_any);

    if (ret == 0) {
        /* Bên kia đóng kết nối trước khi gửi bất kỳ byte nào */
        return 0;
    }
    if (ret < 0) {
        /* Lỗi socket hoặc đóng giữa chừng khi nhận header */
        return -1;
    }

    /* 2. Chuyển đổi từ Network Byte Order sang Host Byte Order */
    pkt->header.payload_len = ntohl(net_header.payload_len);
    pkt->header.command_type = ntohl(net_header.command_type);

    /* 3. Kiểm tra an toàn: payload_len phải nằm trong khoảng [0, MAX_LEN] */
    if (pkt->header.payload_len < 0 || pkt->header.payload_len > MAX_LEN) {
        return -1; /* Không đọc tiếp để chống tràn bộ đệm */
    }

    /* 4. Đọc đúng payload_len byte nếu có payload */
    if (pkt->header.payload_len > 0) {
        int payload_got_any = 0;
        int payload_ret = recv_all(fd, pkt->payload, (size_t)pkt->header.payload_len, &payload_got_any);

        if (payload_ret != 1) {
            /* Lỗi hoặc bên kia đóng kết nối khi đang gửi dở payload */
            return -1;
        }
    }

    /* 5. Luôn thêm ký tự kết thúc chuỗi '\0' an toàn */
    pkt->payload[pkt->header.payload_len] = '\0';

    return 1;
}
