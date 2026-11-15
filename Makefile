CC ?= gcc
CFLAGS ?= -Wall -Wextra -g -pthread -Isrc
LDFLAGS ?= -pthread

# Danh sách source files
SRCS_COMMON ?= $(wildcard src/common/*.c) $(wildcard src/logger/*.c) $(wildcard src/network/*.c)
SRCS_SERVER ?= $(wildcard src/server.c) $(SRCS_COMMON)
SRCS_CLIENT ?= $(wildcard src/client.c) $(SRCS_COMMON)
SRCS_TEST   ?= $(wildcard src/test_protocol.c) $(SRCS_COMMON)

.PHONY: all clean test server client test_protocol

# Build mặc định cả server và client
all: server client

server:
ifeq ($(wildcard src/server.c),)
	@echo "[INFO] src/server.c chua ton tai, bo qua build server."
else
	$(CC) $(CFLAGS) -o server $(SRCS_SERVER) $(LDFLAGS)
endif

client:
ifeq ($(wildcard src/client.c),)
	@echo "[INFO] src/client.c chua ton tai, bo qua build client."
else
	$(CC) $(CFLAGS) -o client $(SRCS_CLIENT) $(LDFLAGS)
endif

# Target chay test
test: test_protocol

test_protocol:
ifeq ($(wildcard src/test_protocol.c),)
	@echo "[INFO] src/test_protocol.c chua ton tai, bo qua build test_protocol."
else
	$(CC) $(CFLAGS) -o test_protocol $(SRCS_TEST) $(LDFLAGS)
endif

# Don dep cac file binary va object
clean:
	rm -f server client test_protocol *.o src/*.o src/*/*.o logs/*.log
	@echo "[INFO] Da don dep sach se moi file build va log."
