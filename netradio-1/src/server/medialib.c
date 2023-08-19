#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <proto.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "server_conf.h"
#include "mytbf.h"
#include "medialib.h"

#define PATHSIZE 1024
#define LINEBUFSIZE     1024
#define MP3_BITRATE     (128 * 1024) // correct bps:128*1024

struct channel_context_st
{
    chnid_t chnid;  //频道id
    char *desc; //频道描述
    glob_t mp3glob; //存放歌迷的目录
    int pos;    //下标
    int fd; //文件
    off_t offset;
    mytbf_t *tbf;//flow control

};
/*数组放着MAXCHNID个频道信息地址+1是空出节目单*/
static struct channel_context_st channel[MAXCHNID+1];

/*读取频道中的目录，得到节目单
*path--频道目录
*return --频道信息地址
*/
static struct channel_context_st *path2entry(const char *path)
{
    // path/desc.text  path/*.mp3
    syslog(LOG_INFO, "current path :%s\n", path);
    /*存放节目描述的路径的数组*/
    char pathstr[PATHSIZE] = {'\0'};
    char linebuf[LINEBUFSIZE];
    FILE *fp;
    struct channel_context_st *me;
    static chnid_t curr_id = MINCHNID;
    /*将path的字符串加到pathstr的末尾*/
    strcat(pathstr, path);
    strcat(pathstr, "/desc.text");

    fp = fopen(pathstr, "r");
    syslog(LOG_INFO, "channel dir:%s\n", pathstr);
    if (fp == NULL)
    {
        syslog(LOG_INFO, "%s is not a channel dir(can't find desc.txt)", path);
        return NULL;
    }
    /*读取频道目录下描述文件信息放入linebuf缓存中*/
    if(fgets(linebuf, LINEBUFSIZE, fp) == NULL)
    {
        syslog(LOG_INFO, "%s is not a channel dir(cant get the desc.text)", path);
        fclose(fp);
        return NULL;
    }
    /*将linebuf中的信息按照struct channel_context_st结构存储*/
    me = malloc(sizeof(*me));
     if (me == NULL)
    {
        syslog(LOG_ERR, "malloc():%s", strerror(errno));
        return NULL;
    }
    /*取出时 流量控制*/
    me->tbf = mytbf_init(MP3_BITRATE /8, MP3_BITRATE /8 * 5);
    if (me->tbf == NULL)
    {
        syslog(LOG_ERR, "mytbf_init():%s", strerror(errno));
        free(me);
        return NULL;
    }

    me->desc = strdup(linebuf);

    /*读取频道目录下音频信息放入linebuf缓存中*/
    strncpy(pathstr, path, PATHSIZE);
    strncat(pathstr, "/*.mp3", PATHSIZE-1);
   // printf("音频路径：%s\n",pathstr);
    pathstr[PATHSIZE -1] = 0;
   // printf("音频路径：%s\n",pathstr);
  
    if(glob(pathstr, 0, NULL, &me->mp3glob) != 0)   //从pathsrt下路径读取的音频目录信息放仔mp3glob中
    {
        //printf("path:%ld,err:%d\n",me->mp3glob.gl_pathc,err);
        curr_id++;
        syslog(LOG_ERR, "%s is not a channel dir(can not find mp3 files:%s", path, strerror(errno));
        free(me);
        return NULL;
    }

    me->pos = 0;
    me->offset = 0;
    me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY);//打开一个频道下第pos个音频
    if(me->fd < 0)
    {
        syslog(LOG_WARNING, "%s open failed.", me->mp3glob.gl_pathv[me->pos]);
        free(me);
        return NULL;
    }
    /*下一次调用转换到下一个频道*/
    me->chnid = curr_id;
    curr_id++;

    return me;
}

