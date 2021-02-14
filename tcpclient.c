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
    int   sockfd, len;
    char  recvline[4096], sendline[4096];
    struct sockaddr_in  servaddr;

    if( argc != 2){
        printf("usage: ./client <ipaddress>\n");
        return -1;
    }

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n",argv[1]);
        return -1;
    }

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    struct timeval timeout = {3,0};
    struct sockaddr_in  client;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    //获取sockfd表示的连接上的本地地址
    if(getsockname(sockfd, (struct sockaddr*)&client, &len) != 0 ){
        printf("client get port error: %s(errno: %d))\n", strerror(errno), errno);
        return -1;
    }
    char str[INET_ADDRSTRLEN]; 
	inet_ntop(AF_INET,&client.sin_addr.s_addr, str, sizeof(str));
    printf("%s:%d send msg to server: \n", str, ntohs(client.sin_port));
    // fgets(sendline, 4096, stdin);
    // char sendline[] = "here is the clien";
    while(1)
    {
        // len = read(sockfd, recvline, 1024);
        len = recv(sockfd, recvline, 1024, 0);
        recvline[len] = 0;
        if(len <= 0)    {
            printf("send msg:");
            scanf("%s", sendline);
            // len = write(sockfd, sendline, sizeof(sendline)); 
            len = send(sockfd, sendline, strlen(sendline), 0);
            if( len < 0){
                printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
            }
        }
        else    {
            printf("rec msg len %d [%s]:", len, recvline);
        }
        // sleep(1);
    }
    close(sockfd);
    return 0;
}