#include "protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define WIRE_HEADER_SIZE (sizeof(uint32_t) * 2)
#define MAX_PACKET_SIZE (WIRE_HEADER_SIZE + MAX_LEN)

typedef struct {
    int active;
    unsigned char input[MAX_PACKET_SIZE];
    size_t input_len;
    size_t expected_len;
    unsigned char output[MAX_PACKET_SIZE];
    size_t output_len;
    size_t output_sent;
} Client;

static Client clients[FD_SETSIZE];
static volatile sig_atomic_t stop_server;

static void handle_stop_signal(int signal_number) {
    (void)signal_number;
    stop_server = 1;
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }

    return 0;
}

static void close_client(int fd) {
    if (fd < 0 || fd >= FD_SETSIZE || !clients[fd].active) {
        return;
    }

    close(fd);
    memset(&clients[fd], 0, sizeof(clients[fd]));
    printf("Client disconnected: fd=%d\n", fd);
    fflush(stdout);
}

static int queue_response(Client *client, int command_type, const char *payload) {
    uint32_t header[2];
    size_t payload_len = strlen(payload);

    if (payload_len > MAX_LEN) {
        return -1;
    }

    header[0] = htonl((uint32_t)payload_len);
    header[1] = htonl((uint32_t)command_type);
    memcpy(client->output, header, WIRE_HEADER_SIZE);
    memcpy(client->output + WIRE_HEADER_SIZE, payload, payload_len);
    client->output_len = WIRE_HEADER_SIZE + payload_len;
    client->output_sent = 0;
    return 0;
}

static int process_packet(Client *client, int fd) {
    uint32_t wire_header[2];
    uint32_t command_type;

    memcpy(wire_header, client->input, WIRE_HEADER_SIZE);
    command_type = ntohl(wire_header[1]);

    if (command_type < CMD_REGISTER || command_type > CMD_ERROR) {
        fprintf(stderr, "Invalid command from fd=%d: %u\n", fd, command_type);
        return queue_response(client, CMD_ERROR, "200 INVALID FORMAT");
    }

    printf("Received command %u from fd=%d\n", command_type, fd);
    fflush(stdout);
    return queue_response(client, CMD_OK, "100 OK");
}

static int read_client(int fd) {
    Client *client = &clients[fd];
    ssize_t bytes_read;

    bytes_read = recv(fd, client->input + client->input_len,
                      client->expected_len - client->input_len, 0);
    if (bytes_read == 0) {
        return -1;
    }
    if (bytes_read < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        perror("recv");
        return -1;
    }

    client->input_len += (size_t)bytes_read;
    if (client->input_len == WIRE_HEADER_SIZE &&
        client->expected_len == WIRE_HEADER_SIZE) {
        uint32_t wire_header[2];
        uint32_t payload_len;

        memcpy(wire_header, client->input, WIRE_HEADER_SIZE);
        payload_len = ntohl(wire_header[0]);
        if (payload_len > MAX_LEN) {
            fprintf(stderr, "Invalid payload length from fd=%d: %u\n",
                    fd, payload_len);
            return -1;
        }
        client->expected_len = WIRE_HEADER_SIZE + payload_len;
    }

    if (client->input_len == client->expected_len) {
        if (process_packet(client, fd) < 0) {
            fprintf(stderr, "Could not queue response for fd=%d\n", fd);
            return -1;
        }
        client->input_len = 0;
        client->expected_len = WIRE_HEADER_SIZE;
    }

    return 0;
}

static int write_client(int fd) {
    Client *client = &clients[fd];
    ssize_t bytes_sent = send(fd, client->output + client->output_sent,
                              client->output_len - client->output_sent, 0);

    if (bytes_sent < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        perror("send");
        return -1;
    }
    if (bytes_sent == 0) {
        return -1;
    }

    client->output_sent += (size_t)bytes_sent;
    if (client->output_sent == client->output_len) {
        client->output_len = 0;
        client->output_sent = 0;
    }

    return 0;
}

static int create_listener(const char *port) {
    struct addrinfo hints;
    struct addrinfo *addresses;
    struct addrinfo *address;
    int listener = -1;
    int result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port, &hints, &addresses);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        int reuse_address = 1;

        listener = socket(address->ai_family, address->ai_socktype,
                          address->ai_protocol);
        if (listener < 0) {
            continue;
        }
        if (listener >= FD_SETSIZE) {
            fprintf(stderr, "Listening socket exceeds FD_SETSIZE\n");
            close(listener);
            listener = -1;
            continue;
        }
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                       sizeof(reuse_address)) < 0 ||
            set_nonblocking(listener) < 0 ||
            bind(listener, address->ai_addr, address->ai_addrlen) < 0 ||
            listen(listener, SOMAXCONN) < 0) {
            close(listener);
            listener = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(addresses);
    if (listener < 0) {
        perror("Could not create listening socket");
    }
    return listener;
}

static void accept_clients(int listener) {
    for (;;) {
        int fd = accept(listener, NULL, NULL);

        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
            }
            return;
        }

        if (fd >= FD_SETSIZE || set_nonblocking(fd) < 0) {
            fprintf(stderr, "Rejecting client: descriptor unavailable\n");
            close(fd);
            continue;
        }

        memset(&clients[fd], 0, sizeof(clients[fd]));
        clients[fd].active = 1;
        clients[fd].expected_len = WIRE_HEADER_SIZE;
        printf("Client connected: fd=%d\n", fd);
        fflush(stdout);
    }
}

static void run_server(int listener) {
    while (!stop_server) {
        fd_set read_fds;
        fd_set write_fds;
        int max_fd = listener;
        int fd;
        int ready;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(listener, &read_fds);

        for (fd = 0; fd < FD_SETSIZE; ++fd) {
            if (!clients[fd].active) {
                continue;
            }
            if (clients[fd].output_len == 0) {
                FD_SET(fd, &read_fds);
            } else {
                FD_SET(fd, &write_fds);
            }
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        ready = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(listener, &read_fds)) {
            accept_clients(listener);
        }

        for (fd = 0; fd <= max_fd; ++fd) {
            if (!clients[fd].active) {
                continue;
            }
            if (FD_ISSET(fd, &read_fds) && read_client(fd) < 0) {
                close_client(fd);
                continue;
            }
            if (clients[fd].active && FD_ISSET(fd, &write_fds) &&
                write_client(fd) < 0) {
                close_client(fd);
            }
        }
    }
}

static int valid_port(const char *value) {
    char *end;
    long port;

    errno = 0;
    port = strtol(value, &end, 10);
    return errno == 0 && *value != '\0' && *end == '\0' &&
           port >= 1 && port <= 65535;
}

int main(int argc, char **argv) {
    struct sigaction action;
    int listener;

    if (argc != 2 || !valid_port(argv[1])) {
        fprintf(stderr, "Usage: %s <port 1-65535>\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_stop_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    listener = create_listener(argv[1]);
    if (listener < 0) {
        return EXIT_FAILURE;
    }

    printf("Server listening on port %s\n", argv[1]);
    fflush(stdout);
    run_server(listener);

    close(listener);
    for (listener = 0; listener < FD_SETSIZE; ++listener) {
        if (clients[listener].active) {
            close_client(listener);
        }
    }

    return EXIT_SUCCESS;
}
