#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

#define TOPORT  6666
#define MAXLINE 4096

int main(int argc, char** argv){
    int   sockfd, len, sin_len;
    char  buf[4096];
    struct sockaddr_in  tager;
    char str[INET_ADDRSTRLEN]; 

    if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return -1;
    }

    struct timeval timeout = {3,0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    
    sin_len = sizeof(tager);
    memset(&tager, 0, sin_len);
    tager.sin_family = AF_INET;
    tager.sin_port = htons(TOPORT);

    if ( bind(sockfd, (const struct sockaddr *)&tager, sin_len) < 0 ){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return -1;
    }
    
    while(1)
    {
        len = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *)&tager, &sin_len);
        buf[len] = 0;
        if(len <= 0)    {
            // printf("send msg:");
            // scanf("%s", buf);
            memcpy(buf, "here receive timeout", strlen("here receive timeout") +1 );
            len = sendto(sockfd, buf, strlen(buf), MSG_CONFIRM,(struct sockaddr *)&tager,sin_len);
            if( len < 0){
                printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
            }
        }
        else    {
            inet_ntop(AF_INET,&tager.sin_addr.s_addr, str, sizeof(str));
            printf("rec msg from %s:%d len %d [%s]\n", str, ntohs(tager.sin_port), len, buf);
        }
        // sleep(1);
    }
    close(sockfd);
    return 0;
}