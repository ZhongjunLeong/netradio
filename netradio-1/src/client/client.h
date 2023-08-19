#ifndef CLIENT_H__
#define CLIENT_H__
#define DEFAULT_PLAYERCMD "/usr/bin/mpg123 - > /dev/null"//播放器地址，- 表示播放器播报标准输入来的内容
struct client_conf_st   //用户通过命令行指定的内容
{
    char *rcvport;//接收端口
    char *mgroup;//广播地址
    char *player_cmd;
};
extern struct client_conf_st client_conf;//entern now variable action scope
#endif
