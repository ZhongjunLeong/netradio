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
#include "thr_channel.h"
#include "server_conf.h"

/*创建多个频道线程：记录线程标识*/
struct thr_channel_ent_sdt
{
    chnid_t chnid;
    pthread_t tid;
};
static int tid_nextpos = 0;

struct thr_channel_ent_sdt thr_channel[CHNNR];

/*组织频道信息和发送*/
static void *thr_channel_snder(void* ptr)
{
    struct msg_channel_st *sbufp;
    struct mlib_listentry_st *ent = ptr;
    int len;
    /*按最大频道内存创建包*/
    sbufp = malloc(MSG_CHANNEL_MAX);
    if(sbufp == NULL)
    {
        syslog(LOG_ERR, "malloc():%s", strerror(errno));
        exit(1);
    }

    sbufp->chnid = ent->chnid;
    while(1)
    {
        /*读频道*/
        len = mlib_readchn(ent->chnid, sbufp->data, MAX_DATA);
        /*读到的发送，按照proto格式发送*/
        if(sendto(serversd, sbufp, len + sizeof(chnid_t), 0, (void *)&sndaddr, sizeof(sndaddr)) < 0)
        {
            syslog(LOG_ERR, "thr_channel(%d):sendto():%s", ent->chnid, strerror(errno));
        }
        else
        {
             syslog(LOG_ERR, "thr_channel(%d):sendto():sucesses", ent->chnid);
        }
        /*主动出让调度器*/
        sched_yield();
    }
    pthread_exit(NULL);
}

int thr_channel_create(struct mlib_listentry_st* ptr)
{
    
    int err;
    
    err = pthread_create(&thr_channel[tid_nextpos].tid, NULL, thr_channel_snder, ptr);
  
    if(err)
    {
        syslog(LOG_WARNING, "phread_create():%s", strerror(err));
        return -err;
    }
    //每创建一次线程，频道号回填
    thr_channel[tid_nextpos].chnid = ptr->chnid;
    tid_nextpos++;
   
    return 0;
}

int thr_channel_destroy(struct mlib_listentry_st* ptr)
{
    int i;
    for(i = 0; i < CHNNR; i++)
    {
        /*取消已经发送过去的频道，并不是按顺序全取消了*/
        if(thr_channel[i].chnid == ptr->chnid)
        {
               if(pthread_cancel(thr_channel[i].tid) < 0)
               {
                    syslog(LOG_ERR, "phread_cancel():the thread of channel %d", ptr->chnid);
                    return -ESRCH;
               }
        }
     
        pthread_join(thr_channel[i].tid, NULL);
        thr_channel[i].chnid = -1;
        return 0;
    }
    
}

/*s所有频道取消*/
int thr_channel_destroyall(void)
{
    int i;
    for(i = 0 ; i < CHNNR; i++)
    {
         if(thr_channel[i].chnid > 0 )
        {
               if(pthread_cancel(thr_channel[i].tid) < 0)
               {
                    syslog(LOG_ERR, "phread_cancel():the thread of channel %d", thr_channel[i].chnid);
                    return -ESRCH;
               }
                pthread_join(thr_channel[i].tid, NULL);
                thr_channel[i].chnid = -1;
        }
    }
}
