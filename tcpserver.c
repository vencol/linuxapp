#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

#define MAXLINE 4096

int main(int argc, char** argv){
    int  listenfd, connfd;
    struct sockaddr_in  servaddr, clientin;
    char  buff[4096];
    char str[INET_ADDRSTRLEN]; 
    int  len;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);

    len = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &len, sizeof(len)) < 0){
        printf("bind addr reuse error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    if( listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    struct timeval timeout = {5,0};//5s timeout close client
    setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	inet_ntop(AF_INET,&servaddr.sin_addr.s_addr, str, sizeof(str));
    printf("====%s:%d==waiting for client's request======\n", str, ntohs(servaddr.sin_port));
    while(1){
        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
        if(getsockname(connfd, (struct sockaddr*)&clientin, NULL) != 0 ){
            printf("client get port error: %s(errno: %d))\n", strerror(errno), errno);
        }
	    inet_ntop(AF_INET,&clientin.sin_addr.s_addr, str, sizeof(str));
        printf("waiting msg from %s:%d client :", str, ntohs(clientin.sin_port));
        len = recv(connfd, buff, 200, 0);
        buff[len] = '\0';
        printf("recv msg from %s:%d client len:%d : [%s]\n", str, ntohs(clientin.sin_port), len, buff);
        memcpy(buff + len, " server get it!", strlen(" server get it!")+1);
        // snprintf(buff, 200, "server get msg [%s]!", buff);
        len = send(connfd, buff, strlen(buff), 0);
        if( len < 0){
            printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        }
        close(connfd);
    }
    close(listenfd);
    return 0;
}