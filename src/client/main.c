#define _POSIX_C_SOURCE 200809L

#include "isabela/client.h"

#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static char g_server_host[256];
static pid_t g_subscription_pids[ISABELA_MAX_SUBSCRIPTIONS];
static size_t g_subscription_count = 0;

static void send_line(int fd, const char *line) {
    size_t len = strlen(line);
    if (len > 0) {
        write(fd, line, len);
    }
    write(fd, "\n", 1);
}

static void cleanup_children(void) {
    for (size_t i = 0; i < g_subscription_count; ++i) {
        if (g_subscription_pids[i] > 0) {
            kill(g_subscription_pids[i], SIGTERM);
        }
    }
    g_subscription_count = 0;
}

static int connect_raw(const char *host, const char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result = NULL;
    int err = getaddrinfo(host, port, &hints, &result);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    int fd = -1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);
    return fd;
}

int isabela_connect(const char *host, const char *port) {
    strncpy(g_server_host, host, sizeof(g_server_host) - 1);
    g_server_host[sizeof(g_server_host) - 1] = '\0';

    int fd = connect_raw(host, port);
    if (fd < 0) {
        perror("connect");
        return -1;
    }
    return fd;
}

static void subscription_worker(const char *port) {
    int fd = connect_raw(g_server_host, port);
    if (fd < 0) {
        perror("subscription connect");
        _exit(EXIT_FAILURE);
    }

    char buffer[ISABELA_CLIENT_BUFFER];
    while (1) {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            break;
        }
        buffer[n] = '\0';
        printf("[SUBSCRIPTION] %s", buffer);
        const char *ack = "Subscription update received\n";
        write(fd, ack, strlen(ack));
    }
    close(fd);
    _exit(0);
}

static void spawn_subscription_reader(int port) {
    if (g_subscription_count >= ISABELA_MAX_SUBSCRIPTIONS) {
        fprintf(stderr, "Maximum number of subscriptions reached.\n");
        return;
    }
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        subscription_worker(port_str);
    }
    g_subscription_pids[g_subscription_count++] = pid;
}

static ssize_t read_server(int fd, char *buffer, size_t len, int echo) {
    ssize_t n = read(fd, buffer, len - 1);
    if (n <= 0) {
        return n;
    }
    buffer[n] = '\0';
    if (echo) {
        fputs(buffer, stdout);
        fflush(stdout);
    }
    return n;
}

static int authenticate(int fd) {
    char buffer[ISABELA_CLIENT_BUFFER];
    while (1) {
        if (read_server(fd, buffer, sizeof(buffer), 1) <= 0) {
            return -1;
        }
        if (strstr(buffer, "ID:")) {
            printf("Enter numeric client ID: ");
            fflush(stdout);
            char input[64];
            if (!fgets(input, sizeof(input), stdin)) {
                return -1;
            }
            char *nl = strchr(input, '\n');
            if (nl) {
                *nl = '\0';
            }
            send_line(fd, input);
        }
        if (strstr(buffer, "ID Accepted!")) {
            return 0;
        }
    }
    return -1;
}

int isabela_client_loop(int fd) {
    if (authenticate(fd) != 0) {
        fprintf(stderr, "Authentication failed.\n");
        return 1;
    }

    struct pollfd pfds[2];
    char server_buf[ISABELA_CLIENT_BUFFER];
    char input[ISABELA_CLIENT_BUFFER];
    int pending_port = -1;
    int awaiting_port = 0;
    int awaiting_ack = 0;

    while (1) {
        pfds[0].fd = fd;
        pfds[0].events = POLLIN;
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events = POLLIN;

        if (poll(pfds, 2, -1) < 0) {
            perror("poll");
            break;
        }

        if (pfds[0].revents & POLLIN) {
            ssize_t n = read_server(fd, server_buf, sizeof(server_buf), 1);
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            if (strstr(server_buf, "Provide local port")) {
                awaiting_port = 1;
                printf("Local subscription port: ");
                fflush(stdout);
            }
            if (strstr(server_buf, "Subscription started") && awaiting_ack && pending_port > 0) {
                spawn_subscription_reader(pending_port);
                awaiting_ack = 0;
                pending_port = -1;
            }
            if (strstr(server_buf, "Closing connection")) {
                break;
            }
        }

        if (pfds[1].revents & POLLIN) {
            if (!fgets(input, sizeof(input), stdin)) {
                send_line(fd, "EXIT");
                break;
            }
            char *nl = strchr(input, '\n');
            if (nl) {
                *nl = '\0';
            }
            if (input[0] == '\0') {
                continue;
            }
            if (awaiting_port) {
                pending_port = atoi(input);
                awaiting_ack = 1;
                awaiting_port = 0;
            }
            send_line(fd, input);
            if (strcasecmp(input, "EXIT") == 0) {
                break;
            }
        }
    }

    cleanup_children();
    return 0;
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <host> <port>\n", prog);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int fd = isabela_connect(argv[1], argv[2]);
    if (fd < 0) {
        return EXIT_FAILURE;
    }

    int rc = isabela_client_loop(fd);
    close(fd);
    return rc;
}
