#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Tạo socket lắng nghe trên 127.0.0.1, cổng 0 để OS tự cấp cổng rảnh */
static int create_listener(int *out_port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; /* Port 0: hệ điều hành tự chọn cổng trống */

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    socklen_t addr_len = sizeof(addr);
    if (getsockname(listen_fd, (struct sockaddr *)&addr, &addr_len) < 0) {
        perror("getsockname");
        close(listen_fd);
        return -1;
    }

    *out_port = ntohs(addr.sin_port);
    return listen_fd;
}

/* Kết nối socket tới cổng loopback chỉ định */
static int connect_to_port(int port) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket client");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(client_fd);
        return -1;
    }

    return client_fd;
}

/* =========================================================================
 * Ca 1: Gửi liền 3 gói khác nhau (thử nghiệm "dính gói")
 * ========================================================================= */
static void *sender_case1(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    Packet p1, p2, p3;
    packet_set(&p1, CMD_REGISTER, "alice|secret123");
    packet_set(&p2, CMD_LOGIN, "bob|pass456");
    packet_set(&p3, CMD_LOGOUT, "");

    /* Gửi liên tiếp 3 gói tin */
    send_packet(fd, &p1);
    send_packet(fd, &p2);
    send_packet(fd, &p3);

    close(fd);
    return NULL;
}

static int test_case1(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case1, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r1, r2, r3;
    int ok1 = (recv_packet(recv_fd, &r1) == 1 &&
               r1.header.command_type == CMD_REGISTER &&
               strcmp(r1.payload, "alice|secret123") == 0);

    int ok2 = (recv_packet(recv_fd, &r2) == 1 &&
               r2.header.command_type == CMD_LOGIN &&
               strcmp(r2.payload, "bob|pass456") == 0);

    int ok3 = (recv_packet(recv_fd, &r3) == 1 &&
               r3.header.command_type == CMD_LOGOUT &&
               strcmp(r3.payload, "") == 0);

    close(recv_fd);
    pthread_join(tid, NULL);
    return (ok1 && ok2 && ok3);
}

/* =========================================================================
 * Ca 2: Bên gửi ghi từng 1-3 byte bằng send() thô kèm usleep (thử "tách gói")
 * ========================================================================= */
static void *sender_case2(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    const char *payload_text = "target_alpha|weapon_laser";
    size_t payload_len = strlen(payload_text);

    PacketHeader hdr;
    hdr.payload_len = htonl((int32_t)payload_len);
    hdr.command_type = htonl(CMD_ATTACK);

    size_t total_bytes = sizeof(hdr) + payload_len;
    char buffer[sizeof(hdr) + 256];
    memcpy(buffer, &hdr, sizeof(hdr));
    memcpy(buffer + sizeof(hdr), payload_text, payload_len);

    /* Gửi ngắt quãng từng 1-3 byte một */
    size_t sent = 0;
    while (sent < total_bytes) {
        size_t chunk = (total_bytes - sent >= 3) ? (sent % 3 + 1) : (total_bytes - sent);
        send(fd, buffer + sent, chunk, 0);
        sent += chunk;
        usleep(2000); /* Chờ 2ms để bên nhận gặp trường hợp gói bị tách */
    }

    close(fd);
    return NULL;
}

static int test_case2(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case2, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r;
    int ret = recv_packet(recv_fd, &r);
    int ok = (ret == 1 &&
              r.header.command_type == CMD_ATTACK &&
              r.header.payload_len == (int32_t)strlen("target_alpha|weapon_laser") &&
              strcmp(r.payload, "target_alpha|weapon_laser") == 0);

    close(recv_fd);
    pthread_join(tid, NULL);
    return ok;
}

/* =========================================================================
 * Ca 3: Payload rỗng (payload_len = 0)
 * ========================================================================= */
static void *sender_case3(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    Packet p;
    packet_set(&p, CMD_START_GAME, "");
    send_packet(fd, &p);

    close(fd);
    return NULL;
}

static int test_case3(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case3, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r;
    int ret = recv_packet(recv_fd, &r);
    int ok = (ret == 1 &&
              r.header.command_type == CMD_START_GAME &&
              r.header.payload_len == 0 &&
              r.payload[0] == '\0');

    close(recv_fd);
    pthread_join(tid, NULL);
    return ok;
}

/* =========================================================================
 * Ca 4: Payload dài đúng MAX_LEN byte (1024 byte)
 * ========================================================================= */
static void *sender_case4(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    Packet p;
    p.header.command_type = CMD_NOTIFY;
    p.header.payload_len = MAX_LEN;
    memset(p.payload, 'A', MAX_LEN);
    p.payload[MAX_LEN] = '\0';

    send_packet(fd, &p);

    close(fd);
    return NULL;
}

static int test_case4(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case4, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    char expected[MAX_LEN + 1];
    memset(expected, 'A', MAX_LEN);
    expected[MAX_LEN] = '\0';

    Packet r;
    int ret = recv_packet(recv_fd, &r);
    int ok = (ret == 1 &&
              r.header.command_type == CMD_NOTIFY &&
              r.header.payload_len == MAX_LEN &&
              memcmp(r.payload, expected, MAX_LEN) == 0 &&
              r.payload[MAX_LEN] == '\0');

    close(recv_fd);
    pthread_join(tid, NULL);
    return ok;
}

