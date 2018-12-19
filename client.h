#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <signal.h> 

#define BUF_SIZE 1024

#define MAX_STRING 1000

int subscribed;
pid_t child;
char host[1024], port[1024];
int pids[1000];
int n=0;


