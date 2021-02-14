#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <pthread.h>

#define TOPORT  6666
#define MAXLINE 4096
#define MAXCLIENT 5
int connectfd[MAXCLIENT] = {0};

static void* serverTask (void* arg)
{
    int  listenfd, i, connfd;
    struct sockaddr_in  servaddr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TOPORT);

    i = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0){
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
    // setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));//not set when debug
    printf("====port:%d==waiting for client's request======\n", ntohs(servaddr.sin_port));
    while(1){
        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }
        for(i=0; i<MAXCLIENT; i++)  {
            if(connectfd[i] == 0)   {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100;
                setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                connectfd[i] = connfd;
                printf("add fd:%d at %d \n", connectfd[i], i);
                break;
            }
        }
        if(i>=MAXCLIENT)
            close(connfd);
    }
    close(listenfd);
    return 0;
}

static void* clientTask (void* arg)
{
    struct sockaddr_in  clientin;
    char  buff[4096];
    char str[INET_ADDRSTRLEN]; 
    int  len, i, connfd, iplen;

    while(1){
        for(i=0; i<MAXCLIENT; i++)  {
            connfd = connectfd[i];
            if(connfd)    {
                len = recv(connfd, buff, 200, 0);
                if(len > 0) {
                    buff[len] = '\0';
                    iplen = sizeof(struct sockaddr);
                    if(getpeername(connfd, (struct sockaddr*)&clientin, &iplen) != 0 ){
                        printf("client %d at %d get port error: %s(errno: %d))\n", connfd, i, strerror(errno), errno);
                        continue;
                    }
                    // inet_ntop(AF_INET,&clientin.sin_addr.s_addr, str, sizeof(str));
                    printf("client %d at %d recv msg from %s:%d len:%d : [%s]\n", connfd, i, inet_ntoa(clientin.sin_addr),ntohs(clientin.sin_port), len, buff);
                    printf("recv msg from client %d at %d len:%d : [%s]\n", connfd, i, len, buff);
                    memcpy(buff + len, "server get it!", strlen("server get it!")+1 );
                    len = send(connfd, buff, strlen(buff), 0);
                    if( len < 0){
                        printf("client %d at %d send msg error: %s(errno: %d)\n", connfd, i, strerror(errno), errno);
                        continue;
                    }
                }
                else if(len != -1)   {//not timeout
                    printf("client %d at %d receive error: %s(errno: %d)\n", connfd, i, strerror(errno), errno);
                    close(connfd);
                    connectfd[i] = 0;
                    continue;
                }
                
            }
        }
        // close(connfd);
    }
    for(i=0; i<MAXCLIENT; i++)  {
        if(connectfd[i])    {
            close(connectfd[i]);
            connectfd[i] = 0;
        }
    }
    return 0;
}


/**@fn main
 * @brief main入口函数
 */
int main (int argc, char *argv[])
{
    pthread_t tid1, tid2;
    for(int i=0; i<MAXCLIENT; i++)
        connectfd[i] = 0;
    if (pthread_create(&tid1, NULL, (void*)serverTask, "serverTask") != 0) {
        printf("pthread_create error.");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tid2, NULL, (void*)clientTask, "clientTask") != 0) {
        printf("pthread_create error.");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid2);

    char* rev = NULL;
    pthread_join(tid1, (void *)&rev);
    printf("%s return.\n", rev);
    pthread_cancel(tid2);

    printf("main thread end.\n");
    return 0;
}