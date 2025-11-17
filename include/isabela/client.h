#ifndef ISABELA_CLIENT_H
#define ISABELA_CLIENT_H

// Public helpers for the Isabela interactive client.

#include <stddef.h>

#define ISABELA_CLIENT_BUFFER 1024
#define ISABELA_MAX_SUBSCRIPTIONS 64

#ifdef __cplusplus
extern "C" {
#endif

// Parse the host argument and connect to the server.
int isabela_connect(const char *host, const char *port);

// Run the interactive client loop against an established socket.
int isabela_client_loop(int fd);

#ifdef __cplusplus
}
#endif

#endif
