#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>

#define BUFF_SIZE 10000
#define READ 0
#define WRITE 1

char **args;

void sigintHandler(int signum)
{
  printf("\n[-]Exiting..\n");
  printf("\n[-]Server terminated. \n");
  exit(signum);
}

int redirection(char** arguments,char** output){
  int i=0;
  while(arguments[i]!=NULL)
  {
      if(strcmp(arguments[i],">")==0){
        *output=(char *)malloc(200 * sizeof(char ));
        arguments[i]=NULL;
        if(arguments[i+1]!=NULL){
          strcpy(*output,arguments[i+1]);
        }else{
          strcpy(*output,"error");
        }
        arguments[i+1]=NULL;
        return 1;
      }
      i++;
  }
  output=NULL;
  return 0;
}

int parser(char *command){
    if(command==NULL||command=="\n"){
        return 0;
    }
   int argnumber=0;
   //Divide the command in words delimited by spaces
   char *word = strtok(command, " ");
   //continues until the end of the command
    while( word != NULL ) {
        args[argnumber]=(char*)malloc(50*sizeof(char));
        strcpy(args[argnumber++],word );
        word = strtok(NULL, " ");
    }
    args[argnumber]=NULL;
    return argnumber;
}

int pipe_parsr(int *pipe_index,int argnumber){
  int pipe_count=0,i=0,j=0;
  while(i<argnumber||args[i]!=NULL)
  {
    if(strcmp(args[i],"|")==0)
    {
      pipe_count++;
      pipe_index[j++]=i;
      args[i]=NULL;
    }
    i++;
  }
 return pipe_count;
}

