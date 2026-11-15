#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * Quy ước giao thức:
 * - Header 8 byte (2 số int32_t theo network byte order): payload_len, command_type.
 * - Payload: chuỗi text (tối đa 1024 byte), các trường phân tách bằng ký tự '|'.
 * - Giá trị trả về của send_packet / recv_packet:
 *     > 0: Thành công (số byte đã gửi/nhận).
 *       0: Bên kia đóng kết nối bình thường (chỉ với recv_packet).
 *     < 0: Lỗi (lỗi socket, payload_len không hợp lệ, hoặc bên kia đóng giữa chừng).
 */

#include <stdint.h>

#define MAX_LEN 1024

/* Danh sách mã lệnh giữa Client và Server */
typedef enum {
    /* Đăng nhập & phiên */
    CMD_REGISTER = 1,       /* C->S: "username|password" */
    CMD_LOGIN,              /* C->S: "username|password" */
    CMD_LOGOUT,             /* C->S: "" */

    /* Quản lý đội */
    CMD_CREATE_TEAM,        /* C->S: "team_name" */
    CMD_DELETE_TEAM,        /* C->S: "team_id" */
    CMD_LIST_TEAMS,         /* C->S: "" */
    CMD_LIST_MEMBERS,       /* C->S: "team_id" */
    CMD_JOIN_REQUEST,       /* C->S: "team_id" (server chuyển tiếp cho đội trưởng) */
    CMD_JOIN_APPROVE,       /* C->S: "username|1 hoặc 0" (đội trưởng duyệt) */
    CMD_INVITE,             /* C->S: "username" (đội trưởng mời) */
    CMD_INVITE_ACCEPT,      /* C->S: "team_id|1 hoặc 0" (người được mời trả lời) */
    CMD_LEAVE_TEAM,         /* C->S: "" */
    CMD_KICK_MEMBER,        /* C->S: "username" */

    /* Thách đấu & trận đấu */
    CMD_CHALLENGE,          /* C->S: "target_team_id" */
    CMD_CHALLENGE_ACCEPT,   /* C->S: "challenger_team_id|1 hoặc 0" */
    CMD_START_GAME,         /* C->S: "" */

    /* Tương tác trong game */
    CMD_BROADCAST_CHEST,    /* S->C: "chest_id|loại(COPPER/SILVER/GOLD)|câu hỏi" */
    CMD_CHEST_ANSWER,       /* C->S: "chest_id|đáp án" */
    CMD_BUY_ITEM,           /* C->S: "item_code" (1: thùng đạn pháo 30mm, 2: pháo laze, 3: bộ nguồn laze, 4: tên lửa, 5: giáp cơ bản, 6: giáp tăng cường) */
    CMD_ATTACK,             /* C->S: "target_username|weapon_code" (1: pháo 30mm, 2: laze, 3: tên lửa) */
    CMD_REPAIR,             /* C->S: "hp_amount" */
    CMD_STATE_UPDATE,       /* S->C: "username|hp|armor|coin" */
    CMD_GAME_OVER,          /* S->C: "winning_team_id" */

    /* Thông báo & phản hồi */
    CMD_NOTIFY,             /* S->C: "nội dung thông báo tự do" */
    CMD_OK,                 /* S->C: "<mã> <TEXT>[|dữ liệu kèm theo]" */
    CMD_ERROR               /* S->C: "<mã> <TEXT>[|dữ liệu kèm theo]" */
} CommandType;

/* Mã phản hồi từ Server */
#define RC_OK                 100  /* OK */
#define RC_INVALID_FORMAT     200  /* INVALID FORMAT */
#define RC_USERNAME_EXISTS    201  /* USERNAME EXISTS */
#define RC_WRONG_CREDENTIALS  202  /* WRONG CREDENTIALS */
#define RC_DENIED             203  /* DENIED */
#define RC_NOT_LOGGED_IN      204  /* NOT LOGGED IN */
#define RC_NOT_FOUND          205  /* NOT FOUND */
#define RC_NOT_ENOUGH_COIN    206  /* NOT ENOUGH COIN */
#define RC_INVALID_STATE      207  /* INVALID STATE */
#define RC_UNKNOWN_ERROR      999  /* UNKNOWN ERROR */

/* Header 8 byte cố định */
typedef struct {
    int32_t payload_len;
    int32_t command_type;
} PacketHeader;

/* Cấu trúc gói tin (dư 1 byte để luôn chèn được '\0' cuối chuỗi) */
typedef struct {
    PacketHeader header;
    char payload[MAX_LEN + 1];
} Packet;

/* Khởi tạo dữ liệu cho packet */
void packet_set(Packet *pkt, int command_type, const char *payload);

/* Gửi gói tin qua socket */
int send_packet(int fd, const Packet *pkt);

/* Nhận gói tin từ socket */
int recv_packet(int fd, Packet *pkt);

#endif /* PROTOCOL_H */
