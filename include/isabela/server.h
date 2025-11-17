#ifndef ISABELA_SERVER_H
#define ISABELA_SERVER_H

// Public API for the Isabela FIWARE server component.

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define ISABELA_DEFAULT_PORT 9000
#define ISABELA_BUFFER_SIZE 1024
#define ISABELA_MAX_CLIENTS 64
#define ISABELA_MAX_ACTIVITY 64
#define ISABELA_MAX_LOCATION 64
#define ISABELA_MAX_DEPARTMENT 64
#define ISABELA_MAX_PENDING_CONNECTIONS 34
#define ISABELA_MAX_SUBSCRIPTIONS 64

// Represents a single FIWARE student/client entity.
typedef struct {
    int id;
    char activity[ISABELA_MAX_ACTIVITY];
    char location[ISABELA_MAX_LOCATION];
    int calls_duration;
    int calls_made;
    int calls_missed;
    int calls_received;
    char department[ISABELA_MAX_DEPARTMENT];
    int sms_received;
    int sms_sent;
    bool subscribed_to_average;
    bool subscribed_single_metric;
    int subscribed_metric; // 0 means aggregate, 1..6 represent single metrics
} IsabelaClientProfile;

// Aggregated statistics across all tracked clients.
typedef struct {
    double calls_duration;
    double calls_made;
    double calls_missed;
    double calls_received;
    double sms_received;
    double sms_sent;
} IsabelaAverages;

// Mutable state shared across the server runtime.
typedef struct {
    IsabelaClientProfile clients[ISABELA_MAX_CLIENTS];
    size_t client_count;
    IsabelaAverages averages;
    pid_t subscription_pids[ISABELA_MAX_SUBSCRIPTIONS];
    size_t subscription_count;
    int listen_port;
} IsabelaServerContext;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the context before starting the server.
void isabela_server_context_init(IsabelaServerContext *ctx);

// Stop any outstanding subscriptions and release resources.
void isabela_server_context_shutdown(IsabelaServerContext *ctx);

// Refresh the client registry from FIWARE or a fixture file.
bool isabela_refresh_registry(IsabelaServerContext *ctx, char *err_buf, size_t err_buf_len);

// Recalculate aggregate statistics from the registry.
void isabela_recalculate_averages(IsabelaServerContext *ctx);

// Find a client profile by numeric identifier.
IsabelaClientProfile *isabela_find_client(IsabelaServerContext *ctx, int client_id);

// Main event loop that accepts and processes clients.
int isabela_run_server(IsabelaServerContext *ctx);

// Spawn a background process that streams subscription updates.
bool isabela_spawn_subscription(IsabelaServerContext *ctx, int port, int metric);

#ifdef __cplusplus
}
#endif

#endif