//Where the magic happens
int main(int argc, char *argv[])
{
    int sockfd, newSocket, portno;
    socklen_t addr_size;
    char buffer[BUFF_SIZE];
    struct sockaddr_in serverAddr, clientAddr;
    int n,argnumber;
    char str[INET_ADDRSTRLEN];
    signal(SIGINT,sigintHandler);
    args = (char **)malloc(20 * sizeof(char *));
    if (argc != 2)
    {
      fprintf(stderr, "Usage: %s port\n", argv[0]);
      exit(0);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
      printf("[-]Error in connection.\n");
      exit(0);
    }
    printf("[+]Server Socket is created. Server is ready.\n");
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    portno = atoi(argv[1]);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portno);

    if (bind(sockfd,(struct sockaddr *) &serverAddr, sizeof(serverAddr) ) < 0){
      printf("[-]Error in binding.\n");
      exit(0);
    }

    //listen
    if(listen(sockfd, 5) == 0){
		    printf("[+]Listening for connections.\n");
	  }else{
		    printf("[-]Error in binding. Too many users.\n");
	  }


    while (1)
    {
        addr_size = sizeof(clientAddr);
        newSocket = accept(sockfd, (struct sockaddr *) &clientAddr, &addr_size);
        if (newSocket < 0){
            close(newSocket);
            printf("[-]Something went wrong while starting the server.\n");
            exit(0);
        }
        pid_t client_pid;
        //fork
        if( (  client_pid=fork() ) ==-1 ) { //check for error
            perror("fork");
            exit(1);
        }

        if(client_pid==0){ //main child process serving the client
            close(sockfd);
            char *hist_dir;
            int pid = getpid();
            char mypid[10];
            sprintf(mypid, "%d", pid);
            strcat(mypid, ".hist");
            hist_dir = getcwd(hist_dir,sizeof(char)*300);
            strcat(hist_dir, "/");
            strcat(hist_dir, mypid);
            FILE *history=fopen(hist_dir,"w+");
            int tmp=dup(1);
            dup2(fileno(history),1);
            printf("--Command History--\n");
            dup2(tmp,1);
            //Every time starting with a different random number
            //Not military grade but secure enough for a simple server app
            srand(time(0));
            //Validation Process
            int pin=1000+(rand()%9000);
            char password[5];
            sprintf(password,"%d",pin);
            printf("[+]Connection attempt from client.\n");
            printf("[+]While connecting enter the password: %d\n",pin );
            bzero(buffer, BUFF_SIZE);
            n = read(newSocket, buffer, BUFF_SIZE-1);
            if (n < 0){
              printf("[-]Could not receive data from client.\n");
              close(newSocket);
              exit(0);
            }
            if(strstr(buffer,password)==NULL){
                printf("[-]Denied server access to the client[Wrong Password].\n" );
                char* access="denied\n";
                write(newSocket,access,sizeof(access));
                close(newSocket);
                exit(EXIT_FAILURE);
            }else{
              char* access="granted\n";
              printf("[+]Granted access to client.\n" );
              write(newSocket,access,sizeof(access));
            }
            bzero(buffer, BUFF_SIZE);

            if (inet_ntop(AF_INET, &clientAddr.sin_addr, str,INET_ADDRSTRLEN) == NULL) {
                fprintf(stderr, "Could not convert byte to address\n");
            }

            fprintf(stdout, "\n[+]The client address is :%s\n", str);
            //fflush(stdout);
            int histcount=0;
            while (1)
            {
                FILE *fd=fopen("tmp","w+");

                bzero(buffer, BUFF_SIZE);
                fflush(stdout);
                n = recv(newSocket, buffer, BUFF_SIZE-1,MSG_WAITALL);
                if (n < 0){
                    printf("[-]Could not receive data from client.\n" );
                    break;
                }
                if (n == 0){
                    printf("[-]Client disconnected from the server.\n");
                    break;
                }
                int tmp=dup(1);
                dup2(fileno(history),1);
                printf("%d) %s\n",++histcount,buffer);
                dup2(tmp,1);
                printf("\nExecuting command: %s\n",buffer);

                if(strstr(buffer, "exit") != NULL) { //check for exit
                    n = send(newSocket, "exit\n", 4,0);
                    if (n < 0){
                        perror("[-]Could not send data to client.\n");
                    }
                    printf("[-]Disconnected from Client:%s:%d.\n", str, portno);
                    break;
                }
                if(strcmp(buffer, "history")==0) { //check for history
                    bzero(buffer,BUFF_SIZE);
                    fseek(history,0,SEEK_SET);
                    n = read(fileno(history), buffer, BUFF_SIZE-1);
                    if (n < 0){
                        perror("[-]Could not receive data from client.\n");
                        continue;
                    }

                    n = send(newSocket,buffer, BUFF_SIZE-1,0);
                    if (n < 0){
                        perror("[-]Could not send data to client.\n");
                        break;
                    }
                    continue;
                }

                argnumber=parser(buffer);
                if(argnumber==0)
                {
                    bzero(buffer,BUFF_SIZE);
                }

                if(strstr(buffer,"cd") != NULL)
                {
                        char result[1000]={0};
                        char cwd[800];
                        if(argnumber == 2){
                            if(chdir(args[1])){
                                char line[]= "[-]cd cannot go to ";
                                snprintf(result, sizeof(result), "%s %s\n", line,args[1]);
                                send(newSocket, result,sizeof(result),0);
                                bzero(buffer, sizeof(buffer));
                            }else{
                            getcwd(cwd, sizeof(cwd));
                            char line[]="[+]Directory changed to ";
                            snprintf(result, sizeof(result), "%s %s\n", line,cwd);
                            send(newSocket, result,sizeof(result),0);
                            }
                       }else if(argnumber == 1)
                       {
                            if(chdir(getenv("HOME")))
                            {
                                char line[]="[-]cd cannot go to Home Directory\n";
                                send(newSocket, line,sizeof(line),0);
                                bzero(buffer, sizeof(buffer));
                            }else{
                            getcwd(cwd, sizeof(cwd));
                            char line[]="[+]Directory changed to ";
                            snprintf(result, sizeof(result), "%s %s\n", line,cwd);
                            send(newSocket, result,sizeof(result),0);
                            }
                        }else{
                            char line[]="[-]SYNTAX ERROR! Correct Usage: cd [dir_name]\n";
                            send(newSocket, line,sizeof(line),0);
                        }
                        bzero(buffer, sizeof(buffer));
                        continue;
                }
                //fork
                int cc_pid,status2;
                if ((cc_pid=fork())== -1)
                {// check for error
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                if (cc_pid!= 0){// The parent(child) process
                    fflush(stdout);
                    if (wait(&status2)== -1)
                    { // Wait for child
                        perror("wait");
                    }
                    int i;
                    for(i=0;i<argnumber;i++)
                    {
                        free(args[i]);
                    }
                    bzero(buffer,BUFF_SIZE);
                    fseek(fd,0,SEEK_SET);
                    n = read(fileno(fd), buffer, BUFF_SIZE-1);
                    if (n < 0){
                        perror("[-]Could not receive data from the temporary file.\n");
                        break;
                    }
                    n = send(newSocket,buffer, BUFF_SIZE-1,0);
                    if (n < 0){
                        perror("[-]Could not send data to client.\n");
                        break;
                    }
                }else{
                    int pipe_index[20],i;
                    for(i=0;i<20;i++){
                      pipe_index[i]=-1;
                    }
                    int pipe_count=pipe_parsr(pipe_index,argnumber);
                    dup2(fileno(fd),1);
                    dup2(fileno(fd),2);
                    close(newSocket);
                    if(pipe_count==1){             //if there is a pipe in given command
                        char **cmd_before_pipe=(char **)malloc(10 * sizeof(char *));
                        char **cmd_after_pipe=(char **)malloc(10 * sizeof(char *));
                        int position=0,position2=0;
                        while(args[position]!=NULL){
                            cmd_before_pipe[position]=(char*)malloc(10*sizeof(char));
                            strcpy(cmd_before_pipe[position],args[position]);
                            position++;
                        }
                        cmd_before_pipe[position++]=NULL;
                        while(args[position]!=NULL){
                            cmd_after_pipe[position2]=(char*)malloc(10*sizeof(char));
                            strcpy(cmd_after_pipe[position2],args[position]);
                            position++;
                            position2++;
                        }
                        cmd_after_pipe[position2++]=NULL;
                        int pipe_fds[2];
                        pipe(pipe_fds);
                        int pid;
                        if ((pid=fork())== -1)
                        {
                            perror("fork");
                            exit(0);
                        }else if(pid==0)
                        {
                          char * output=NULL;
                          if(redirection(cmd_after_pipe,&output)==1)
                          {
                            if(strcmp(output,"error")!=0)
                            {
                              printf("\n[+]File %s was successfully created.\n",output);
                              FILE *redir=fopen(output,"w+");
                              int tmp=dup(1);
                              dup2(fileno(redir),1);
                              dup2(pipe_fds[READ], 0);
                              close(pipe_fds[WRITE]);
                              execvp(cmd_after_pipe[0], cmd_after_pipe);
                              perror(cmd_after_pipe[0]);
                              dup2(tmp,1);
                            }else{
                              printf("\n[-]Could not execute. Incorrect usage of '>' in command.\n");
                            }
                          }else{
                            dup2(pipe_fds[READ], 0);
                            close(pipe_fds[WRITE]);
                            execvp(cmd_after_pipe[0], cmd_after_pipe);
                            perror(cmd_after_pipe[0]);
                          }
                        }else{
                          char * output;
                          if(redirection(cmd_before_pipe,&output)==1)
                          {
                            if(strcmp(output,"error")!=0)
                            {
                              printf("\n[+]File %s was successfully created.\n\n",output);
                              FILE *redir=fopen(output,"w+");
                              int tmp=dup(1);
                              dup2(fileno(redir),1);
                              execvp(cmd_before_pipe[0], cmd_before_pipe);
                              perror(cmd_before_pipe[0]);
                              dup2(tmp,1);
                            }else{
                              printf("\n[-]Could not execute. Incorrect usage of '>' in command.\n");
                            }
                          }else{
                            dup2(pipe_fds[WRITE], 1);
                            close(pipe_fds[READ]);
                            execvp(cmd_before_pipe[0], cmd_before_pipe);
                            perror(cmd_before_pipe[0]);
                          }
                        }
                        exit(EXIT_SUCCESS);
                    }else if(pipe_count>1){
                      printf("[-]For multiple pipes we recommend using bash.\n");
                      exit(EXIT_FAILURE);
                    }
                    char * output;
                    if(redirection(args,&output)==1)
                    {
                      if(strcmp(output,"error")!=0)
                      {
                        printf("\n[+]File %s was successfully created.\n",output);
                        FILE *redir=fopen(output,"w+");
                        int tmp=dup(1);
                        dup2(fileno(redir),1);
                        execvp(args[0], args);
                        perror("execvp");
                        dup2(tmp,1);
                      }else{
                        printf("\n[-]Could not execute. Incorrect usage of '>' in command.\n");
                      }
                    }else{
                      execvp(args[0],args); // Executes simple commands
                      perror("execvp");
                      exit(EXIT_SUCCESS);
                    }
                }
              fclose(fd);
            }
            close(newSocket);
            fclose(history);
            exit(EXIT_SUCCESS);
        }else{ //Main server process
            fflush(stdout);
            if (wait(NULL)== -1){ //Waiting for child
                perror("wait");
            }
            printf("\n[+]Listening for connections.\n");
            close(newSocket);
        }
    }
    close(sockfd);
    return 0;
}
