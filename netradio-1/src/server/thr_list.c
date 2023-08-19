#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <proto.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "thr_list.h"
#include "server_conf.h"


static pthread_t tid_list;//线程号
static int nr_list_ent; //频道数
static struct mlib_listentry_st *list_ent;  //单个频道节目单包

static void *thr_list(void *s)
{
    int i, size, ret;
    int totalsize;
    struct msg_list_st *entlistp;   //可通过socket发送的总的节目单包结构
    struct msg_listentry_st *entryp;    //可通过socket发送的单个频道的节目单包结构

    totalsize = sizeof(chnid_t);
    /*计算需要发送的频道包总大小*/
    for(i = 0; i < nr_list_ent; i++)
    {
        //要发送的节目单包大小+获取的媒体库中节目包的描述字符串长度（desc）+每个频道号大小（chnid_t)
        totalsize += sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);//list_ent[i].desc是字符数组的指针，strlen表示描述信息的字符串长度
    }
   // printf("一个频道包大小：%d\n",totalsize);
    /*创建一个按proto协议的节目单包*/
    entlistp = malloc(totalsize);
    if(entlistp == NULL)
    {
        syslog(LOG_ERR, "malloc:%s", strerror(errno));
        exit(1);
    }
   // printf("协议节目单格式包地址：%p\n", entlistp);
    entlistp->chnid = LISTCHNID;
    entryp = entlistp->entry;//proto协议格式的节目单实体，描述节目单信息
  
    for(i = 0; i < nr_list_ent; i++)
    {//循环：每个频道的节目单创建内存，entryp地址+size指首地址创建
        size = sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);
        entryp->chnid = list_ent[i].chnid;
        entryp->len = htons(size);
        strcpy(entryp->desc, list_ent[i].desc);//将媒体库中的节目单描述复制给proto格式的节目单实体描述中
        entryp = (void*)(((char *)entryp) + size);
    }
    
    while(1)
    {
        ret = sendto(serversd, entlistp, totalsize, 0, (void *)&sndaddr, sizeof(sndaddr));
    if(ret < 0)
    {
        syslog(LOG_WARNING, "sendto(serversd, entlistp....:%s",strerror(errno));
    }
    else
    {
          syslog(LOG_DEBUG, "sendto(serversd, entlistp....:");
    }
    sleep(1);
    }

}

/*从媒体库获取节目单
*listp --从媒体库中获取每个频道的节目单信息
*nr_ent--频道数
*return--成功：0 失败-1
*/
int thr_list_create(struct mlib_listentry_st* listp, int nr_ent)
{
    int err;
    list_ent = listp;
    nr_list_ent = nr_ent;
    /*创建线程：发送节目单*/
    err = pthread_create(&tid_list, NULL, thr_list, NULL);
     printf("节目单list create success\n");
    if(err)
    {
        syslog(LOG_ERR, "phread_create.:%s",strerror(errno));
        return -1;
    }

    return 0;
    
}
int thr_list_destory(void)
{
    pthread_cancel(tid_list);
    pthread_join(tid_list, NULL);
    
    return 0;
}