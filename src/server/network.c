#include "isabela/server.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static int create_server_socket(int port) {
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

    if (listen(fd, ISABELA_MAX_PENDING_CONNECTIONS) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static ssize_t read_line(int fd, char *buf, size_t len) {
    size_t pos = 0;
    while (pos + 1 < len) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) {
            return n;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            break;
        }
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (ssize_t)pos;
}

static void trim(char *s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static void send_menu(int fd) {
    const char *menu =
        "\nMENU:\n"
        "1) Client data\n"
        "2) Average data of all clients\n"
        "3) Subscribe to average updates\n"
        "4) Subscribe to single metric\n"
        "Type EXIT to disconnect\n> ";
    write(fd, menu, strlen(menu));
}

static void describe_client(int fd, const IsabelaClientProfile *client) {
    char buffer[ISABELA_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer),
             "\nClient %d\nActivity: %s\nLocation: %s\nDepartment: %s\nCalls duration: %d\n"
             "Calls made: %d\nCalls missed: %d\nCalls received: %d\nSMS received: %d\nSMS sent: %d\n",
             client->id, client->activity, client->location, client->department, client->calls_duration,
             client->calls_made, client->calls_missed, client->calls_received, client->sms_received,
             client->sms_sent);
    write(fd, buffer, strlen(buffer));
}

static void describe_average(int fd, const IsabelaAverages *avg) {
    char buffer[ISABELA_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer),
             "\nAverage metrics\nCalls duration: %.2f\nCalls made: %.2f\nCalls missed: %.2f\n"
             "Calls received: %.2f\nSms received: %.2f\nSms sent: %.2f\n",
             avg->calls_duration, avg->calls_made, avg->calls_missed, avg->calls_received,
             avg->sms_received, avg->sms_sent);
    write(fd, buffer, strlen(buffer));
}

static bool handle_subscription(IsabelaServerContext *ctx, int fd, int metric, bool *flag) {
    if (*flag) {
        const char *msg = "\nAlready subscribed.\n";
        write(fd, msg, strlen(msg));
        return true;
    }
    const char *prompt = "\nSubscription accepted. Provide local port:\n";
    write(fd, prompt, strlen(prompt));
    char line[64];
    if (read_line(fd, line, sizeof(line)) <= 0) {
        return false;
    }
    int port = atoi(line);
    if (port <= 0) {
        const char *msg = "\nInvalid port.\n";
        write(fd, msg, strlen(msg));
        return false;
    }
    if (!isabela_spawn_subscription(ctx, port, metric)) {
        const char *msg = "\nFailed to create subscription.\n";
        write(fd, msg, strlen(msg));
        return false;
    }
    *flag = true;
    const char *msg = "\nSubscription started.\n";
    write(fd, msg, strlen(msg));
    return true;
}

static bool process_command(IsabelaServerContext *ctx, int fd, IsabelaClientProfile *client,
                            const char *command, bool *avg_flag, bool metric_flags[7]) {
    if (strcmp(command, "1") == 0) {
        describe_client(fd, client);
        return true;
    }
    if (strcmp(command, "2") == 0) {
        isabela_recalculate_averages(ctx);
        describe_average(fd, &ctx->averages);
        return true;
    }
    if (strcmp(command, "3") == 0) {
        return handle_subscription(ctx, fd, 0, avg_flag);
    }
    if (strcmp(command, "4") == 0) {
        const char *single_menu =
            "\nSelect metric:\n"
            "1) Calls duration\n"
            "2) Calls made\n"
            "3) Calls missed\n"
            "4) Calls received\n"
            "5) Sms received\n"
            "6) Sms sent\n> ";
        write(fd, single_menu, strlen(single_menu));
        char line[16];
        if (read_line(fd, line, sizeof(line)) <= 0) {
            return false;
        }
        int metric = atoi(line);
        if (metric < 1 || metric > 6) {
            const char *msg = "\nInvalid metric.\n";
            write(fd, msg, strlen(msg));
            return true;
        }
        if (metric_flags[metric]) {
            const char *msg = "\nMetric already subscribed.\n";
            write(fd, msg, strlen(msg));
            return true;
        }
        bool ok = handle_subscription(ctx, fd, metric, &metric_flags[metric]);
        if (ok) {
            client->subscribed_single_metric = true;
            client->subscribed_metric = metric;
        }
        return ok;
    }
    if (strcasecmp(command, "EXIT") == 0) {
        const char *bye = "Closing connection.\n";
        write(fd, bye, strlen(bye));
        return false;
    }
    const char *msg = "\nERROR: COMMAND NOT FOUND\n";
    write(fd, msg, strlen(msg));
    return true;
}

static bool authenticate_client(IsabelaServerContext *ctx, int fd, IsabelaClientProfile **out_client) {
    char line[64];
    while (1) {
        const char *prompt = "ID: ";
        write(fd, prompt, strlen(prompt));
        ssize_t n = read_line(fd, line, sizeof(line));
        if (n <= 0) {
            return false;
        }
        trim(line);
        int id = atoi(line);
        IsabelaClientProfile *client = isabela_find_client(ctx, id);
        if (client) {
            const char *ok = "ID Accepted!\n";
            write(fd, ok, strlen(ok));
            *out_client = client;
            return true;
        }
        const char *fail = "ID Denied!\n";
        write(fd, fail, strlen(fail));
    }
}

static void process_client(IsabelaServerContext *ctx, int client_fd) {
    char err[256];
    if (!isabela_refresh_registry(ctx, err, sizeof(err))) {
        write(client_fd, err, strlen(err));
        return;
    }

    IsabelaClientProfile *client = NULL;
    if (!authenticate_client(ctx, client_fd, &client)) {
        return;
    }

    bool average_flag = false;
    bool metric_flags[7] = {false};

    while (1) {
        send_menu(client_fd);
        char line[ISABELA_BUFFER_SIZE];
        ssize_t n = read_line(client_fd, line, sizeof(line));
        if (n <= 0) {
            break;
        }
        trim(line);
        if (!process_command(ctx, client_fd, client, line, &average_flag, metric_flags)) {
            break;
        }
    }
}

int isabela_run_server(IsabelaServerContext *ctx) {
    int listen_fd = create_server_socket(ctx->listen_port);
    if (listen_fd < 0) {
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d\n", ctx->listen_port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        process_client(ctx, client_fd);
        close(client_fd);
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
