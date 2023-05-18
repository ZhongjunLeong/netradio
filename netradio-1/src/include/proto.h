#ifdef PROTO_H__
#define PROTO_H__

#include <site_type.h>
#define DEFAULT_MGROUP  "224.2.2.2"
#define DEFAULT_RCVPORT "1989"

#define CHNNR   200
#define MINCHNID  1
#define MAXCHNID  (MINCHNID+CHNNR-1)
#define LISTENCHNID 0 //0 send project
#define MSG_CHANNEL_MAX (65536-20-8) //ip daragram head is 20 &udp head is 
#define MAX_DATA   (MSG_CHNNL_MAX-sizeof(chnid_t))
#define MSG_LIST MAX  (65536-20-8)
#define MAX_ENTRY       MSG_LIST_MAX-sizeof(chnid_t))
struct msg_channel_st //send
{

    chnid_t chnid;          /*must between [MINCHNID,MAXCHNID]*/
    uint8_t data[1];
}__attribute__((packed));

struct msg_listentry_st
{
    chnid_t chnid;
    uint8_t desc[1];

}__attribute__((packed));
struct msg_list_st
{
    chnid_t chnid;  //must be LISTCHNID
    struct msg_listentry_st entry[1];
}__attributr__((packed));


#endif