/*获取每个频道的节目单信息
*result：频道里的节目单
*resnum：频道个数
*/
int mlib_getchnlist(struct mlib_listentry_st **result, int *resnum)
{
    glob_t globres;
    char path[PATHSIZE];
    int num = 0;
    struct mlib_listentry_st *ptr;  
    struct channel_context_st *res;
    int i;
    /*初始化*/
    for(i = 0; i < MAXCHNID+1; i++)
    {
        channel[i].chnid = -1;//now do not start

    }
    snprintf(path, PATHSIZE, "%s/*", server_conf.media_dir);//获取媒体库路径
    /*解析媒体库所在目录下的个数，拿到每一个mp3文件和desc文件*/

    if(glob(path, 0, NULL, &globres))
    {
        return -1;
    }
    /*将频道信息通过节目单mlib_listentry_st形式呈现，回填入参*/
    ptr = malloc(sizeof(struct mlib_listentry_st)*globres.gl_pathc);
    if(ptr == NULL)
    {
        syslog(LOG_ERR,"malloc() error");
        exit(1);

    }
    /*获取每个待解析的路径，即频道个数*/
    for(i = 0; i < globres.gl_pathc; i++)//read vaildate catalogue
    {
    //globres.gl_pathv[i] -> "/var/media/ch1"
       res = path2entry(globres.gl_pathv[i]);
       if(res != NULL)
       {
            syslog(LOG_ERR,"path2entry() return :%d %s", res->chnid, res->desc);
            memcpy(channel + res->chnid, res, sizeof(*res));//将读取到的节目单信息拷贝到存放节目单信息的数组内存中
            ptr[num].chnid = res->chnid;
            ptr[num].desc = strdup(res->desc);
             num++;
       }
      
    }
    /*每个频道都有一个节目单，每读取一个频道节目单，扩建一个内存*/
    *result = realloc(ptr, sizeof(struct mlib_listentry_st) * num);
   
    if (*result == NULL)
    {
        syslog(LOG_ERR, "realloc() failed.");
    }

    *resnum = num;

    return 0;
}

int mlib_freechnlist(struct mlib_listentry_st *ptr)
{

    free(ptr);
    return 0;

}
//当前是失败了或者已经读取完毕才调用,打开下一个
static int open_next(chnid_t chnid)
{
    int i;
    for(i = 0; i < channel[chnid].mp3glob.gl_pathc; i++)
    {
        channel[chnid].pos++;
        /*打开失败*/
        if(channel[chnid].pos == channel[chnid].mp3glob.gl_pathc)
        {
            channel[chnid].pos = 0;
           break;
        }
        /*重新打开*/
        close(channel[chnid].fd);
        channel[chnid].fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], O_RDONLY);
        if(channel[chnid].fd < 0)
        {
            syslog(LOG_WARNING, "open(%s):%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
        }
        else
        {
            channel[chnid].offset = 0;
            return 0;
        }
    }
    syslog(LOG_ERR, "none of mp3 in channel %d is available", chnid );
    
}
ssize_t mlib_readchn(chnid_t chnid, void * buf, size_t size)
{
    int tbfsize = 0;
    int len;//读取大小

    tbfsize = mytbf_fetchtoken(channel[chnid].tbf, size);
    printf("tbfsize:%d\n",tbfsize);
    while(1)
    {
        len = pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);//从文件指定位置channel[chnid].offset开始读
        if(len < 0)
        {
            syslog(LOG_ERR, "media file %s pread() failed.:%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno) );
           open_next(chnid);
                //syslog(LOG_ERR, "channel  %s:there is no successed open.",chnid);

        }
        else if(len == 0)
        {
            syslog(LOG_ERR, "media file %s is over.", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos] );
            open_next(chnid);
                //syslog(LOG_ERR, "channel  %s:there is no successed open.",chnid);
        }
            else    //len >  0
            {
                channel[chnid].offset += len;
                break;
            }
    }
    //消耗了len令牌数
    if(tbfsize - len > 0)
    {
        mytbf_returntoken(channel[chnid].tbf, tbfsize-len);
    }

    return len;

    
}
