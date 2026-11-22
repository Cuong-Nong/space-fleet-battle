#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/*
 * Ghi log với mức độ (level) và nội dung (message).
 * An toàn đa luồng, tự động gắn nhãn thời gian [YYYY-MM-DD HH:MM:SS].
 */
void log_event(const char *level, const char *message);

/*
 * Các macro tiện ích định dạng chuỗi trước khi ghi log.
 * Sử dụng buffer tạm 512 byte và ##__VA_ARGS__ an toàn.
 */
#define LOG_INFO(fmt, ...) do { \
    char _log_buf[512]; \
    snprintf(_log_buf, sizeof(_log_buf), fmt, ##__VA_ARGS__); \
    log_event("INFO", _log_buf); \
} while (0)

#define LOG_WARN(fmt, ...) do { \
    char _log_buf[512]; \
    snprintf(_log_buf, sizeof(_log_buf), fmt, ##__VA_ARGS__); \
    log_event("WARN", _log_buf); \
} while (0)

#define LOG_ERROR(fmt, ...) do { \
    char _log_buf[512]; \
    snprintf(_log_buf, sizeof(_log_buf), fmt, ##__VA_ARGS__); \
    log_event("ERROR", _log_buf); \
} while (0)

#endif /* LOGGER_H */
