#ifndef MEDIALIB_H__
#define MEDIALIB_H__
#include <proto.h>
//记录每条节目信息：频道号chnid，节目描述信息 *desc
struct mlib_listentry_st
{
    chnid_t chnid;
    char *desc; //节目单描述信息
};

int mlib_getchnlist(struct mlib_listentry_st **, int *); //从媒体库中获取节目单信息和频道总数，最终以结构体mlib_listentry_st形式呈现

int mlib_freechnlist(struct mlib_listentry_st *);   //释放节目单信息存储所占的内存

ssize_t mlib_readchn(chnid_t, void *, size_t);    //按频道读取对应媒体库流媒体内容

#endif