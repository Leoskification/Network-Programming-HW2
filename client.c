#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_BUFFER 512

static int socketFd;

void interruptHandler(int sig_unused) {

    char msg[] = "/exit";
    if(write(socketFd, msg, strlen(msg) + 1) == -1) {
        printf("Write failed when exiting room.\n");
    }

    close(socketFd);
    exit(1);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in servaddr;
    struct hostent *host;

    if(argc != 2) {
        printf("./client [username]\n");
        exit(1);
    }

    printf("/a to show all Online Users\n/w to whisper(private message) to a User(ex. /w User message)\n/f to send file to a User(ex. /f User file)\n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8080);

    if((socketFd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        perror("Socket error\n");
        exit(1);
    }

    if(connect(socketFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0) {
        perror("Connect error\n");
        exit(1);
    }

    //Set a handler for the interrupt signal
    signal(SIGINT, interruptHandler);

    //Send user name
    write(socketFd, argv[1], strlen(argv[1]) + 1);

    fd_set clientFds;
    char sendMsg[MAX_BUFFER], recvMsg[MAX_BUFFER];
    char *ptr, tmpBuffer[MAX_BUFFER];

    while(1) {

        FD_ZERO(&clientFds);
        FD_SET(socketFd, &clientFds);
        FD_SET(0, &clientFds);

        if(select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) { //wait for an available fd
            if(FD_ISSET(socketFd, &clientFds)) { //receive data from server
                read(socketFd, recvMsg, MAX_BUFFER - 1);
                    printf("%s\n", recvMsg);
            }

            if(FD_ISSET(0, &clientFds)) {
                fgets(sendMsg, MAX_BUFFER - 1, stdin);
                sendMsg[ strlen(sendMsg)-1 ] = '\0';

                strcpy(tmpBuffer, sendMsg);
                ptr = strtok(tmpBuffer, " ");

                if(strcmp(sendMsg, "/exit") == 0) {
                    interruptHandler(-1); //Disconnect the client
                    exit(1);
                }
                else if(strcmp(ptr, "/f") == 0) { //not using continue might cause error
                    if((ptr = strtok(NULL, " ")) == NULL) {  //ptr is user name
                        printf("/f [user name] [file name]\n");
                    }
                    if((ptr = strtok(NULL, " ")) == NULL) {  //ptr is file name
                        printf("/f [user name] [file name]\n");
                    }

                    FILE *fp = fopen(ptr, "rb");
                    if(fp == NULL) {
                        printf("File doesn't exist.\n");
                    }

                    //Send request
                    if(write(socketFd, sendMsg, strlen(sendMsg) + 1) == -1) {
                        printf("Write failed when sending message.");
                    }
                    read(socketFd, recvMsg, MAX_BUFFER-1);

                    if(strcmp(recvMsg, "Valid.") == 0) {
                        //Connection for upload
                        struct sockaddr_in servFileAddr;
                        bzero(&servFileAddr, sizeof(servFileAddr));
                        servFileAddr.sin_family = AF_INET;
                        servFileAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                        servFileAddr.sin_port = htons(8888);

                        int socketFileFd = socket(AF_INET, SOCK_STREAM, 0);
                        connect(socketFileFd, (struct sockaddr *) &servFileAddr, sizeof(struct sockaddr));

                        int nbytes;
                        while(!feof(fp)) {
                            nbytes = fread(tmpBuffer, sizeof(char), MAX_BUFFER, fp);
                            write(socketFileFd, tmpBuffer, nbytes);
                        }

                        fclose(fp);
                        close(socketFileFd);
                        printf("Upload success.\n");
                    }
                    else {
                        printf("Wrong user.\n");
                    }
                }
                else if(strcmp(sendMsg, "/y") == 0) {
                    if(write(socketFd, sendMsg, strlen(sendMsg) + 1) == -1) {
                        printf("Write failed when sending message.");
                    }

                    read(socketFd, recvMsg, MAX_BUFFER);

                    char fileName[MAX_BUFFER];
                    strcpy(fileName, "from_");
                    strcat(fileName, recvMsg);

                    //Connection for download
                    struct sockaddr_in servFileAddr;

                    bzero(&servFileAddr, sizeof(servFileAddr));
                    servFileAddr.sin_family = AF_INET;
                    servFileAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                    servFileAddr.sin_port = htons(8800);

                    int socketFileFd = socket(AF_INET, SOCK_STREAM, 0);
                    connect(socketFileFd, (struct sockaddr *) &servFileAddr, sizeof(struct sockaddr));

                    FILE *fp = fopen(fileName, "wb");
                    int nbytes;

                    while(nbytes = read(socketFileFd, recvMsg, MAX_BUFFER)) {
                        fwrite(recvMsg, sizeof(char), nbytes, fp);
                    }

                    close(socketFileFd);
                }
                else {
                    if(write(socketFd, sendMsg, strlen(sendMsg) + 1) == -1) {
                        printf("Write failed when sending message.");
                    }
                }
            }
        }
        else {
            printf("select() failed\n");
            exit(1);
        }
    }
}
