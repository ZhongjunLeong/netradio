#ifndef THR_LIST_H__
#define THR_LIST_H__
#include "medialib.h"
/*该节目单通过socket往外发*/
int thr_list_create(struct mlib_listentry_st*, int);

int thr_list_destory(void);



#endif
