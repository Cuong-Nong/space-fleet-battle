#include "logger.h"

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* Mutex bảo vệ tránh tranh chấp ghi log giữa nhiều luồng */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Ghi log ra đồng thời màn hình stdout và file logs/server.log.
 * Định dạng dòng log: [YYYY-MM-DD HH:MM:SS] [LEVEL] message
 */
void log_event(const char *level, const char *message) {
    if (level == NULL) {
        level = "INFO";
    }
    if (message == NULL) {
        message = "";
    }

    /* 1. Lấy thời gian hiện tại chính xác và định dạng chuỗi */
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);

    /* 2. Khóa mutex để đảm bảo an toàn đa luồng */
    pthread_mutex_lock(&log_mutex);

    /* 3. In ra màn hình console (stdout) */
    printf("[%s] [%s] %s\n", time_str, level, message);
    fflush(stdout);

    /* 4. Tự động tạo thư mục logs/ nếu chưa có */
    if (mkdir("logs", 0755) == -1) {
        if (errno != EEXIST) {
            /* Bỏ qua nếu đã tồn tại, nếu lỗi khác thì vẫn tiếp tục ghi stdout */
        }
    }

    /* 5. Ghi vào file logs/server.log ở chế độ append ("a") */
    FILE *fp = fopen("logs/server.log", "a");
    if (fp != NULL) {
        fprintf(fp, "[%s] [%s] %s\n", time_str, level, message);
        fflush(fp);
        fclose(fp);
    }

    /* 6. Mở khóa mutex */
    pthread_mutex_unlock(&log_mutex);
}