/* =========================================================================
 * Ca 5: Header ghi tay có payload_len = MAX_LEN + 1 và payload_len âm -> trả -1
 * ========================================================================= */
static void *sender_case5(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    /* Gửi header có payload_len = MAX_LEN + 1 */
    PacketHeader hdr1;
    hdr1.payload_len = htonl(MAX_LEN + 1);
    hdr1.command_type = htonl(CMD_BUY_ITEM);
    send(fd, &hdr1, sizeof(hdr1), 0);

    /* Chờ một chút trước khi gửi tiếp sub-case */
    usleep(5000);

    /* Gửi header có payload_len âm (-1) */
    PacketHeader hdr2;
    hdr2.payload_len = htonl(-1);
    hdr2.command_type = htonl(CMD_BUY_ITEM);
    send(fd, &hdr2, sizeof(hdr2), 0);

    close(fd);
    return NULL;
}

static int test_case5(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case5, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r1, r2;
    /* Cả 2 lần đều phải trả về -1 do payload_len không hợp lệ */
    int ret1 = recv_packet(recv_fd, &r1);
    int ret2 = recv_packet(recv_fd, &r2);

    int ok = (ret1 == -1 && ret2 == -1);

    close(recv_fd);
    pthread_join(tid, NULL);
    return ok;
}

/* =========================================================================
 * Ca 6: Bên gửi close() ngay sau 1 gói: nhận gói đó rồi lần sau trả 0
 * ========================================================================= */
static void *sender_case6(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    Packet p;
    packet_set(&p, CMD_LEAVE_TEAM, "");
    send_packet(fd, &p);

    /* Đóng kết nối ngay lập tức */
    close(fd);
    return NULL;
}

static int test_case6(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case6, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r1, r2;
    int ret1 = recv_packet(recv_fd, &r1);
    int ok1 = (ret1 == 1 && r1.header.command_type == CMD_LEAVE_TEAM);

    /* Lần nhận tiếp theo bên kia đã đóng kết nối sạch sẽ nên trả 0 */
    int ret2 = recv_packet(recv_fd, &r2);
    int ok2 = (ret2 == 0);

    close(recv_fd);
    pthread_join(tid, NULL);
    return (ok1 && ok2);
}

/* =========================================================================
 * Ca 7: Bên gửi chỉ gửi 5 byte đầu của header rồi close() -> trả -1
 * ========================================================================= */
static void *sender_case7(void *arg) {
    int port = *(int *)arg;
    int fd = connect_to_port(port);
    if (fd < 0) return NULL;

    char partial_hdr[5] = {0x00, 0x00, 0x00, 0x04, 0x00};
    send(fd, partial_hdr, 5, 0);

    /* Đóng giữa chừng khi chưa gửi đủ 8 byte header */
    close(fd);
    return NULL;
}

static int test_case7(void) {
    int port = 0;
    int listen_fd = create_listener(&port);
    if (listen_fd < 0) return 0;

    pthread_t tid;
    pthread_create(&tid, NULL, sender_case7, &port);

    int recv_fd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (recv_fd < 0) {
        pthread_join(tid, NULL);
        return 0;
    }

    Packet r;
    int ret = recv_packet(recv_fd, &r);
    /* Đóng kết nối giữa chừng gói tin phải trả về -1 */
    int ok = (ret == -1);

    close(recv_fd);
    pthread_join(tid, NULL);
    return ok;
}

/* =========================================================================
 * Hàm main chạy toàn bộ 7 ca kiểm thử
 * ========================================================================= */
int main(void) {
    printf("============================================================\n");
    printf("   KIỂM THỬ GIAO THỨC (PROTOCOL TEST - TCP LOOPBACK)\n");
    printf("============================================================\n");

    int total_tests = 7;
    int passed_tests = 0;

    struct {
        int (*func)(void);
        const char *name;
    } cases[] = {
        {test_case1, "Ca 1: Gui lien tiep 3 goi (thu nghiem dinh goi)"},
        {test_case2, "Ca 2: Gui ngat quang 1-3 byte voi usleep (thu nghiem tach goi)"},
        {test_case3, "Ca 3: Payload rong (payload_len = 0)"},
        {test_case4, "Ca 4: Payload dai dung MAX_LEN (1024 byte)"},
        {test_case5, "Ca 5: payload_len > MAX_LEN hoac am (phai tra -1)"},
        {test_case6, "Ca 6: Ben gui dong ket noi sau 1 goi du (nhan du roi tra 0)"},
        {test_case7, "Ca 7: Ben gui dong sau khi gui 5 byte header (phai tra -1)"}
    };

    for (int i = 0; i < total_tests; i++) {
        int res = cases[i].func();
        if (res) {
            printf("[PASS] %s\n", cases[i].name);
            passed_tests++;
        } else {
            printf("[FAIL] %s\n", cases[i].name);
        }
    }

    printf("============================================================\n");
    printf("Ket qua: %d/%d ca kiem thu THANH CONG (PASS).\n", passed_tests, total_tests);
    printf("============================================================\n");

    if (passed_tests == total_tests) {
        return 0;
    } else {
        return 1;
    }
}
