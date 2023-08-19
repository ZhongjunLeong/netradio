#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <proto.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include "client.h"

//客户端的接收ip及端口，播放器地址
struct client_conf_st client_conf = { .rcvport = DEFAULT_RCVPORT, .mgroup = DEFAULT_MGROUP, .player_cmd = DEFAULT_PLAYERCMD};

//命令行参数打印
static void printhelp(void)
{
    printf("-P --port :point recive port\n -M --mgroup point multibroad\n -p --player: point player oder\n -H --help\n");
    
}

/*父进程往管道写收到的频道内容*/
static ssize_t writen(int fd, const char *buf, size_t len)
{
   int pos = 0,ret;

    while(len > 0)
    {
        ret = write(fd, buf+pos, len);
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
    int index = 0;

    struct option argarr[] = {{"port", 1, NULL, 'P'}, {"mgroup", 1, NULL, 'M'}, \
    {"player", 1, NULL, 'p'}, {"help", 0, NULL, 'H'}, {NULL, 0, NULL, 0}};//命令行参数解析

    int c; 
    int sd;
    int len;
    int val;
    int chosenid;
    struct ip_mreqn mreq;
    struct sockaddr_in laddr;
    int pd[2];
    pid_t pid;

    struct msg_list_st *msg_list;//节目单包
    struct sockaddr_in serverraddr,raddr;
    socklen_t serverraddr_len,raddr_len;

    struct msg_listentry_st *pos;
    int ret = 0;
    struct msg_channel_st *msg_channel;

    /**解析由命令行指定选项：段格式   长格式
    **-M  --mgroup  指定多播组ip
    **-P  --port    指定接收端口
    **-p  --player  指定播放器
    **-H  --help    show help
    */
    while(1)
    {/*初始化*
        **级别:default,configuration,environment value,command argument：带选项的
        **/
        c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);  //读取命令行
        if(c < 0)
            break;
        else
        {
            switch(c)
            {
                case 'P':
                    client_conf.rcvport = optarg;   //getopt_long函数中定义的，如果带参的，在识别完字符串，optarg自动指向串后的参数
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
                    abort();//自己发送结束信号
                    break;
            }
        }
    }

    /*建立网络连接 socket UDP协议*/
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0)
    {
        perror("socket()");
        exit(1);

    }

    /*建立端口复用监听*/
    /*加入多播组初始化*/
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);   //将本机的地址放到imr_address中，再转成ipv4格式
    mreq.imr_ifindex = if_nametoindex("eth0");  //当前用的网络设备

    if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    // 允许组播数据包本地回环
    val = 1;    //布尔类型 真
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &(val), sizeof(val)) < 0)
    {
       perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    /*给客户端命名套接字*/
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    if(bind(sd, (void *)&laddr, sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }
    
    /*建立管道*/
    if(pipe(pd) < 0)    //一端为读，一端为写
    {
        perror("pipe()");
        exit(1);
    }
 
    /*创建父子进程*/
    pid = fork();
    if(pid < 0)
    {
        perror("fork()");
        exit(1);
    }
    /*子进程：调用解码器*/
    if(pid == 0)
    {
    //call decoder
        close(sd);  //子进程不需要使用socket，关掉
        close(pd[1]);   //关闭写端
        dup2(pd[0], 0); //把管道的读端作为0号描述符的读端（0：标准输入），即读取的数据重定向到标准输入
        if(pd[0] > 0)
            close(pd[0]);
        execl("/bin/sh","sh","-c",client_conf.player_cmd,NULL); //通过sh解释器执行play_cmd的路径，调用解码器
        perror("execl()");
        exit(1);
    }

    //parent:从网络上收包，通过管道发送给子进程 
   
    msg_list = malloc(MSG_LIST_MAX);
    if(msg_list == NULL)
    {
        perror("malloc()");
        exit(1);
    }
    /*收节目单*/
    while(1)
    {
        len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (void *)&serverraddr, &serverraddr_len);
        if(len < sizeof(struct msg_list_st))  //收包小于该结构体的大小
        {
            fprintf(stderr,"message is too small\n");
            continue;
        }

        if(msg_list ->chnid != LISTCHNID)   //表示不是节目单
        {
            fprintf(stderr,"chinid is not match\n");
            continue;
        }
        break;
    }

    
    //获得节目单，打印节目单
    for(pos = msg_list->entry; (char *)pos < ((char *)msg_list+len); pos = (void *)(((char *)pos)+ntohs(pos->len)))
    {
        printf("channel %d:%s\n", pos->chnid, pos->desc);
    }    
   
    free(msg_list);
   
   /*选择节目单*/
   printf("please enter channerl:\n");
    while(ret < 1)
    {
        ret = scanf("%d", &chosenid);
        if(ret != 1)
            exit(1);
        break;
    }

    //收频道包，发送给子进程
    msg_channel = malloc(MSG_CHANNEL_MAX);
    if(msg_channel == NULL)
    {
        perror("malloc()");
        exit(1);
    }
    raddr_len = sizeof(raddr);
    while(1)
    {
        len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&raddr, &raddr_len);
        if(raddr.sin_addr.s_addr != serverraddr.sin_addr.s_addr ||raddr.sin_port != serverraddr.sin_port) //验证当前收包的地址/端口与前面的收包是否一个地址
        { 
            fprintf(stderr, "ignore:address not match\n");
            continue;
        }

        if(len < sizeof(struct msg_channel_st))
        {
            fprintf(stderr, "ignore:message too small\n");
            continue;
        }

        if(msg_channel->chnid == chosenid)
        {
            fprintf(stderr, "accepted:msg:%d recieved\n", msg_channel->chnid);

            //父进程往管道中写收到的包内容给子进程
            if(writen(pd[1], (void *)msg_channel->data, len-sizeof(chnid_t)) < 0)
            {
                exit(1);
            }
        }

    }
    
    /*放在信号中进行杀死*/
    free(msg_channel);
    close(sd);
    exit(0);

}
