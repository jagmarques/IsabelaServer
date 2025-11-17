#define _POSIX_C_SOURCE 200809L

#include "isabela/server.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void isabela_subscription_loop(int port, int metric);

void isabela_server_context_init(IsabelaServerContext *ctx) {
    if (!ctx) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->listen_port = ISABELA_DEFAULT_PORT;
}

static void kill_subscription(pid_t pid) {
    if (pid <= 0) {
        return;
    }
    if (kill(pid, SIGTERM) == -1) {
        perror("kill");
    }
}

void isabela_server_context_shutdown(IsabelaServerContext *ctx) {
    if (!ctx) {
        return;
    }
    for (size_t i = 0; i < ctx->subscription_count; ++i) {
        kill_subscription(ctx->subscription_pids[i]);
    }
    ctx->subscription_count = 0;
}

void isabela_recalculate_averages(IsabelaServerContext *ctx) {
    if (!ctx || ctx->client_count == 0) {
        memset(&ctx->averages, 0, sizeof(ctx->averages));
        return;
    }

    IsabelaAverages totals = {0};
    for (size_t i = 0; i < ctx->client_count; ++i) {
        const IsabelaClientProfile *c = &ctx->clients[i];
        totals.calls_duration += c->calls_duration;
        totals.calls_made += c->calls_made;
        totals.calls_missed += c->calls_missed;
        totals.calls_received += c->calls_received;
        totals.sms_received += c->sms_received;
        totals.sms_sent += c->sms_sent;
    }

    ctx->averages.calls_duration = totals.calls_duration / ctx->client_count;
    ctx->averages.calls_made = totals.calls_made / ctx->client_count;
    ctx->averages.calls_missed = totals.calls_missed / ctx->client_count;
    ctx->averages.calls_received = totals.calls_received / ctx->client_count;
    ctx->averages.sms_received = totals.sms_received / ctx->client_count;
    ctx->averages.sms_sent = totals.sms_sent / ctx->client_count;
}

IsabelaClientProfile *isabela_find_client(IsabelaServerContext *ctx, int client_id) {
    if (!ctx) {
        return NULL;
    }
    for (size_t i = 0; i < ctx->client_count; ++i) {
        if (ctx->clients[i].id == client_id) {
            return &ctx->clients[i];
        }
    }
    return NULL;
}

bool isabela_spawn_subscription(IsabelaServerContext *ctx, int port, int metric) {
    if (!ctx || ctx->subscription_count >= ISABELA_MAX_SUBSCRIPTIONS) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        isabela_subscription_loop(port, metric);
        _exit(0);
    }

    ctx->subscription_pids[ctx->subscription_count++] = pid;
    return true;
}
