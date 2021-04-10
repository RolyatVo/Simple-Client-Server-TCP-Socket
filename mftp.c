#include "mftp.h" 




int local_cmd(char *buf);
int server_cmd (char*buf);
int run_local(int cmd);



int main(int argc, char const *argv[])
{
    struct addrinfo hints, *actualdata; 
    int socketfd, readbytes, err, cmd; 
    char *init_err = "Usage: ./mftp [-d] <port> <hostname | IP address>\n";
    char *exit_str = "Client exiting normally.\n";
    char *mftp = "mftp>";

    char buf[256] = {0}; 

    memset(&hints, 0, sizeof(hints)); 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_family = AF_INET; 


    //Get info on server and set contents of actual data.
    if( (err = getaddrinfo(argv[2], argv[1], &hints, &actualdata)) !=0) { 
        write(STDERR_FILENO, init_err, strlen(init_err));
        fflush(stderr);
        exit(1);
    }

    if( (socketfd = socket(actualdata->ai_family, actualdata->ai_socktype, 0)) < 0) { 
        perror("Error");
        exit(1);
    }
    if( connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen) < 0) { 
        perror("Error"); 
        exit(1); 
    }
    

    //Main loop
    while(1) { 
        //Always write mftp> before getting command
        write(STDOUT_FILENO, mftp, strlen(mftp));
        if ((readbytes = read(STDIN_FILENO, buf, 256)) > 0) { 
            //Get rid of newline.
            buf[strlen(buf)-1] = '\0';
            if( (cmd = local_cmd(buf)) > 0) { //Check if commmand is for local
                if(cmd == 1) { 
                    


                    /* CLEAN UP GOES HERE */

                    write(stdout, exit_str, strlen(exit_str));
                    exit(1);
                }
                run_local(cmd); 
            }
            else if( (cmd = server_cmd(buf)) > 0) { //Check if command is for server
               //run_server(cmd);
            }
            else { //If neither then we do not recognize command
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