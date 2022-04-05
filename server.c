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
#include <fcntl.h>
#include <netinet/in.h>
#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535
struct client_data
{
    sockaddr_in address;
    char* write_buf;
    char buf[BUFFER_SIZE];
};
int setnonblocking(int fd)
{
    int oldoption = fcntl(fd,F_GETFL);
    int newoption = oldoption|O_NONBLOCK;
    fcntl(fd,F_SETFL,newoption);
    return newoption;
}
int main(int argc,char *argv[])
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
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_family = AF_INET;
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);
    ret = listen(sock,5);
    assert(ret != -1);
    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT+1];
    int user_count = 0;
    for(int i = 1;i <= USER_LIMIT;i++)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = sock;
    fds[0].events = POLLIN|POLLERR;
    fds[0].revents = 0;
    while(1)
    {
        ret = poll(fds,user_count+1,-1);
        if(ret < 0)
        {
            printf("poll failure\n");
            break;
        }
        for(int i = 0;i < user_count+1;i++)
        {

            if((fds[i].fd == sock) && fds[i].revents&POLLIN)
            {
                struct sockaddr_in client_address;
                socklen_t clientlen = sizeof(client_address);
                int connfd = accept(sock,(struct sockaddr*)&client_address,&clientlen);
                if(connfd < 0)
                {
                    printf("errno is %d\n",errno);
                    continue;
                }
                if(user_count >= USER_LIMIT)
                {
                    const char* info = "too many users\n";
                    printf("%s\n",info);
                    send(connfd,info,strlen(info),0);
                    continue;
                }
                user_count++;
                users[connfd].address = client_address;
                setnonblocking(connfd);
                fds[user_count].fd = connfd;
                fds[user_count].events = POLLIN|POLLRDHUP|POLLERR;
                fds[user_count].revents = 0;
                printf("comes a new user,now have %d users\n",user_count);

            }
            else if(fds[i].revents & POLLERR)
            {
                printf("get an error from%d\n",fds[i].fd);
                char errors[100];
                memset(errors,'\0',sizeof(errors));
                socklen_t lenght = sizeof(errors);
                continue;
            }
            else if(fds[i].revents & POLLRDHUP)// client close the connection
            {
                users[fds[i].fd] = users[fds[user_count].fd];
                close(fds[i].fd);
                fds[i] = fds[user_count];
                i--;
                user_count--;
                printf("a client left\n");
            }
            else if(fds[i].revents & POLLIN)
            {
                int connfd = fds[i].fd;
                memset(users[connfd].buf,'\0',BUFFER_SIZE);
                ret = recv(connfd,users[connfd].buf,BUFFER_SIZE-1,0);
                printf("get %d bytes of client data %sfrom %d\n",ret,users[connfd].buf,connfd);
                
                for(int j = 1;j <= user_count;j++)
                {
                    if(fds[j].fd = connfd)
                        continue;
                    fds[j].events |= ~POLLIN;
                    fds[j].events |= POLLOUT;
                    users[fds[j].fd].write_buf = users[connfd].buf;
                }
            }
            else if(fds[i].revents & POLLOUT)
            {
                int connfd = fds[i].fd;
                if(!users[connfd].write_buf)
                    continue;
                ret = send(connfd,users[connfd].write_buf,strlen(users[connfd].write_buf),0);
                users[connfd].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }
    delete []users;
    close(sock);
    return 0;
}