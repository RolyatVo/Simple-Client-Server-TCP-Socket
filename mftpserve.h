#include "includes.h"

#define DEFAULT_PORTNUM     49999
#define DEFAULT_PORTNUM_STR "49999"
#define BACKLOG             4
#define PATH_MAX            4096

int childprocess(int listenfd);
int exit_cmd(int listenfd);
int cd_cmd(int listenfd, char* buf);
int sendE(const char* message, int listenfd);
int sendA (int listenfd, int pid);
int getdatasocket(int listenfd);
int ls_cmd(int datalistenfd);