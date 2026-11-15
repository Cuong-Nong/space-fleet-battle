CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# Danh sach source files cho server: moi file src/*.c tru client.c va test_*.c
SERVER_SRCS = $(filter-out src/client.c $(wildcard src/test_*.c), $(wildcard src/*.c))

# Cac muc tieu can build trong all (chi them khi co file nguon)
TARGETS =
ifneq ($(wildcard src/server.c),)
TARGETS += server
endif
ifneq ($(wildcard src/client.c),)
TARGETS += client
endif

.PHONY: all clean test

all: $(TARGETS)
ifeq ($(TARGETS),)
	@echo "Chua co src/server.c hoac src/client.c de bien dich."
endif

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

client: src/client.c src/protocol.c
	$(CC) $(CFLAGS) -o $@ $^

test:
ifneq ($(wildcard src/test_protocol.c),)
	$(CC) $(CFLAGS) -o test_protocol src/test_protocol.c src/protocol.c
	./test_protocol
else
	@echo "Bo qua test_protocol (chua co src/test_protocol.c)"
endif
ifneq ($(wildcard src/test_logger.c),)
	$(CC) $(CFLAGS) -o test_logger src/test_logger.c src/logger.c
	./test_logger
else
	@echo "Bo qua test_logger (chua co src/test_logger.c)"
endif

clean:
	rm -f server client test_protocol test_logger *.o src/*.o
	@echo "Da xoa tat ca cac file .o va file chay."
