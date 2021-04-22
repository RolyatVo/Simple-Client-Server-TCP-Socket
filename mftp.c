#include "mftp.h" 
//Check if write fails, unlink 
//Double check for a connection if first fails
//keep reading until new lines, do not put assume/anything in static arrays.
//Check if we cant obtain data port number
//lstat cd and remote cd
//connect can be fatal error
int local_cmd(char *buf);
int server_cmd (char*buf);
int exit_cmd(int socketfd, char *buf, int D_FLAG); 
int get_datasocket(int socketfd, int D_FLAG, const char* ip); 
int rls_cmd(int socketfd, char *buf, int D_FLAG, const char* arg);
int ls_cmd(char*buf, int D_FLAG);
int cd_cmd(char *buf, int D_FLAG); 
int rcd_cmd(int socketfd, char *buf, int D_FLAG, const char* arg);
int show_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);
int fork_to_more(int datasocket);
int get_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);
int checkfile(char *inputPath);
int put_cmd(int socketfd, char*buf, int D_FLAG, const char*arg);
int server_response(int socketfd, char * r_buf, int size);
int checkconnection(int readbytes, int socketfd);
int checkdir(char *inputPath);

#define READ_BUFFER 1
#define DATA_BUFFER 512
int D_FLAG;

int main(int argc, char const *argv[]){
    struct addrinfo hints, *actualdata; 
    int socketfd, readbytes, err, cmd; 
    char *init_err = "Usage: ./mftp [-d] <port> <hostname | IP address>\n";
    char *mftp = "MFTP>";
    char *dflag = "-d"; 
    char read_buf[1] = {0};
    char buf[PATH_MAX + 6] = {0};
    const char *ip;
    
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_family = AF_INET; 


    //Get info on server and set contents of actual data.
    if  (argc == 1) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fflush(stderr);
            free(actualdata);
            exit(1);
    }
    else if (strcmp(argv[1], dflag) ==0) { 
        D_FLAG = 1; 
        if( (err = getaddrinfo(argv[3], argv[2], &hints, &actualdata)) !=0) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fprintf(stderr,"%s\n", gai_strerror(err));
            fflush(stderr);
            free(actualdata);
            exit(1);
        }
    }
    else { 
        if( (err = getaddrinfo(argv[2], argv[1], &hints, &actualdata)) !=0) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fprintf(stderr,"%s\n", gai_strerror(err));
            fflush(stderr);
            free(actualdata);
            exit(1);
        }
    }

    if( (socketfd = socket(actualdata->ai_family, actualdata->ai_socktype, 0)) < 0) { 
        fprintf(stderr, "Failed socket: ");
        perror("");
        free(actualdata);
        exit(1);
    }
    if( connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen) < 0) { 
        fprintf(stderr, "Failed connecting to server: ");
        perror("");
        free(actualdata);
        exit(1); 
    }

    if(D_FLAG) { 
        printf("Debug flag detected.\n");
        printf("Created socket with descriptor %d\n", socketfd);  
        printf("Connected to server %s\n", argv[3]);
        ip = argv[3]; 
    }
    else { 
        printf("Connected to server %s\n", argv[2]);
        ip = argv[2]; 
    }
    
    //Main loop
    while(1) { 
        int pid; 
        //Always write mftp> before getting command
        write(STDOUT_FILENO, mftp, strlen(mftp));
        //Get command from user.
        while(readbytes = read(STDIN_FILENO, read_buf, 1) > 0) { 
            strncat(buf, read_buf, 1);
            if(read_buf[0] == '\n') break;
        }
        if (readbytes > 0) { 
            if(D_FLAG) printf("Commmand string = %s", buf);
            
            if( (cmd = local_cmd(buf)) > 0) { //Check if commmand is for local
                switch (cmd){
                case 1:   //EXIT PROGRAMS
                    free(actualdata);
                    exit_cmd(socketfd, buf, D_FLAG); 
                    break;
                case 2: // LS COMMAND  
                    ls_cmd(buf, D_FLAG); 
                    break;
                case 3: // CD COMMAND
                    cd_cmd(buf, D_FLAG); 
                    break;
                }
            }
            else if( (cmd = server_cmd(buf)) > 0) { //Check if command is for server
               switch (cmd){
                case 1: // RLS COMMAND
                    if(D_FLAG) printf("Executing remote ls command\n");
                    rls_cmd(socketfd, buf, D_FLAG, ip); 
                    break;
                case 2: //RCD COMMAND
                    if(D_FLAG) printf("Executing remote cd command\n");
                    rcd_cmd(socketfd, buf, D_FLAG, ip); 
                    break;
                case 3: //GET COMMAND
                    if(D_FLAG) printf("Executing get command\n");
                    get_cmd(socketfd, buf, D_FLAG, ip);
                    break;
                case 4: //SHOW COMMAND
                    if(D_FLAG) printf("Executing show command\n");
                    show_cmd(socketfd, buf, D_FLAG,ip);
                    break;
                case 5: //PUT COMMAND
                    if(D_FLAG) printf("Executing put command\n");
                    put_cmd(socketfd, buf, D_FLAG, ip);
                    break;
               }
            }
            else { //If neither then we do not recognize command
                buf[strlen(buf)-1] = '\0';
                fprintf(stderr, "Command: '%s' is unknown -ignored.\n", buf);
                fflush(stderr);
            }
            memset(buf, 0, sizeof(buf));
            memset(read_buf, 0, sizeof(read_buf));
        }
    } 
    return 0;
}

