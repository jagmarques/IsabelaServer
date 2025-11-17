#define _POSIX_C_SOURCE 200809L

#include "isabela/server.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void assert_close(double lhs, double rhs, const char *label) {
    if (fabs(lhs - rhs) > 0.001) {
        fprintf(stderr, "%s mismatch: %.3f != %.3f\n", label, lhs, rhs);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    IsabelaServerContext ctx;
    isabela_server_context_init(&ctx);
    setenv("ISABELA_DATA_FILE", "fixtures/sample_students.json", 1);

    char err[256];
    if (!isabela_refresh_registry(&ctx, err, sizeof(err))) {
        fprintf(stderr, "Failed to load registry: %s\n", err);
        return EXIT_FAILURE;
    }

    if (ctx.client_count != 2) {
        fprintf(stderr, "Unexpected client count: %zu\n", ctx.client_count);
        return EXIT_FAILURE;
    }

    isabela_recalculate_averages(&ctx);
    assert_close(ctx.averages.calls_duration, 25.0, "calls_duration");
    assert_close(ctx.averages.calls_made, 8.5, "calls_made");
    assert_close(ctx.averages.calls_missed, 2.5, "calls_missed");
    assert_close(ctx.averages.calls_received, 7.0, "calls_received");
    assert_close(ctx.averages.sms_received, 4.0, "sms_received");
    assert_close(ctx.averages.sms_sent, 4.5, "sms_sent");

    printf("Registry tests passed.\n");
    return EXIT_SUCCESS;
}
