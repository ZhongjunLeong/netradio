#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/scoket.h>
#include <proto.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "client.h"
/**
**-M  --mgroup  point to multibroacd
**-P  --port    point recive port
**-p  --player  point player
**-H  --help    show help
*/
struct client_conf_st client_conf = { .rcvport = DEFAULT_RCVPORT,\
.mgroup = DEFAULT_RCVPORT,.player_cmd = DEFAULT_PLAYERCMD};
static void printhelp(void)
{
    printf("-P --port :point recive port\n -M --mgroup point multibroad\n -p --player: point player oder\n -H --help\n");
    
}
static ssize_t writen(int fd,const char *buf,size_t len)
{
   int pos = 0,ret;
    while(len > 0)
    {
        ret = write(fd,buf+pos,len);
        if(ret < 0)
        {
            if(errno == EINTR)
                continue;
            perror("write()");
             return -1;
        }   
        len -= ret;
        pos += ret;
    }
    return pos;
}
int main(int argc,char **argv)
{
    int index =0;
    struct option argarr[] = {{"port",1,NULL,'P'},{"mgroup",1,NULL,'M'},\
    {"player",1,NULL,'p'},{"help",0,NULL,'H'},{NULL,0,NULL,0}};
    int c;
    int sd;
    int len;
    int chosenid;
    struct ip_mreqn mreq;
    struct sockaddr_in laddr;
    int pd[2];
    /*origin*
    **level:default,configuration,environment value,command argument
    **/
    while(1)
    {
        c = getopt_long(argc,argv,"P:M:p:H", argarr,&index);  //get oder
        if(c < 0)
            break;
        else
        {
            switch(c)
            {
                case 'P':
                    client_conf.rcvport = optarg;
                    break;
                case 'M':
                    client_conf.mgroup = optarg;
                    break;
                case 'p':
                    client_conf.player_cmd = optarg;
                    break;
                case 'H':
                    printhelp();
                    exit(0);
                default:
                    abort();
                    break;
            }
        }
    }
    sd = socket(AF_INET,SOCK_DGRAM,0);
    if(sd < 0)
    {
        perror("socket()");
        exit(1);

    }
    inet_pton(AF_INET,client_conf.mgroup,&mreq.imr_multiaddr);
    inet_pton(AF_INET,"0.0.0.0",&mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex("eth0");
    if(setsockopt(sd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);
    if(bind(sd,(void *)&laddr,sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }
    if(pipe(pd) < 0)
    {
        perror("pipe()");
        exit(1);
    }
    pid_t pid;
    pid = fork();
    if(pid < 0)
    {
        perror("fork()");
        exit(1);
    }
    if(pid == 0)
    {
    //call decoder
        close(sd);
        close(pd[1]);
        dup2(pd[0],0);
        if(pd[0] > 0)
            close(pd[0]);
        execl("/bin/sh","sh","-c",client_conf.player_cmd,NULL);
        perror("execl()");
        exit(1);
    }
    //parent
    struct msg_list_st *msg_list;
    struct sockaddr_in serverraddr,raddr;
    socklen_t serverraddr_len,raddr_len;
    msg_list = malloc(MSG_LIST_MAX);
    if(msg_list == NULL)
    {
        perror("malloc()");
        exit(1);
    }
        //get packet from net and send to child process,using pipe
    while(1)
    {
        len = recvfrom(sd,msg_list,MSG_LIST_MAX,0,(void *)&serverraddr,&serverraddr_len);
        if(len < sizeof(struct msg_list_st));
        {
            fprintf(stderr,"message is too small\n");
            continue;
        }
        if(msg_list ->chnid != LISTCHNID)
        {
            fprintf(stderr,"chinid is not match\n");
            continue;
        }
        break;
    }
    //get list
    //choose chanel
    struct msg_listentry_st *pos;
    for(pos = msg_list->entry;(char *)pos < (((char *)msg_list+len) ;pos = (void *)(((char *)pos)+ntohs(pos->len) ))
        printf("channel %d:%s\n",pos->chnid,pos->desc);
    
    free(msg_list);
    int ret;
    while(1)
    {
        ret = scanf("%d",&chosenid);
        if(ret != 1)
            exit(1);
        break;
    }
    //get chanel packet then send to child
    struct msg_channel_st *msg_channel;
    msg_channel = malloc(MSG_CHANNEL_MAX);
    if(msg_channel == NULL)
    {
        perror("malloc()");
        exit(1);
    }
    while(1)
    {
        len = rcvfrom(sd,msg_channel,MSG_CHANNEL_MAX,0,(void *)&raddr,&raddr_len);
        if(raddr.sin_addr.s_addr != serverraddr.sin_addr.s_addr ||raddr.sin_port != severraddr.sin_port)
        {
            fprintf(stderr,"ignore:address not match\n");
            continue;
        }
        if(len < sizeof(struct msg_channel_st))
        {
            fprintf(stderr,"ignore:message too small\n");
            continue;
        }
        if(msg_channel->chnid == chosenid)
        {
            fprintf(stderr,"accepted:msg:%d recieved\n",msg-channel->chnid);
            if(writen(pd[1],msg_channel->data,len-sizeof(chnid_t)) < 0)
            {
                exit(1);
            }
        }

    }
    free(mag_channel);
    close(sd);
    exit(0);

}