int local_cmd(char *buf) { 
    char *lsn = "ls\n",*ls ="ls", *cdn = "cd\n", *exit = "exit\n", *cd = "cd";
    char *space = " ";
    char *token = strtok(buf, space);
    if(token != NULL) { 
        if(strcmp(token, exit) ==0) { 
            return 1; 
        }
        else if (strcmp(token, ls) ==0 || strcmp(token, lsn) ==0) { 
            return 2; 
        }
        else if (strcmp(token, cdn) ==0 || strcmp(token, cd) ==0) { 
            return 3;
        }
        else { 
            return -1;
        }
    }
    else { 
        return -1;
    }    
}

int server_cmd (char*buf) {
    //List of commands
    char *rls = "rls",*rlsn = "rls\n";
    char *rcd = "rcd",*rcdn = "rcd\n";
    char *get = "get", *getn = "get\n";
    char *show = "show", *shown = "show\n";
    char *put = "put", *putn = "put\n";
    char *space = " "; 
    char *token = strtok(buf, space); 

    if(token != NULL) { 
        if(strcmp(token, rlsn) ==0 || strcmp(token,rls) ==0 ) {
            return 1;     
        }
        else if(strcmp(token, rcd)==0 || strcmp(token, rcdn) ==0) { 
            return 2; 
        }
        else if (strcmp(token, get)==0 || strcmp(token, getn) ==0) {
            return 3; 
        } 
        else if (strcmp(token, show)==0 || strcmp(token, shown) ==0) { 
            return 4;
        } 
        else if (strcmp(token, put)==0 || strcmp(token, putn) ==0) {
            return 5;
        }
        else { 
            return -1;
        } 
    }
}

int exit_cmd(int socketfd, char *buf, int D_FLAG) { 
    int readbytes; 
    char *server_quit = "Q\n";
    char *exit_str = "Client exiting normally.\n"; 

    if(D_FLAG) printf("Giving server command: %s", buf);
    if(write(socketfd, server_quit, strlen(server_quit)) < 0) { 
        fprintf(stderr, "error writing to server exiting...\n");
        exit(-1);
    }
    if (server_response(socketfd,buf, 512) < 0) exit(-1);
    else { 
        write(STDOUT_FILENO, exit_str, strlen(exit_str));
        fflush(stdout);
        exit(1);        
    }
}

