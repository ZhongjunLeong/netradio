#ifndef PROTO_H__
#define PROTO_H__

#include <stdint.h>
#include <stdio.h>
typedef uint8_t chnid_t; //无符号整形数

#define DEFAULT_MGROUP  "224.2.2.2"     //广播地址
#define DEFAULT_RCVPORT "1989"          //端口号

#define CHNNR   200     //总频道个数
#define MINCHNID  1     //最小频道数
#define MAXCHNID  (MINCHNID+CHNNR-1)
#define LISTCHNID 0 //节目单的频道号
#define MSG_CHANNEL_MAX (65536-20-8) //UDP包允许的最大值-ip包的报头 20字节-UDP包报头8字节
#define MAX_DATA   (MSG_CHANNEL_MAX-sizeof(chnid_t)) //data包上限，若大于则出问题
#define MSG_LIST_MAX  (65536-20-8)
#define MAX_ENTRY       MSG_LIST_MAX-sizeof(chnid_t))
struct msg_channel_st //频道包
{

    chnid_t chnid;          /*must between [MINCHNID,MAXCHNID]频道号*/
    uint8_t data[1];        //变长的包
}__attribute__((packed));   //告诉编译器该结构体不对齐

struct msg_listentry_st //节目单的描述信息
{
    chnid_t chnid;  
    uint16_t len;   //一个节目的描述信息长度
    uint8_t desc[1];

}__attribute__((packed));
struct msg_list_st  //总节目单
{
    chnid_t chnid;  //must be LISTCHNID
    struct msg_listentry_st entry[1];
}__attribute__((packed));


#endif
