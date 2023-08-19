#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__
#define DEFAULT_MEDIADIR    "/var/media"   //默认媒体库位置
#define DEFAULT_IF          "eth0"      //默认网卡

enum
{
    RUN_DAEMON = 1,
    RUN_FOREGROUND
};

struct server_conf_st
{
    char *rcvport;  //默认接收端口
    char *mgroup;   //默认多播组号
    char *media_dir;    //媒体库位置
    char runmode;  //运行模式，前台/后台运行
    char *ifname;       //网卡
};

extern struct server_conf_st server_conf;
extern int serversd;
extern struct sockaddr_in sndaddr;







#endif