int ls_cmd(char *buf, int D_FLAG) { 
    buf += 3;
    if(strlen(buf) !=0) { 
        printf("Command error: ls does not need a paramter.\n"); 
        return -1;
    }
    int reader, writer; 
    int fd[2]; 
    if(pipe(fd) < 0) { 
        fprintf(stderr, "%s\n", strerror(errno)); 
        exit(-1); 
    }
    else { 
        pipe(fd); 
    } 
    writer =fd[1]; 
    reader =fd[0]; 
    int pid;
    if((pid = fork()) ==0) { 
        int pid2 = fork(); 
        if(pid2 < 0) {   //Check if fork() worked. 
            fprintf(stderr, "Error occured while forking."); 
            exit(-1); 
        } 
        else if(pid2 ==0) { //Child process
            dup2(writer, STDOUT_FILENO); 
            close(reader);
            close(writer);
            execlp("ls","ls","-l", (char*) NULL); 
            printf("%s\n", strerror(errno));
            exit(-1);
        }
        else if (pid2) { 
            if(D_FLAG) printf("Waiting for pid to finish\n"); 
            
            waitpid(pid2,NULL,0); 
            
            if(D_FLAG) printf("%d finished now executing more -20\n",pid); 

            dup2(reader,STDIN_FILENO); 
            close(writer);
            close(reader);
            execlp("more", "more", "-20", (char*) NULL); 
            printf("%s\n", strerror(errno));
            exit(-1);
        }
        close(reader);
        close(writer);
    }
    else {
        close(writer);
        close(reader);
        waitpid(pid, NULL, 0);

    }
    fflush(stdout);
    return 0;
}

int cd_cmd(char *buf, int D_FLAG) { 
    char *space = " "; 
    int err;
    char cwd[100]; 
    char *ptr = buf; 
    buf += 3;
    if(strlen(buf) ==0) { 
        printf("Command error: cd needs a paramter.\n"); 
    }
    else { 
        buf[strlen(buf)-1] = '\0'; 
        if(checkdir(buf) < 0) { 
            if(D_FLAG) printf("Problem with given path\n");
            return -1;
        } 
        if(D_FLAG) printf("Changing to Path %s\n", buf); 
        err = chdir(buf); 
        if (err < 0) fprintf(stderr, "%s\n", strerror(errno));  
    }    

}

