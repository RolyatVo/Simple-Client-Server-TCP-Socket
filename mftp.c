#include "mftp.h" 

#define ACKNOWLEDGED "A"
int D_FLAG;



int local_cmd(char *buf);
int server_cmd (char*buf);
int run_local(int cmd);



int main(int argc, char const *argv[])
{
    struct addrinfo hints, *actualdata; 
    int socketfd, readbytes, err, cmd; 
    char *init_err = "Usage: ./mftp [-d] <port> <hostname | IP address>\n";
    char *exit_str = "Client exiting normally.\n";
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
        //Always write mftp> before getting command
        write(STDOUT_FILENO, mftp, strlen(mftp));
        //Get command from user.
        if ((readbytes = read(STDIN_FILENO, buf, 256)) > 0) { 
        
            if(D_FLAG) printf("Commmand string = %s", buf);
            write(socketfd, buf, strlen(buf));

            if( (cmd = local_cmd(buf)) > 0) { //Check if commmand is for local
                char *server_quit = "exit:"; 
                switch (cmd){
                case 1:   //EXIT PROGRAMS

                    /* CLEAN UP GOES HERE */
                    write(socketfd, server_quit, strlen(server_quit));
                    
                    /*GET SERVER RESPONSE*/
                    // write(STDOUT_FILENO, exit_str, strlen(exit_str));
                    // exit(1);
                    
                    break;
                
                case 2: // LS COMMAND
                
                default:
                    break;
                }
            
                
            
            }
            else if( (cmd = server_cmd(buf)) > 0) { //Check if command is for server
               //run_server(cmd);
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
    char *ls = "ls", *cd = "cd", *exit = "exit";
    char *space = " ";
    char *token = strtok(buf, space);
    if(token != NULL) { 
        if(strcmp(token, exit) ==0) { 
            return 1; 
        }
        else if (strcmp(token, ls) ==0) { 
            return 2; 
        }
        else if (strcmp(token, cd) ==0) { 
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

}

int run_local(int cmd) { 
    
    switch (cmd)
    {
    case 2: //ls command
        printf("Got the 'ls' command!\n");
        break; 
    case 3: //cd command
        printf("Got the 'cd' command!\n");
        break; 
    default:
        break;
    }
}

// int reponse(socketfd) { 
//     char rbuf[3]; 
//     char *tmp; 
//     int rbuf_len=0;

//     while(tmp != '\n') { 
//         read(socketfd, tmp, 1); 
//         rbuf[rbuf_len++] = tmp; 
//         rbuf_len++;
//     }
// }