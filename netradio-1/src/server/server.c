#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "server_conf.h"
#include "medialib.h"
#include "thr_list.h"
#include "thr_channel.h"
#include <proto.h>
/**
*-M   指定的多播组
*-P   指定接收端口
*-F   前台运行
*-H   show help
*-D   指定媒体库位置
*-I   指定网络设备 
*/
//默认
struct server_conf_st server_conf = {.rcvport = DEFAULT_RCVPORT,\
.mgroup = DEFAULT_MGROUP, .media_dir = DEFAULT_MEDIADIR, .runmode = RUN_DAEMON, .ifname = DEFAULT_IF};
int serversd;
struct sockaddr_in sndaddr;//待发送端地址
static struct mlib_listentry_st *list;

static void printfhelp(void)
{

    printf("-M   multibrocast\n");
    printf("-P   accept port\n");
    printf("-F   ahead operating\n");
    printf("-H   show help\n");
    printf("-D   media lib ocasstion\n");
    printf("-I   internet facility\n");
    exit(0);


}

/*接到信号时执行：进程结束*/
static void daemon_exit(int s)
{
   thr_list_destory();
   thr_channel_destroyall();
   mlib_freechnlist(list);

   syslog(LOG_WARNING, "signal -%d caught, exit now.",s );
   closelog();
    exit(0);

}
/*守护进程实现*/
static int  daemonize(void)
{
    pid_t pid;
    pid = fork();
    int fd;
    if(pid < 0)
    {
        //perror("fork()");
        syslog(LOG_ERR,"fork():%s",strerror(errno));
        return -1;
    }

    if(pid > 0) //parent
        exit(0);
    
    fd = open("/dev/null",O_RDWR);
    if(fd < 0)
    {
     //   perror("open()");
        syslog(LOG_WARNING,"open():%s",strerror(errno));
        return -2;
    }
    else//将标准输入出重定向到一个空文件
    {
        dup2(fd,0);
        dup2(fd,1);
        dup2(fd,2);
        if(fd > 2)
            close(fd);
    }
    setsid();   //创建一个会话，脱离控制终端
    chdir("/"); //把守护进程指定到一个合适的位置上
    umask(0);
    return 0;

}

static int socket_init(void)
{
    
    struct ip_mreqn mreq;
    /*创建套接字*/
    serversd = socket(AF_INET, SOCK_DGRAM, 0);
    if(serversd < 0)
    {
        syslog(LOG_ERR, "socket();%s", strerror(errno));
        exit(1);
    }

    /*设置属性*/
    inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);//多播地址
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);  //当前ip
    mreq.imr_ifindex = if_nametoindex(server_conf.ifname);
    /*建立多播组*/
    if(setsockopt(serversd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0)
    {
        syslog(LOG_ERR, "setsockopt(IP_MULTICAST_IF);%s", strerror(errno));
        exit(1);
    }
    /*发送端的套接字命名*/
    sndaddr.sin_family = AF_INET;
    sndaddr.sin_port = htons(atoi(server_conf.rcvport));
    inet_pton(AF_INET, server_conf.mgroup, &sndaddr.sin_addr.s_addr);//发送端地址可以是默认也可以是命令行指定

    return 0;

}

int main(int argc,char **argv)
{
    int c;

    
    int list_size;
    int err;
    int i;
    struct sigaction sa;
    sa.sa_handler = daemon_exit;    //信号终止守护进行
    
    /*信号设置*/
    sigemptyset(&sa.sa_mask);//信号集，对多个信号操作
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    //打开系统日志
    openlog("netradio",LOG_PID|LOG_PERROR,LOG_DAEMON);

    /***analysic oder***/
    while(1)
    {
        /*命令行分析*/
        c = getopt(argc, argv, "M:P:FD:I:H");
        if(c < 0)
            break;
        else
        {
            switch(c)
            {
                case 'M':
                    server_conf.mgroup = optarg;    //由命令行重新指定
                    break;
                case 'P':
                    server_conf.rcvport = optarg;
                    break;
                case 'F':
                    server_conf.runmode = RUN_FOREGROUND;
                    break;
                case 'D':
                    server_conf.media_dir = optarg;
                    break;
                case 'I':
                    server_conf.ifname = optarg;
                    break;
                case 'H':
                    printfhelp();
                    break;
                default:
                    abort();
                    break;
            }
        }
    }

    /***守护进程***/
    if(server_conf.runmode == RUN_DAEMON)
    {
        if(daemonize() != 0)
            exit(1);
    }
    //前台进行
    else if(server_conf.runmode == RUN_FOREGROUND)
    {
        /*do nothing*/

    }
    else 
    {
        //fprintf(stderr,"EINVAL\n");
        syslog(LOG_ERR,"EINVAL server_conf.runmode");
        exit(1);
    }


    /***socket初始化**/
    socket_init();

    /**获取频道信息----创建节目线程*/

   err =  mlib_getchnlist(&list, &list_size);
   if(err )
   {
        syslog(LOG_ERR,"milib_getchnlist fail %s", strerror(errno));
        exit(1);
   }
   
   /*获取节目单--线程发送节目单*/
    if(thr_list_create(list, list_size) != 0)
    {
        syslog(LOG_ERR,"节目单创建失败\n");
    }
    /*if error*/

    /*创建频道线程*/
    
    for( i = 0; i < list_size; i++) //32位机器4G，一个栈10M，可创建大概300个线程；若64位机器，进程虚拟空间128T，可创建很多线程，不能用当前栈个数限制。线程标识花完了也不会到达上限
    {//所以同时创建200个线程没有压力
   // printf("进创建频道线程\n");
        err = thr_channel_create(list+i);//传地址值
        if(err)
        {
           
            fprintf(stderr, "thr_channel_create():%s\n", strerror(errno));
            exit(1);
        }
        /*if error*/
    }

    syslog(LOG_DEBUG,"%d channel threads create ",i);
    while(1)
    pause();





    exit(0);
}