int get_datasocket(int socketfd, int D_FLAG, const char* ip) { 
    struct addrinfo h, *actdata; 
    struct sockaddr_in* server; 
    char r_buf[100] = {0}; 
    int readbytes, f_newport; 
    char *newport;
    char *d = "D\n"; 
    char *failed_connection = "Failed data socket connection\n";
    memset(&h, 0, sizeof(h));
    h.ai_socktype = SOCK_STREAM; 
    h.ai_family = AF_INET; 

    write(socketfd, d, strlen(d));  
    if(D_FLAG) printf("Sent D command to server\n");

    if(D_FLAG) printf("Waiting server Response..\n");
    if( (readbytes = read(socketfd, r_buf, sizeof(r_buf))) >0) {
        r_buf[readbytes-1] = '\0';
        if(D_FLAG) printf("Recieved server response: '%s'\n", r_buf);

        newport = r_buf;
        newport++;
        if(D_FLAG)  printf("Obtained port number '%s' from server\n", newport); 
    }
    if(checkconnection(readbytes, 1) ==0);

    //Create data socket with description
    if(D_FLAG) printf("IP: %s\nNEWPORT: %s\n", ip, newport);
    if( (getaddrinfo(ip,newport , &h, &actdata)) !=0) { 
        write(STDERR_FILENO, failed_connection, strlen(failed_connection));
        fflush(stderr);
        exit(1);
    }
    if( (f_newport = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("Error");
        exit(1);
    }
    if(D_FLAG) printf("Created data socket with descriptor %d\n", f_newport); 
    server = (struct sockaddr_in*)actdata->ai_addr; 

    if(D_FLAG) { 
        printf("hostname: %s\n", inet_ntoa(server->sin_addr));
        printf("Attempting to establish Data connection\n"); 
    }
    if( connect(f_newport, actdata->ai_addr, actdata->ai_addrlen) < 0) { 
        perror("Error"); 
        exit(1); 
    }
    if(D_FLAG) printf("Data connetion to server established\n"); 
    free(actdata);

    return f_newport;
} 

int rls_cmd(int socketfd, char *buf, int D_FLAG, const char* arg) { 
    int reader, writer, readbytes, datasocket; 
    char r_buf[50]; 
    buf += 4; 
    if(strlen(buf) !=0) { 
        printf("rls command invald.\n");
        return 1;
    }
    datasocket = get_datasocket(socketfd, D_FLAG, arg);

    char *ls = "L\n";
    // Write our command to main socketfd connection.
    write(socketfd, ls, strlen(ls)); 

    if( server_response(socketfd, r_buf, sizeof(r_buf)) < 0)  return -1; 

    fork_to_more(datasocket);
}

int rcd_cmd(int socketfd, char *buf, int D_FLAG, const char* arg) {
    char *space = " "; 
    char *c = "C"; 
    int err, readbytes;
    char r_buf[50]; 
    char *ptr = buf; 
    buf += 4;
    if(strlen(buf) ==0) { 
        printf("Command error: rcd needs a paramter.\n"); 
    }
    else {
        //More robust checking needed. 
        checkconnection(1, socketfd);
        if(write(socketfd, c, strlen(c)) < 0) fprintf(stderr, "Error writing to socketfd\n"); 
        if(write(socketfd, buf, strlen(buf)) < 0) fprintf(stderr, "Error writing to socketfd\n"); 

        if( server_response(socketfd, r_buf, sizeof(r_buf)) < 0)  return -1; 

        else {        
            printf("Changed remote path to: %s", buf); 
        }
    }    
}

int show_cmd(int socketfd, char*buf, int D_FLAG, const char*arg) { 
    int datasocket, readbytes; 
    char r_buf[50];
    char *get= "G";

    buf+=5;
    if(strlen(buf) ==0) { 
        printf("Command error: show needs a paramter.\n"); 
        return -1;
    }

    datasocket = get_datasocket(socketfd, D_FLAG, arg);
    write(socketfd, get, strlen(get)); 
    write(socketfd, buf, strlen(buf)); 
    
    if( server_response(socketfd, r_buf, sizeof(r_buf)) < 0) { 
        close(datasocket);
        return -1; 
    }

    else {        
        printf("Showing file: %s", buf); 
    }
    fork_to_more(datasocket);     
}

int fork_to_more(int datasocket) {

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
        dup2(datasocket, STDIN_FILENO); 
        close(writer); 
        close(reader);
        execlp("more", "more", "-20", (char*) NULL); 
        printf("%s\n", strerror(errno));
        exit(-1);
    } 
    else { 
        close(reader);
        close(writer);
        waitpid(pid, NULL, 0); 
    }
    close(datasocket);
} 

int get_cmd(int socketfd, char*buf, int D_FLAG, const char*arg) { 
    int datasocket, readbytes, filefd, err;
    char r_buf[DATA_BUFFER]; 
    char *get = "G", *line = "/";
    char *token, *filename = NULL; 

    buf+=4;

    if(strlen(buf) ==0) { 
        printf("Command error: get needs a paramter.\n"); 
        return -1;
    }
    // Get the file name from path
    token = strtok(buf, line); 
    while(token != NULL) {
        if(filename != NULL) free(filename);
        filename = strdup(token); 
        token = strtok(NULL, line); 
    }
    filename[strlen(filename)-1] = '\0';
    printf("Getting File: %s\n", filename);
    if( access(filename, F_OK) == 0) { 
        printf("File already exists locally.\n"); 
        return -1;
    }

    datasocket = get_datasocket(socketfd, D_FLAG, arg); 
    write(socketfd, get, strlen(get));
    write(socketfd, buf, strlen(buf)); 

    if( server_response(socketfd, r_buf, sizeof(r_buf)) < 0) { 
        close(datasocket);
        free(filename);
        return -1; 
    }
    filefd = open(filename, O_RDWR | O_CREAT , 0700);

    if( lseek(filefd, 0, SEEK_SET) < 0) fprintf(stderr, strerror(errno), sizeof(strerror(errno))); 

    while( (readbytes = read(datasocket, r_buf, sizeof(r_buf))) > 0) { 
        if(D_FLAG) printf("Read %d bytes from server, writing same to local file.\n", readbytes); 
        write(filefd, r_buf, readbytes); 
    }
    if(readbytes < 0) { 
        fprintf(stderr, "Problem with reading datasocket\n"); 
        unlink(filename);
    } 
    close(filefd);
    close(datasocket);
    free(filename);
}   

