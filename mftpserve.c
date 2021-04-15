#include "mftp.h" 

#define DEFAULT_PORTNUM 49999
#define DEFAULT_PORTNUM_STR "49999"
#define BACKLOG 4

int D_FLAG; 
const char *USER_PORT = NULL; 
int main(int argc, char const *argv[])
{
    char *debug_flag = "-d", *port_flag = "-p"; 
    USER_PORT = DEFAULT_PORTNUM_STR;
    if(argc > 1) { 
        for(int i = 1; i <argc; i++)  {
            printf("arg %s\n", argv[i]);
            if(strcmp(argv[i], debug_flag) ==0) D_FLAG = 1; 
            if(strcmp(argv[i], port_flag) ==0)  USER_PORT = argv[i+1]; 
        } 
    } 
    if(D_FLAG) printf("Debug flag detected. \n");

    struct sockaddr_in server_address, client_address; 
    int socketfd, listenfd, length = sizeof(struct sockaddr_in); 

    if( (socketfd = socket(AF_INET, SOCK_STREAM, 0)) ==0) { 
        perror("Error"); 
        exit(-1); 
    } 
    if(D_FLAG) printf("sockerfd was created with a descriptor %d\n", socketfd); 
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) { 
        perror("Error");
        exit(-1); 
    }

    memset(&server_address, 0, sizeof(struct sockaddr_in)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(USER_PORT);

    if( bind(socketfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in) ) ) { 
        perror("Error");
        exit(-1); 
    }
    if(D_FLAG) printf("socket bounded to port %s\n", USER_PORT); 

    if( listen(socketfd, BACKLOG) < 0) { 
        perror("Error"); 
        exit(-1);
    } 
    if(D_FLAG) printf("listen with connection queue of %d\n", BACKLOG); 

    return 0;

}
