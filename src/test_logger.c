#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Kiểm tra cấu trúc thời gian và nhãn log cố định:
 * "[YYYY-MM-DD HH:MM:SS] [INFO] "
 *  01234567890123456789012345678
 */
static int verify_log_format(const char *line, const char *expected_level, const char *expected_text) {
    if (line == NULL) return 0;
    size_t len = strlen(line);

    /* Độ dài tối thiểu cho "[YYYY-MM-DD HH:MM:SS] [LEVEL] " là 29 ký tự */
    if (len < 29) return 0;

    /* Kiểm tra các ký tự cố định của timestamp: [YYYY-MM-DD HH:MM:SS] */
    if (line[0] != '[') return 0;
    for (int i = 1; i <= 4; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[5] != '-') return 0;
    for (int i = 6; i <= 7; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[8] != '-') return 0;
    for (int i = 9; i <= 10; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[11] != ' ') return 0;
    for (int i = 12; i <= 13; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[14] != ':') return 0;
    for (int i = 15; i <= 16; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[17] != ':') return 0;
    for (int i = 18; i <= 19; i++) if (!isdigit((unsigned char)line[i])) return 0;
    if (line[20] != ']' || line[21] != ' ') return 0;

    /* Kiểm tra phần level: ví dụ [INFO] */
    char level_tag[32];
    snprintf(level_tag, sizeof(level_tag), "[%s] ", expected_level);
    if (strncmp(line + 22, level_tag, strlen(level_tag)) != 0) {
        return 0;
    }

    /* Kiểm tra chuỗi thông điệp */
    if (expected_text != NULL && strstr(line, expected_text) == NULL) {
        return 0;
    }

    return 1;
}

int main(void) {
    printf("============================================================\n");
    printf("   KIỂM THỬ MODULE LOGGER (TEST LOGGER)\n");
    printf("============================================================\n");

    /* 1. Ghi vài dòng log mẫu với các mức độ khác nhau */
    LOG_INFO("Khoi dong module test logger: ID=%d", 1001);
    LOG_WARN("Canh bao thu nghiem: Tai nguyen sap day (%d%%)", 85);
    LOG_ERROR("Loi he thong mo phong: Mat ket noi toi cong %d", 8080);
    LOG_INFO("Kiem tra dong log cuoi cung: Hoan tat ghi log test");

    /* 2. Đọc lại file logs/server.log để kiểm tra dòng cuối cùng */
    FILE *fp = fopen("logs/server.log", "r");
    if (fp == NULL) {
        printf("[FAIL] Khong the mo file logs/server.log de kiem tra.\n");
        return 1;
    }

    char line_buf[1024];
    char last_line[1024] = {0};

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        /* Bỏ ký tự xuống dòng ở cuối nếu có */
        size_t l = strlen(line_buf);
        if (l > 0 && (line_buf[l - 1] == '\n' || line_buf[l - 1] == '\r')) {
            line_buf[l - 1] = '\0';
        }
        strncpy(last_line, line_buf, sizeof(last_line) - 1);
    }
    fclose(fp);

    /* 3. Kiểm tra định dạng của dòng log cuối cùng */
    int is_valid = verify_log_format(last_line, "INFO", "Kiem tra dong log cuoi cung: Hoan tat ghi log test");

    if (is_valid) {
        printf("[PASS] Ca kiem thu Logger: Dinh dang thoi gian va level hop le.\n");
        printf("       Dong cuoi doc duoc: \"%s\"\n", last_line);
        printf("============================================================\n");
        printf("Ket qua: 1/1 ca kiem thu Logger THANH CONG (PASS).\n");
        printf("============================================================\n");
        return 0;
    } else {
        printf("[FAIL] Ca kiem thu Logger: Dinh dang dong log khong dung yeu cau.\n");
        printf("       Dong cuoi nhan duoc: \"%s\"\n", last_line);
        return 1;
    }
}
