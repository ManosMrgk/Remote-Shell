#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define BUFF_SIZE 10000


void sigintHandler(int sig_num)
{
    printf("\n[-]Exiting..\n");
    printf("\n[-]Client terminated. \n");
    exit(sig_num);
}

int main(int argc, char *argv[])
{
    int serverSocket, portno, n;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    //char str[INET_ADDRSTRLEN];
    char buffer[BUFF_SIZE];
    signal(SIGINT,sigintHandler);
    if(argc != 3)
    {
       fprintf(stderr, "Usage: %s ip-address port\n", argv[0]);
       exit(0);
    }
    if(strcmp(argv[1],"localhost")==0)
    {
        strcpy(argv[1],"127.0.0.1");
    }

    portno = atoi(argv[2]);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        printf("[-]Error in opening the socket.\n");
        exit(0);
    }
    printf("[+]Client Socket is created.\n");
    server = gethostbyname(argv[1]);

    if (server == NULL)
    {
        fprintf(stderr, "[-]Could determine the host's name.\n");
        exit(0);
    }
    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portno);
    memmove((char *)server->h_addr,(char *)&serverAddr.sin_addr.s_addr,server->h_length);
    if(connect(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Connecting to Server..\n");
    printf("[+]Give %s's 4 digit password(Printed on server's terminal console):", argv[1]);
    char password[5];
    fgets(password,55,stdin);

    //Send given password back to the server for authentication

    n=write(serverSocket,password,sizeof(password));
    if(n < 0)
    {
      printf("[-]Could not communicate with the server.\n");
      close(serverSocket);
      exit(0);
    }
    bzero(buffer, BUFF_SIZE);
    //Receive authentication result [granted or denied]
    n=read(serverSocket,buffer,BUFF_SIZE-1);
    if(n < 0)
    {
      printf("[-]Could not communicate with the server.\n");
      close(serverSocket);
      exit(0);
    }

    if(strcmp(buffer,"granted\n")==0)
    {
        printf("[+]Granted access to the server.\n" );
    }else if(strcmp(buffer,"denied\n")==0){
        printf("[-]Access Denied[Wrong Password].\n");
        close(serverSocket);
        exit(1);
    }else{
      printf("[-]Lost connection to the server.\n");
      close(serverSocket);
      exit(0);
    }
    bzero(buffer, BUFF_SIZE);

    while (1)
    {
        printf("%s_> ",argv[1]);
        fflush(stdout);

        bzero(buffer, BUFF_SIZE);
        fflush(stdin);
        fgets(buffer,BUFF_SIZE-1,stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        n = send(serverSocket, buffer, BUFF_SIZE-1,0);
        if (n < 0)
        {
           printf("[-]Could not send data to the server.\n");
           close(serverSocket);
           exit(0);
        }

        bzero(buffer, BUFF_SIZE);
        n = recv(serverSocket, buffer, BUFF_SIZE-1,0);
        if (n < 0)
        {
           printf("[-]Could not receive data from the server.\n");
           close(serverSocket);
           exit(0);
        }

        if(strstr(buffer, "exit") != NULL){
            printf("[-]Disconnected from the server.\n");
            break;
        }
        printf("\n%s\n",buffer);
        fflush(stdout);

    }

    //free(buffer);
    close(serverSocket);
    return 0;
}
