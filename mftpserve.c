#include "mftp.h"
#define DEFAULT_PORTNUM     49999
#define DEFAULT_PORTNUM_STR "49999"
#define BACKLOG             4
#define PATH_MAX            4096
#define DATA_BUFFER         512
#define READ_BUFFER         1

int childprocess(int listenfd);
int exit_cmd(int listenfd);
int cd_cmd(int listenfd, char* buf);
int sendE(const char* message, int listenfd);
int sendA (int listenfd, int pid);
int getdatasocket(int listenfd);
int ls_cmd(int datalistenfd);
int get_cmd(int listenfd, int datalistenfd, char*buf); 
int checkfile(char *inputPath);
int checkdir( char* inputPath, int listenfd);
int put_cmd(int listenfd, int datalistenfd, char *buf);


int D_FLAG; 
const char *USER_PORT = NULL; 
int main(int argc, char const *argv[])
{

    //Set up debug mode and obtaining port number if given
    char *debug_flag = "-d", *port_flag = "-p"; 
    USER_PORT = DEFAULT_PORTNUM_STR;
    if(argc > 1) { 
        for(int i = 1; i <argc; i++)  {
            if(strcmp(argv[i], debug_flag) ==0) D_FLAG = 1; 
            if(strcmp(argv[i], port_flag) ==0)  USER_PORT = argv[i+1]; 
        } 
    } 
    if(USER_PORT == NULL) USER_PORT = DEFAULT_PORTNUM_STR; //If user did -p on accident with no portnum
    
    if(D_FLAG) printf("Debug flag detected. \n");

    struct sockaddr_in server_address, client_address; 
    int socketfd, listenfd, length = sizeof(struct sockaddr_in); 

    //Set up server just like we did in assignment 8
    if( (socketfd = socket(AF_INET, SOCK_STREAM, 0)) ==0) { 
        perror("Error"); 
        exit(-1); 
    } 
    if(D_FLAG) printf("socketfd was created with a descriptor %d\n", socketfd); 
    //Some options for socketfd
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) { 
        perror("Error");
        exit(-1); 
    }

    memset(&server_address, 0, sizeof(struct sockaddr_in)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(atoi(USER_PORT));

    //bind that socketfd we got with setup structs we initalized.
    if( bind(socketfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in) ) ) { 
        perror("Error");
        exit(-1); 
    }
    if(D_FLAG) printf("P: socket bounded to port %s\n", USER_PORT); 

    //Listen with a backlog of 4
    if( listen(socketfd, BACKLOG) < 0) { 
        perror("Error"); 
        exit(-1);
    } 
    if(D_FLAG) printf("P: listen with connection queue of %d\n", BACKLOG); 

    char hostName[NI_MAXHOST];
    int hostEntry, pid, status;



    while(1) { //Main loop for getting connections.
        //When connection accepted fork off and go into child process. Also get some info on client
        if( (listenfd = accept(socketfd, (struct sockaddr*) &client_address, &length)) ) { 
            if(D_FLAG) printf("P: accepted client connection with descriptor %d\n", listenfd); 
            pid = fork(); 
            if(pid < 0) fprintf(stderr,"Was not able to fork off child.."); 
            else if(pid ==0) { 
                //info on client
                if((hostEntry = getnameinfo((struct sockaddr*)&client_address, sizeof(client_address), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV)) != 0) { 
                    fprintf(stderr, "Error: %s", gai_strerror(hostEntry));
                    fflush(stderr);
                }
                printf("P: Connection accepted from %s\n", hostName); 

                //hand off to childprocess 
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


        if (readbytes >0 ) { //Check to see if we get anything
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
                    if(datalistenfd > 0) sendE("Data socket already exists\n", listenfd); 
                    else datalistenfd = getdatasocket(listenfd); 
                    break;
                case 76: // <L> command 
                // Check to see if they established a data connection.
                    if(datalistenfd < 0) sendE("Data connection was never established\n", listenfd);
                    else {
                        //Send A then go into ls when done close off and set listenfd
                        sendA(listenfd, getpid());   
                        ls_cmd(datalistenfd);
                        close(datalistenfd);
                        datalistenfd = -20;
                    }
                    break;
                case 71: // G command
                    if(datalistenfd < 0) sendE("Data connection was never established\n", listenfd);
                    else { 
                        get_cmd(listenfd, datalistenfd, buf); 
                        close(datalistenfd);
                        datalistenfd = -20;
                    }
                    break; 
                case 80: // P command
                    if(datalistenfd < 0) sendE("Data connection was never established\n", listenfd);
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
        if(readbytes ==0) { //if socket was closed.
            close(listenfd); 
            printf("C %d: Connection closed unexpectedly or EOF\n", getpid());
            exit(-1);
        }
    }
}

int sendA (int listenfd, int pid) { //Function to send A to client whenever sucessful.
    char *posA = "A\n";
    if (D_FLAG) printf("C %d: Sending positive acknowledgement\n", pid); 
    if(write(listenfd, posA, strlen(posA)) < 0) { //if problem with write itself.
        fprintf(stderr, "error writing A to socket\n");
        return -1;
    }
    return 0;
} 
int sendE(const char* message, int listenfd) { //Function to send E to client if theres a failure.
    char *E= "E"; 
    if(D_FLAG) printf("Sending E response %s", message);
    printf("C %d: ERROR: %s", getpid(), message); 
    write(listenfd, E, strlen(E));
    write(listenfd, message, strlen(message));
} 
int exit_cmd(int listenfd) {
    //Send A to client and exit normally. 
    int pid = getpid();
    printf("C %d: Quitting\n", pid);
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
    if(checkdir(buf, listenfd) < 0) { //Check if we have perms for that dir
        fprintf(stderr,"Problem with given path\n");
        return -1;
    } 
    err = chdir(buf); //if we have perms cd into dir
    if (err < 0){ //problem with chdir
        fprintf(stderr,"Error: %s\n", strerror(errno));
        sendE("Could not cd to given path.\n", listenfd); 
        return -1;
    } 
    else printf("C %d: changed cwd to %s\n", getpid(), buf);
    buf--;
    sendA(listenfd, pid); 

}   

int getdatasocket(int listenfd) {
    //Set up datasocket, similar to setting up regular server socket. \
    but with a bind to port 0
    char*a = "A"; 
    char *newn = "\n";
    char port[20] = {0}; 
    if(D_FLAG) printf("C %d: Establishing data connection\n", getpid());
    struct sockaddr_in server_address, client_address; 
    int datasocketfd, datalistenfd, hostEntry, length = sizeof(struct sockaddr_in); 
    char hostName[NI_MAXHOST] = {0};

    //get fd from socket
    if( (datasocketfd = socket(AF_INET, SOCK_STREAM, 0)) ==0) { 
        sendE("Socket Invalid\n", listenfd);
        perror("Error"); 
        exit(-1); 
    } 
    if(D_FLAG) printf("C %d: datasocketfd was created with a descriptor %d\n",getpid(),  datasocketfd); 
    if(setsockopt(datasocketfd, SOL_SOCKET, SO_REUSEADDR,  &(int){1}, sizeof(int))) { 
        sendE("Setsockopt Invalid\n", listenfd);
        perror("Error");
        exit(-1); 
    }
    //Set up like regular server but with a portnum of 0 to itll find a port for us.
    memset(&server_address, 0, sizeof(struct sockaddr_in)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = 0;

    //bind with port 0 
    if( bind(datasocketfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr_in) ) < 0) { 
        sendE("Could not bind\n", listenfd);
        perror("Error");
        exit(-1); 
    }
    //set contents of server_adress
    if (getsockname(datasocketfd, (struct sockaddr*) &server_address, &length) < 0) { 
        sendE("Could not getsockanme\n", listenfd);
        perror("Error");
        exit(-1);
    }
    if(D_FLAG) printf("C %d: socket bounded to port %d\n",getpid(),  ntohs(server_address.sin_port)); 

    //Listen with back log of 1
    if( listen(datasocketfd, 1) < 0) {
        sendE("Could not listen\n", listenfd); 
        perror("Error"); 
        exit(-1);
    }

    //Sending client A<portnum>\n 
    if(D_FLAG) printf("C %d; listening to data socket\n", getpid());
    sprintf(port, "A%d\n", ntohs(server_address.sin_port)); 
    write(listenfd, port, strlen(port));

    if(D_FLAG) printf("C %d: Sent acknowledgement -> %s",getpid(), port);

    //Waiting for client to accept the connection.
    if( (datalistenfd = accept(datasocketfd, (struct sockaddr*) &client_address, &length)) ) { 
        if((hostEntry = getnameinfo((struct sockaddr*)&client_address, sizeof(client_address), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV)) != 0) { 
                //get info on client
                fprintf(stderr, "Error: %s", gai_strerror(hostEntry));
                fflush(stderr);
            }
        if(D_FLAG) { 
            printf("C %d: Accepted connection from %s on data socket with fd %d\n", getpid(), hostName, datalistenfd);
            printf("C: %d Data socket port on client is %d\n", getpid(), ntohs(client_address.sin_port));
        }
        //close the unleeded data asocket only need listen.
        close(datasocketfd);
        return datalistenfd;
    }
}
int ls_cmd(int datalistenfd) { 
    //ls command, have child execute ls command and redirect stdout to datalisten
    int reader, writer; 

    int pid = fork(); 
    if(pid < 0) { 
        printf("ERROR: Problem with forking.\n");
        exit(-1); 
    } 
    else if (pid ==0) { //Child directing output to datalisten
        dup2(datalistenfd, STDOUT_FILENO); close(datalistenfd);

        execlp("ls", "ls", "-l", (char*) NULL); 
        printf("%s\n", strerror(errno));
        exit(-1);
    } 
    else { //parent just waits for child to finish.
        waitpid(pid, NULL, 0); 
        printf("C %d: ls to client command completed\n", getpid());
    }
} 

int get_cmd(int listenfd, int datalistenfd, char*buf) { 

    int filefd, readbytes;
    char r_buf[DATA_BUFFER]; 
    buf+=1; 
    if(strlen(buf) ==0) { //If there wasnt a path given along with G, send error 
        sendE("No path given.\n", listenfd);
        return -1;
    }
    buf[strlen(buf)-1] = '\0'; //removing newline from path
    if( access(buf, F_OK) != 0) { //Check if we have access and it exists. 
        sendE("File does not exist in path.\n", listenfd); 
        return -1;
    }
    else if(checkfile(buf) < 0 || access(buf, R_OK) != 0) { //Check if its regular and readable. 
        sendE("File is not readable/regular.\n", listenfd);
        return -1;
    }
    else { //Send A to let client know we are transmitting
        sendA(listenfd, getpid()); 
        filefd = open(buf, O_RDONLY); //open file in read only mode
    }
    if( lseek(filefd, 0, SEEK_SET) < 0) fprintf(stderr, strerror(errno), sizeof(strerror(errno))); //Go to beginning of file.
    printf("C %d: Transmitting %s to client.\n", getpid(), buf);
    while( (readbytes = read(filefd, r_buf, sizeof(r_buf))) > 0) { //Read BUFFER at a time and write what ever we read to datalisten
        if(D_FLAG) printf("Read %d bytes from local path, writing to server.\n", readbytes); 
        write(datalistenfd, r_buf, readbytes);
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
int checkdir( char* inputPath, int listenfd) { //Function to check dir permissions
    struct stat area, *s = &area; 
    DIR *dir;
    char cwd[PATH_MAX + 1]; //longest possible path with null
    if(access(inputPath, F_OK) !=0) { //if we cant access or it doesnt exist.
        sendE("Directory does not exist!\n", listenfd);    
        return -1;
    } 
    if(stat(inputPath, s) ==0 && (S_ISDIR(s->st_mode) ==1)) { //Check to see if its dir and we have rx permissions.
        if((s->st_mode & S_IXUSR) && (s->st_mode & S_IRUSR)) { 
            return 1;
        }
        else { //if no perms then return error
            sendE("No permissions for this directory\n", listenfd);
            return -1;
        } 
    }
} 
int put_cmd(int listenfd, int datalistenfd, char *buf) { 
    int filefd, readbytes, writebytes;
    char r_buf[DATA_BUFFER]; 
    char *line = "/", *filename = NULL, *token;
    buf+=1; 
    if(strlen(buf) ==0) { //check if path was given along with P
        sendE("No path given.\n", listenfd);
        return -1;
    }
    token = strtok(buf, line); 
    while(token != NULL) { //Get the file name
        if(filename != NULL) free(filename);
        filename = strdup(token); 
        token = strtok(NULL, line); 
    }
    filename[strlen(filename)-1] = '\0'; //remove newline from filename
    if( access(filename, F_OK) == 0) { //Check if we have access and it exists.
        sendE("File already exists locally.\n", listenfd); 
        return -1;
    }
    sendA(listenfd, getpid()); 
    printf("C %d: Recieving %s and now reading.\n", getpid(), filename);
    filefd = open(filename, O_RDWR | O_CREAT , 0700); //Create new file with perms and prepare to write. 

    if( lseek(filefd, 0, SEEK_SET) < 0) fprintf(stderr, strerror(errno), sizeof(strerror(errno))); //set seek
    while( (readbytes = read(datalistenfd, r_buf, sizeof(r_buf))) > 0) { //Read from datalisten and write to file 
        if(D_FLAG) printf("Read %d bytes from server, writing same to local file.\n", readbytes); 
            writebytes = write(filefd, r_buf, readbytes); 
            if(writebytes < 0) { //if write somehow fails, unlink
                fprintf(stderr, "Problem with writing file\n"); 
                unlink(filename);
                close(filefd);
                free(filename); 
                break;           
                return -1;    
            }
    }
    //If theres a problen while reading, unlink
    if(readbytes < 0) { 
        fprintf(stderr, "Problem with reading datasocket\n"); 
        unlink(filename);
        close(filefd);
        free(filename);
        return -1;
    } 
    printf("C %d: %s to server transfer complete.\n", getpid(), filename);
    //Clean up.
    close(filefd);
    free(filename);
}