int put_cmd(int socketfd, char*buf, int D_FLAG, const char*arg) { 
    //Rename r_buf to something else and make server reponse not take that.
    int datasocket, readbytes, filefd, err;
    char r_buf[DATA_BUFFER]; 
    char *put = "P", *line = "/";
    char *token, *filename = NULL; 
    buf+=4;
    strcpy(r_buf, buf);
    if(strlen(buf) ==0) { 
        printf("Command error: put needs a paramter.\n"); 
        return -1;
    }
    // Get the file name from path
    token = strtok(r_buf, line); 
    while(token != NULL) {
        if(filename != NULL) free(filename);
        filename = strdup(token); 
        token = strtok(NULL, line); 
    }
    buf[strlen(buf)-1] = '\0';
    filename[strlen(filename)-1] = '\0';
    if( access(buf, F_OK) != 0) { 
        printf("File does not exist in path.\n"); 
        return -1;
    }
    else if(checkfile(buf) < 0) {
        printf("File is not readable/regular.\n");
        return -1;
    }
    else { 
        filefd = open(buf, O_RDONLY); 
    }

    datasocket = get_datasocket(socketfd, D_FLAG, arg);

    filename[strlen(filename)] = '\n';
    write(socketfd, put, strlen(put)); 
    write(socketfd, filename, strlen(filename));


    if( server_response(socketfd, r_buf, sizeof(r_buf)) < 0) { 
        close(datasocket);
        free(filename);
        return -1; 
    } 

    while ( (readbytes = read(filefd, r_buf, sizeof(r_buf))) > 0) { 
        if(D_FLAG) printf("Read %d bytes from local path, writing to server.\n", readbytes); 
        write(datasocket, r_buf, readbytes);
    }
    close(filefd);
    close(datasocket);

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

int server_response(int socketfd, char * r_buf, int size) { 
    //!!Change server repsonse so that it reads one char at a time.!!
    
    int readbytes ; 
    if(D_FLAG) printf("Awaiting server response..\n"); 
    memset(r_buf, 0, size);
    readbytes = read(socketfd, r_buf,size); 
    if(checkconnection(readbytes, 1) == 0);

    if(D_FLAG) printf("Server response: %c\n", r_buf[0]); 
    /*GET SERVER RESPONSE*/
    if(r_buf[0] == 69) { 
        r_buf++;
        fprintf(stderr, "Server: %s", r_buf);
        return -1;
    }
    return 0;

} 

int checkconnection(int readbytes, int socketfd) { 
    if ( readbytes ==0 || fcntl(socketfd, F_GETFL) <0) { 
        fprintf(stderr, "Error: main socket closed unexpectedly\n");
        exit(-1);
    }
    return 0;
} 

int checkdir(char *inputPath) { 
    struct stat area, *s = &area; 
    DIR *dir;
    char cwd[PATH_MAX + 1]; 

    if(access(inputPath, F_OK) != 0) { 
        printf("ERROR: Directory does not exist!\n");
        return -1;
    }
    if((stat(inputPath, s) ==0) && (S_ISDIR(s->st_mode) ==1)) {
        if((s->st_mode & S_IRUSR) && (s->st_mode & S_IXUSR)) { 
            return 1;
        }
        else { 
            printf("ERROR: No permissions for this directory\n");
            return -1;
        }
    } 
      
} 