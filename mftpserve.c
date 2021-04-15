#include "mftpserve.h" 


int D_FLAG; 
const char *USER_PORT = NULL; 
int main(int argc, char const *argv[])
{
    char *debug_flag = "-d", *port_flag = "-p"; 
    USER_PORT = DEFAULT_PORTNUM_STR;
    if(argc > 1) { 
        for(int i = 1; i <argc; i++)  {
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
    server_address.sin_port = htons(atoi(USER_PORT));

    if( bind(socketfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in) ) ) { 
        perror("Error");
        exit(-1); 
    }
    if(D_FLAG) printf("P: socket bounded to port %s\n", USER_PORT); 

    if( listen(socketfd, BACKLOG) < 0) { 
        perror("Error"); 
        exit(-1);
    } 
    if(D_FLAG) printf("P: listen with connection queue of %d\n", BACKLOG); 

    char hostName[NI_MAXHOST];
    int hostEntry, pid, status;



    while(1) { 
        if( (listenfd = accept(socketfd, (struct sockaddr*) &client_address, &length)) ) { 
            if(D_FLAG) printf("P: accepted client connection with descriptor %d\n", listenfd); 
            pid = fork(); 
            if(pid < 0) printf("Was not able to fork off child.."); 
            else if(pid ==0) { 
                if((hostEntry = getnameinfo((struct sockaddr*)&client_address, sizeof(client_address), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV)) != 0) { 
                    fprintf(stderr, "Error: %s", gai_strerror(hostEntry));
                    fflush(stderr);
                }
                printf("P: Connection accepted from %s\n", hostName); 
                childprocess(listenfd); 

            }
            else { 
                if(D_FLAG) printf("P: spawned child %d, waiting for new connection.\n", pid);


            }
            //fix problem of only get 1 for pid when cleaning zombie process. 
            while(pid = waitpid(0, &status, WNOHANG) > 0) { 
                if(D_FLAG) printf("P: Detected termination of child %d, ecode: %d\n", pid, status);
            }  
        } 
        
    } 

}

int childprocess(int listenfd) {
    int readbytes, cmd; 

    //Space for longest command, space, newline, and longest possible file path(4096).
    char buf[6 + PATH_MAX]; 
    while ( (readbytes = read(listenfd, buf, sizeof(buf))) > 0 ) { 
        if(D_FLAG) printf("C %d: Recieved %s", getpid(), buf);
        switch (buf[0]){
        case 81:
            exit_cmd(listenfd); 
            break;
        default:
            break;
        }


    } 
    //Add check here if connection ended sooner than thought to be.
}

int exit_cmd(int listenfd) { 
    char *posA = "A\n";
    int pid = getpid();
    if (D_FLAG) printf("C %d: Quitting\n", pid);
    if (D_FLAG) printf("C %d: Sending positive acknowledgement\n", pid); 
    if(write(listenfd, posA, strlen(posA)) < 0) fprintf(stderr, "error writing A to socket\n");

    if (D_FLAG) printf("C %d: exiting normally.\n", pid); 
    exit(0);

} 
