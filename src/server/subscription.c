#include "isabela/server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int open_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 1) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static void format_metric(char *buffer, size_t len, const IsabelaAverages *avg, int metric) {
    switch (metric) {
        case 1:
            snprintf(buffer, len, "Calls duration: %.2f\n", avg->calls_duration);
            break;
        case 2:
            snprintf(buffer, len, "Calls made: %.2f\n", avg->calls_made);
            break;
        case 3:
            snprintf(buffer, len, "Calls missed: %.2f\n", avg->calls_missed);
            break;
        case 4:
            snprintf(buffer, len, "Calls received: %.2f\n", avg->calls_received);
            break;
        case 5:
            snprintf(buffer, len, "Sms received: %.2f\n", avg->sms_received);
            break;
        case 6:
            snprintf(buffer, len, "Sms sent: %.2f\n", avg->sms_sent);
            break;
        default:
            snprintf(buffer, len,
                     "AVERAGE DATA UPDATE:\nCalls duration: %.2f\nCalls made: %.2f\nCalls missed: %.2f\n"
                     "Calls received: %.2f\nSms received: %.2f\nSms sent: %.2f\n",
                     avg->calls_duration, avg->calls_made, avg->calls_missed, avg->calls_received,
                     avg->sms_received, avg->sms_sent);
            break;
    }
}

void isabela_subscription_loop(int port, int metric) {
    int listen_fd = open_listen_socket(port);
    if (listen_fd < 0) {
        _exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int conn = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
    close(listen_fd);
    if (conn < 0) {
        perror("accept");
        _exit(EXIT_FAILURE);
    }

    IsabelaServerContext ctx;
    isabela_server_context_init(&ctx);

    char err[256];
    char message[ISABELA_BUFFER_SIZE];
    char ack[ISABELA_BUFFER_SIZE];

    while (1) {
        if (!isabela_refresh_registry(&ctx, err, sizeof(err))) {
            snprintf(message, sizeof(message), "Subscription update failed: %s\n", err);
            write(conn, message, strlen(message));
            sleep(3);
            continue;
        }
        isabela_recalculate_averages(&ctx);
        format_metric(message, sizeof(message), &ctx.averages, metric);
        write(conn, message, strlen(message));
        ssize_t nread = read(conn, ack, sizeof(ack) - 1);
        if (nread <= 0) {
            break;
        }
        ack[nread] = '\0';
        sleep(3);
    }

    close(conn);
    _exit(0);
}
