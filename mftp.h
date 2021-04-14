#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <fcntl.h>
// #include <time.h>

#define PORT_NUMBER     49999
#define PORT_NUMBER_STR "49999"
#define BACKLOG         4 


int local_cmd(char *buf);
int server_cmd (char*buf);
int exit_cmd(int socketfd, char *buf, int D_FLAG); 
int get_datasocket(int socketfd, int D_FLAG, const char* ip); 
int rls_cmd(int socketfd, char *buf, int D_FLAG, const char* arg);
int ls_cmd(int D_FLAG);
int cd_cmd(char *buf, int D_FLAG); 
int rcd_cmd(int socketfd, char *buf, int D_FLAG, const char* arg);
int show_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);
int fork_to_more(int datasocket);
int get_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);
int checkfile(char *inputPath);
int put_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);