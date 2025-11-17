#include "isabela/server.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static IsabelaServerContext g_ctx;

static void handle_signal(int sig) {
    (void)sig;
    isabela_server_context_shutdown(&g_ctx);
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    isabela_server_context_init(&g_ctx);

    char err[256];
    if (!isabela_refresh_registry(&g_ctx, err, sizeof(err))) {
        fprintf(stderr, "Initial registry load failed: %s\n", err);
    }

    int rc = isabela_run_server(&g_ctx);
    isabela_server_context_shutdown(&g_ctx);
    return rc;
}
