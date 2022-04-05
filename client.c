#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <poll.h>
#include <netinet/in.h>
#include <fcntl.h>
#define BUF_SIZE 64
int main(int argc,char* argv[])
{
    if(argc <= 2)
    {
        printf("The argc number not right,%s\n",argv[0]);
        return 0;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in address;
    memset(&address,0,sizeof(address));
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    int ret = connect(sock,(struct sockaddr*)&address,sizeof(address));
    if(ret < 0)
    {
        printf("connection failed!\n");
        close(sock);
        return 1;
    }
    pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sock;
    fds[1].events = POLLIN|POLLRDHUP;
    fds[1].revents = 0;
    char read_buf[BUF_SIZE];
    int pipefd[2];
    ret = pipe(pipefd);
    assert(ret != -1);
    while(1)
    {
        ret = poll(fds,2,-1);
        if(ret < 0)
        {
            printf("poll failed\n");
            break;
        }
        if(fds[1].revents&POLLRDHUP)
        {
            printf("server close the connection!\n");
            break;
        }
        else if(fds[1].revents&POLLIN)
        {
            memset(read_buf,'\0',BUF_SIZE);
            recv(fds[1].fd,read_buf,BUF_SIZE-1,0);
            printf("%s",read_buf);
        }
        if(fds[0].revents&POLLIN)
        {
            ret = splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
            ret = splice(pipefd[0],NULL,sock,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(sock);
    return 0;
}