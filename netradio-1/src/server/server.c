#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "server_conf.h"
#include <proto.h>
/**
*-M   multibrocast
*-P   accept port
*-F   ahead operating
*-H   show help
*-D   media lib ocasstion
*-I   internet facility 
*/
struct server_conf_st server_conf = {.rcvport = DEFAULT_RCVPORT,\
.mgroup = DEFAULT_MGROUP,.media_dir = DEFAULT_MEDIADIR,.runmode = RUN_DAEMON,.ifname = DEFAULT_IF};
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
static void daemon_exit(int s)
{
   
   
   closelog();
    exit(0);

}
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
    else
    {
        dup2(fd,0);
        dup2(fd,1);
        dup2(fd,2);
        if(fd > 2)
            close(fd);
    }
    setsid();
    chdir("/");
    umask(0);
    return 0;

}
static socket_init(void)
{
    int serversd;
    struct ip_mreqn mreq;
    serversd = socket(AF_INET,SOCK_DGRAM,0);
    if(serversd < 0)
    {
        syslog(LOG_ERR,"socket();%s",strerror(errno));
        exit(1);
    }
    inet_pton(AF_INET,server_conf.mgroup,&mreq.imr_multiaddr);
    inet_pton(AF_INET,"0.0.0.0",&mreqi.imr_address);
    mreq.imr_ifindex = if_nametoindex(server_conf.ifname);
    if(setsockopt(serversd,IPPROTO_IP,IP_MULTICAST,&mreq,sizeof(mreq)) < 0)
    {
        syslog(LOG_ERR,"setsockopt(IP_MULTICAST_IF);%s",strerror(errno));
        exit(1);
    }

}
int main(int argc,char **argv)
{
    int c;
    struct aigaction sa;
    sa.sa_handler = daemon_exit;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask,SIGINT);
 
    sigaddset(&sa.sa_mask,SIGQUIT);
    sigaddset(&sa.sa_mask,SIGTERM);
    sigaction(SIGTERM,&sa,NULL);
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGQUIT,&sa,NULL);
    openlog("netradio",LOG_PID|LOG_PERROR,LOG_DAEMON);
    /***analysic oder***/
    while(1)
    {
        c = getopt(argc,argv,"M:P:FD:I:H");
        if(c < 0)
            break;
        else
        {
            switch(c)
            {
                case 'M':
                    server_conf.mgroup = optarg;
                    break;
                case 'P':
                    server_conf.rcvport = optarg;
                    break;
                case 'F':
                    server_conf.runmode = RUN_FOREGROUD;
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
                    break();
            }
        }
    }
    /***daemon process***/
    if(server_conf.runmode == RUN_DAEMON)
    {
        if(daemonize() != 0)
            exit(1);
    }

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


    /***socket init**/
    socket_init()

    /**get channel message----create list pthread*/
    struct mlist_listentry_st *listl;
    int list_size;
    int err;
   err =  mlib_getchnlist(&list,&list_size);
   if()
   {

   }
    thr_list_create(list,list_size);
    /*if error*/
    /*create channel pthread*/
    
    for( i = 0; i < list_size;i++)
    {
        thr_channel_create(list+i);
        /*if error*/
    }
    syslog(LOG_DEBUG,"%d channel threads create ",i);
    while(1)
    pause();

    




    exit(0);
}
