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
    if(D_FLAG) printf("socketfd was created with a descriptor %d\n", socketfd); 
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
            while((pid = waitpid(0, &status, WNOHANG)) > 0) { 
                if(D_FLAG) printf("P: Detected termination of child %d, ecode: %d\n", pid, status);
            }  
        } 
        
    } 

}

int childprocess(int listenfd) {
    char *newn = "\n";
    int readbytes, cmd, datalistenfd = -20; 
    char *string = NULL;
    //Space for longest command, space, newline, and longest possible file path(4096).
    char buf[6 + PATH_MAX] = {0}; 
    char onec[1];


    //Check for listen fd, 
    while (listenfd, F_GETFD) { 
        //Get entire command until newline
        while( (readbytes = read(listenfd, onec, 1) > 0)) { 
            strncat(buf, onec, 1);
            if(onec[0] == '\n') break;
        } 


        if (readbytes >0 ) {
        // while ( (readbytes = read(listenfd, buf, sizeof(buf))) > 0 ) { 
            if(D_FLAG) printf("C %d: Recieved %s", getpid(), buf);
            printf("GOT: %s", buf);
            switch (buf[0]){
            case 81:
            //set exit command so that we exited normally
                exit_cmd(listenfd); 
                break;
            case 67: 
                cd_cmd(listenfd, buf); 
                break;
            case 68:
                if(datalistenfd > 0) sendE("Data socket already exists", listenfd); 
                else datalistenfd = getdatasocket(listenfd); 
                break;
            case 76: 
                if(datalistenfd < 0) sendE("Data connection was never established", listenfd);
                else {
                    sendA(listenfd, getpid());   
                    ls_cmd(datalistenfd);
                }
                break;
            default:
                break;
            }
            memset(buf, 0, sizeof(buf));
        }
        //check to see if we exited normally, if not close fd and return error to server stdout
    }
    
    //Add check here if connection ended sooner than thought to be.
}

int sendA (int listenfd, int pid) { 
    char *posA = "A\n";
    if (D_FLAG) printf("C %d: Sending positive acknowledgement\n", pid); 
    if(write(listenfd, posA, strlen(posA)) < 0) {
        fprintf(stderr, "error writing A to socket\n");
        return -1;
    }
    return 0;
} 
int sendE(const char* message, int listenfd) { 

} 
int exit_cmd(int listenfd) { 
    int pid = getpid();
    if (D_FLAG) printf("C %d: Quitting\n", pid);
    sendA(listenfd, pid);  
    if (D_FLAG) printf("C %d: exiting normally.\n", pid);
    close(listenfd); 
    exit(0);

} 
int cd_cmd(int listenfd, char* buf) { 
    int pid = getpid(), err;
    buf++;
    printf("%s",buf);
    buf[strlen(buf)-1] = '\0';
    err = chdir(buf);
    if (err < 0) fprintf(stderr,"%s\n", strerror(errno)); 
    else printf("changed cwd to %s\n", getcwd(buf, PATH_MAX));
    buf--;
    sendA(listenfd, pid); 

}   

int getdatasocket(int listenfd) {
    char*a = "A"; 
    char *newn = "\n";
    char port[20] = {0}; 
    if(D_FLAG) printf("C %d: Establishing data connection\n", getpid());
    struct sockaddr_in server_address, client_address; 
    int datasocketfd, datalistenfd, hostEntry, length = sizeof(struct sockaddr_in); 
    char hostName[NI_MAXHOST] = {0};


    if( (datasocketfd = socket(AF_INET, SOCK_STREAM, 0)) ==0) { 
        perror("Error"); 
        exit(-1); 
    } 
    if(D_FLAG) printf("socketfd was created with a descriptor %d\n", datasocketfd); 
    if(setsockopt(datasocketfd, SOL_SOCKET, SO_REUSEADDR,  &(int){1}, sizeof(int))) { 
        perror("Error");
        exit(-1); 
    }

    memset(&server_address, 0, sizeof(struct sockaddr_in)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = 0;

    if( bind(datasocketfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in) ) < 0) { 
        perror("Error");
        exit(-1); 
    }
    if (getsockname(datasocketfd, (struct sockaddr*) &server_address, &length) < 0) { 
        perror("Error");
        exit(-1);
    }
    if(D_FLAG) printf("C %d: socket bounded to port %d\n",getpid(),  ntohs(server_address.sin_port)); 

    if( listen(datasocketfd, 1) < 0) { 
        perror("Error"); 
        exit(-1);
    }
    if(D_FLAG) printf("C %d; listening to data socket\n", getpid());
    sprintf(port, "A%d\n", ntohs(server_address.sin_port)); 
    printf("POOORT: %s", port);
    write(listenfd, port, strlen(port));

    if(D_FLAG) printf("C %d: Sent acknowledgement -> %s%s",getpid(), a, port);

    if( (datalistenfd = accept(datasocketfd, (struct sockaddr*) &client_address, &length)) ) { 
        if((hostEntry = getnameinfo((struct sockaddr*)&client_address, sizeof(client_address), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV)) != 0) { 
                fprintf(stderr, "Error: %s", gai_strerror(hostEntry));
                fflush(stderr);
            }
        if(D_FLAG) { 
            printf("C %d: Accepted connection from %s on data socket with fd %d\n", getpid(), hostName, datalistenfd);
            printf("C: %d Data socket port on client is %d\n", getpid(), ntohs(client_address.sin_port));
        }
    

        return datalistenfd;
    }
}
int ls_cmd(int datalistenfd) { 
    int fd[2], reader, writer; 
    if(pipe (fd) < 0) { 
        printf("%s\n", strerror(errno)); 
        exit(-1); 
    }  
    pipe(fd); 
    writer = fd[1];
    reader = fd[0];
    int pid = fork(); 
    if(pid < 0) { 
        printf("ERROR: Problem with forking.\n");
        exit(-1); 
    } 
    else if (pid ==0) { 
        dup2(datalistenfd, STDOUT_FILENO); 
        close(writer); 
        close(reader);
        execlp("ls", "ls", "-l", (char*) NULL); 
        printf("%s\n", strerror(errno));
        exit(-1);
    } 
    else { 
        close(reader);
        close(writer);
        waitpid(pid, NULL, 0); 
        if(D_FLAG) printf("ls command completed\n");
    }
    close(datalistenfd);
} 