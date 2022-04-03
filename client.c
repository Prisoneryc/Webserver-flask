#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
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
    assert(ret != -1);
    char buf[128] = "Hello my server!\n";
    write(sock,buf,sizeof(buf)-1);
    close(sock);
}