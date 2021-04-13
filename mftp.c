#include "mftp.h" 

#define ACKNOWLEDGED "A"
int D_FLAG;



int main(int argc, char const *argv[])
{
    struct addrinfo hints, *actualdata; 
    int socketfd, readbytes, err, cmd; 
    char *init_err = "Usage: ./mftp [-d] <port> <hostname | IP address>\n";
    char *mftp = "MFTP>";
    char *dflag = "-d"; 

    char buf[512] = {0}; 

    memset(&hints, 0, sizeof(hints)); 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_family = AF_INET; 


    //Get info on server and set contents of actual data.
    if  (argc == 1) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fflush(stderr);
            exit(1);
    }
    else if (strcmp(argv[1], dflag) ==0) { 
        D_FLAG = 1; 
        if( (err = getaddrinfo(argv[3], argv[2], &hints, &actualdata)) !=0) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fflush(stderr);
            exit(1);
        }
    }
    else { 
        if( (err = getaddrinfo(argv[2], argv[1], &hints, &actualdata)) !=0) { 
            write(STDERR_FILENO, init_err, strlen(init_err));
            fflush(stderr);
            exit(1);
        }
    }

    if( (socketfd = socket(actualdata->ai_family, actualdata->ai_socktype, 0)) < 0) { 
        perror("Error");
        exit(1);
    }
    if( connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen) < 0) { 
        perror("Error"); 
        exit(1); 
    }
    if(D_FLAG) { 
        printf("Debug flag detected.\n");
        printf("Created socket with descriptor %d\n", socketfd);  
        printf("Connected to server %s\n", argv[3]);
    }
    else { 
        printf("Connected to server %s\n", argv[2]);
    }
    
    //Main loop
    while(1) { 
         int pid; 
        //Always write mftp> before getting command
        write(STDOUT_FILENO, mftp, strlen(mftp));
        //Get command from user.
        if ((readbytes = read(STDIN_FILENO, buf, 256)) > 0) { 
            if(D_FLAG) printf("Commmand string = %s", buf);

            if( (cmd = local_cmd(buf)) > 0) { //Check if commmand is for local
                switch (cmd){
                case 1:   //EXIT PROGRAMS
                    exit_cmd(socketfd, buf, D_FLAG); 
                    break;
                
                case 2: // LS COMMAND  
                    ls_cmd(D_FLAG); 
                    break;
                case 3: // CD COMMAND
                    cd_cmd(buf, D_FLAG); 
                    break;
                default:
                    break;
                }
            }
            else if( (cmd = server_cmd(buf)) > 0) { //Check if command is for server
               switch (cmd){
                case 1: // RLS COMMAND
                    if(D_FLAG) {  
                        printf("Executing remote ls command\n");
                        rls_cmd(socketfd, buf, D_FLAG, argv[3]); 
                    }
                    else {  
                        rls_cmd(socketfd, buf, D_FLAG, argv[2]); 
                    } 
                    break;
                case 2: //RCD COMMAND
                    if(D_FLAG) {  
                        printf("Executing remote cd command\n");
                        rcd_cmd(socketfd, buf, D_FLAG, argv[3]); 
                    }
                    else {  
                        rcd_cmd(socketfd, buf, D_FLAG, argv[2]); 
                    } 
                   break;
                case 3:
                   break;
                case 4:
                   break;
                case 5:
                   break;
                default:
                    break;
               }
            }
            else { //If neither then we do not recognize command
                buf[strlen(buf)-1] = '\0';
                fprintf(stderr, "Command: '%s' is unknown -ignored.\n", buf);
                fflush(stderr);
            }

            memset(buf, 0, sizeof(buf));
        }
    } 
    return 0;
}
int local_cmd(char *buf) { 
    char *ls = "ls\n", *cdn = "cd\n", *exit = "exit\n", *cd = "cd";
    char *space = " ";
    char *token = strtok(buf, space);
    if(token != NULL) { 
        if(strcmp(token, exit) ==0) { 
            return 1; 
        }
        else if (strcmp(token, ls) ==0) { 
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
    char *rlsn = "rls\n",*rls = "rls", *rcd = "rcd",*rcdn = "rcd\n", *get = "get", *show = "show", *put = "put"; 
    char *space = " "; 
    char *token = strtok(buf, space); 

    if(token != NULL) { 
        if(strcmp(token, rlsn) ==0 || strcmp(token,rls) ==0 ) {
            return 1;     
        }
        else if(strcmp(token, rcd)==0 || strcmp(token, rcdn) ==0) { 
            return 2; 
        }
        else if (strcmp(token, get) ==0) {
            return 3; 
        } 
        else if (strcmp(token, show) ==0) { 
            return 4;
        } 
        else if (strcmp(token, put) ==0) {
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
    write(socketfd, server_quit, strlen(server_quit));
    
    if(D_FLAG) printf("Waiting server Response..\n");

    readbytes = read(socketfd, buf, sizeof(buf)); 

    if(D_FLAG) {  
        printf("Server response: %c\n", buf[0]);
    } 
    /*GET SERVER RESPONSE*/
    if(buf[0] == 65) {  // response A
        write(STDOUT_FILENO, exit_str, strlen(exit_str));
        fflush(stdout);
        exit(1);
    }
    else if(buf[0] == 69) { 
        write(STDERR_FILENO, buf, readbytes); 
        fflush(stderr);
    }

}
int ls_cmd(int D_FLAG) { 
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
        if(D_FLAG) printf("Changing to Path %s\n", buf); 
        err = chdir(buf); 
        if (err < 0) printf("%s\n", strerror(errno)); 
        printf("Changed cwd to %s\n", getcwd(cwd, 100)); 
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
    

    return f_newport;
} 

int rls_cmd(int socketfd, char *buf, int D_FLAG, const char* arg) { 
    int reader, writer, readbytes, datasocket; 
    
    buf += 4; 
    if(strlen(buf) !=0) { 
        printf("rls command invald.\n");
        close(datasocket);
        return 1;
    }
    datasocket = get_datasocket(socketfd, D_FLAG, arg);

    int fd[2]; 
    char *ls = "L\n";

    // Write our command to main socketfd connection.

    write(socketfd, ls, strlen(ls)); 

    if(D_FLAG) printf("Waiting server Response..\n");
    readbytes = read(socketfd, buf, sizeof(buf)); 

    if(D_FLAG) {  
        printf("Server response: %c\n", buf[0]);
    } 
    /*GET SERVER RESPONSE*/
    if(buf[0] == 69) { 
        write(STDERR_FILENO, buf, readbytes); 
        fflush(stderr);
    }

    if(pipe (fd) < 0) { 
        printf("%s\n", strerror(errno)); 
        exit(-1); 
    }  

    pipe(fd); 
    writer = fd[1];
    reader = fd[1];

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
int rcd_cmd(int socketfd, char *buf, int D_FLAG, const char* arg) {
    char *space = " "; 
    char *c = "C"; 
    int err, readbytes;
    char cwd[100]; 
    char *ptr = buf; 
    buf += 4;
    if(strlen(buf) ==0) { 
        printf("Command error: rcd needs a paramter.\n"); 
    }
    else {
        //More robust checking needed. 
        write(socketfd, c, strlen(c)); 
        write(socketfd, buf, strlen(buf));
        if(D_FLAG) printf("Awaiting server response.."); 
        readbytes = read(socketfd, cwd, sizeof(buf)); 

        if(D_FLAG) {  
            printf("Server response: %c\n", buf[0]);
        } 
        /*GET SERVER RESPONSE*/
        if(cwd[0] == 69) { 
            write(STDERR_FILENO, buf, readbytes); 
            fflush(stderr);
    }
        else {        
            printf("Changed remote path to: %s", buf); 
        }
    }    

}