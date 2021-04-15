#include "includes.h"

#define DEFAULT_PORTNUM     49999
#define DEFAULT_PORTNUM_STR "49999"
#define BACKLOG             4
#define PATH_MAX            4096

int childprocess(int listenfd);
int exit_cmd(int listenfd);