#include "mftpserve.h" 

#define DATA_BUFFER 512
#define READ_BUFFER 1


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
    char onec[READ_BUFFER];


    //Check for listen fd, 
    while (listenfd, F_GETFD) { 
        //Get entire command until newline
        while( (readbytes = read(listenfd, onec, READ_BUFFER) > 0)) { 
            strncat(buf, onec, readbytes);
            if(onec[0] == '\n') break;
        } 


        if (readbytes >0 ) {
        // while ( (readbytes = read(listenfd, buf, sizeof(buf))) > 0 ) { 
            if(D_FLAG) printf("C %d: Recieved %s", getpid(), buf);
            switch (buf[0]){
                case 81:
                //set exit command so that we exited normally
                    exit_cmd(listenfd); 
                    break;
                case 67: // C<pathname> command 
                    cd_cmd(listenfd, buf); 
                    break;
                case 68: // <D> command
                    if(datalistenfd > 0) sendE("Data socket already exists", listenfd); 
                    else datalistenfd = getdatasocket(listenfd); 
                    break;
                case 76: // <L> command 
                // Check to see if they established a data connection.
                    if(datalistenfd < 0) sendE("Data connection was never established", listenfd);
                    else {
                        sendA(listenfd, getpid());   
                        ls_cmd(datalistenfd);
                        close(datalistenfd);
                        datalistenfd = -20;
                    }
                    break;
                case 71: // G command
                    if(datalistenfd < 0) sendE("Data connection was never established", listenfd);
                    else { 
                        get_cmd(listenfd, datalistenfd, buf); 
                        close(datalistenfd);
                        datalistenfd = -20;
                    }
                    break; 
                case 80: // P command
                    if(datalistenfd < 0) sendE("Data connection was never established", listenfd);
                    else { 
                        put_cmd(listenfd, datalistenfd, buf); 
                        close(datalistenfd);
                        datalistenfd = -20;
                    }
                    break;
                default:
                    break;
            }
            memset(buf, 0, sizeof(buf));
        }
        if(readbytes ==0) { 
            close(listenfd); 
            printf("C %d: Connection closed unexpectedly or EOF\n", getpid());
            exit(-1);
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
    char *E= "E"; 
    if(D_FLAG) printf("Sending E responsse %s", message);
    write(listenfd, E, strlen(E));
    write(listenfd, message, strlen(message));
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
    //Check permissions and based on that permissions error check
    int pid = getpid(), err;
    buf++;
    buf[strlen(buf)-1] = '\0';
    if(checkdir(buf, listenfd) < 0) { 
        if(D_FLAG) printf("Problem with given path\n");
        return -1;
    } 
    err = chdir(buf);
    if (err < 0){ 
        fprintf(stderr,"Error: %s\n", strerror(errno));
        sendE("Could not cd to given path.\n", listenfd); 
        return -1;
    } 
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
    write(listenfd, port, strlen(port));

    if(D_FLAG) printf("C %d: Sent acknowledgement -> %s",getpid(), port);

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
    int reader, writer; 

    int pid = fork(); 
    if(pid < 0) { 
        printf("ERROR: Problem with forking.\n");
        exit(-1); 
    } 
    else if (pid ==0) { 
        dup2(datalistenfd, STDOUT_FILENO); 

        execlp("ls", "ls", "-l", (char*) NULL); 
        printf("%s\n", strerror(errno));
        exit(-1);
    } 
    else { 
        waitpid(pid, NULL, 0); 
        if(D_FLAG) printf("ls command completed\n");
    }
} 

int get_cmd(int listenfd, int datalistenfd, char*buf) { 

    int filefd, readbytes;
    char r_buf[DATA_BUFFER]; 
    buf+=1; 
    if(strlen(buf) ==0) { 
        sendE("No path given.\n", listenfd);
        return -1;
    }
    buf[strlen(buf)-1] = '\0'; 
    if( access(buf, F_OK) != 0) { 
        sendE("File does not exist in path.\n", listenfd); 
        return -1;
    }
    else if(checkfile(buf) < 0 || access(buf, R_OK) != 0) {
        sendE("File is not readable/regular.\n", listenfd);
        printf("Error file not readable on server.\n");
        return -1;
    }
    else {
        sendA(listenfd, getpid()); 
        filefd = open(buf, O_RDONLY); 
    }
    if( lseek(filefd, 0, SEEK_SET) < 0) fprintf(stderr, strerror(errno), sizeof(strerror(errno)));

    while( (readbytes = read(filefd, r_buf, sizeof(r_buf))) > 0) { 
        if(D_FLAG) printf("Read %d bytes from local path, writing to server.\n", readbytes); 
        write(datalistenfd, r_buf, readbytes);
        //ERROR CHECK WRITE
    }
    close(filefd); 
} 
int checkfile(char *inputPath) { 
    struct stat area, *s = &area;

	// Check to see if inputpath is a regular file, if it is and it's readable return 1, else just return -1
	if(stat(inputPath, s) == 0 ) { 
		//Regular file and readable
		if(S_ISREG(s->st_mode) && (s->st_mode & S_IRUSR)) { 
			return 1; 
		}
	}
    return -1;
} 
int checkdir( char* inputPath, int listenfd) {
    struct stat area, *s = &area; 
    DIR *dir;
    char cwd[PATH_MAX + 1]; 
    if(inputPath == ".") inputPath = getcwd(cwd, sizeof(cwd)); 
    else if(stat(inputPath, s) ==0 && S_ISDIR(s->st_mode) ==0) { 
        if(S_ISREG(s->st_mode) && (s->st_mode & S_IRUSR)) { 
            return 1;
        }
        else { 
            sendE("No permissions for this directory\n", listenfd);
            return -1;
        } 
    }
    else { 
        sendE("Directory does not exist!\n", listenfd);
        return -2;
    }  
} 
int put_cmd(int listenfd, int datalistenfd, char *buf) { 
    int filefd, readbytes;
    char r_buf[DATA_BUFFER]; 
    char *line = "/", *filename = NULL, *token;
    buf+=1; 
    if(strlen(buf) ==0) { 
        sendE("No path given.\n", listenfd);
        return -1;
    }
    token = strtok(buf, line); 
    while(token != NULL) {
        if(filename != NULL) free(filename);
        filename = strdup(token); 
        token = strtok(NULL, line); 
    }
    filename[strlen(filename)-1] = '\0';
    if( access(filename, F_OK) == 0) { 
        sendE("File already exists locally.\n", listenfd); 
        return -1;
    }
    filefd = open(filename, O_RDWR | O_CREAT , 0700);
    if( lseek(filefd, 0, SEEK_SET) < 0) fprintf(stderr, strerror(errno), sizeof(strerror(errno))); 
    while( (readbytes = read(datalistenfd, r_buf, sizeof(r_buf))) > 0) { 
        if(D_FLAG) printf("Read %d bytes from server, writing same to local file.\n", readbytes); 
        write(filefd, r_buf, readbytes); 
        //CHECK WRITE FOR ERRRORS
    }
    if(readbytes < 0) { 
        fprintf(stderr, "Problem with reading datasocket\n"); 
        unlink(filename);
    } 
    close(filefd);
    free(filename);
}