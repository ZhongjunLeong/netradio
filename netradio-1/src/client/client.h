#ifndef CLIENT_H__
#define CLIENT_H__
#define DEFAULT_PALYERCMD "/usr/bin/mpg123 - > /dev/null"//add - doing stdin file 
struct client_conf_st
{
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};
extern struct client_conf_st client_conf;//entern now variable action scope
#endif
