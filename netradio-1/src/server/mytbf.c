#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "mytbf.h"
/*令牌桶结构*/
struct mytbf_st
{
    int cps;    //速率：每次传送速率
    int brust;  //桶的令牌上限
    int token;  //取令牌数
    int pos;    //当前结构体数组的下标
    pthread_mutex_t mut;    //动态定义互斥锁，保护桶：分别加令牌和取令牌
    pthread_cond_t cond;    //通知机制：条件变量
};

static struct mytbf_st *job[MYTBF_MAX];//共享数组，多个桶
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER; //静态定义互斥锁，保护共享数组
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static pthread_t tid;
/*线程：往多个桶内cps速率加令牌*/
static void *thr_alrm(void *p)
{
    int i;
    int j = 0;
    while(1)
    {
        pthread_mutex_lock(&mut_job);
        for(i = 0; i < MYTBF_MAX ; i++)
        {//printf("令牌线程ok\n");
            /*遍历数组地址，桶若存在，加令牌*/
            if(job[i] != NULL)
            { //printf("令牌桶加令牌\n");
                pthread_mutex_lock(&job[i]->mut);
                /*以cps速率往桶里加令牌*/
                job[i]->token += job[i]->cps;
                if(job[i]->token > job[i]->brust)
                {
                    job[i]->token =  job[i]->brust;
                }
                j++;
                pthread_cond_broadcast(&job[i]->cond);
                pthread_mutex_unlock(&job[i]->mut);
            }
        }
      //  printf("令牌桶加令牌:%d\n",j);
        pthread_mutex_unlock(&mut_job);

        sleep(1);
        
    }
}

static void module_unload(void)
{
    int i;
    pthread_cancel(tid);
    pthread_join(tid, NULL);
/*每个桶使用数组表示，都malloc一块空间*/
    for(i = 0; i < MYTBF_MAX; i++)
        free(job[i]);
    return ;
}
/*只被执行一次，创建线程*/
static void module_load(void )
{
    
    int err;

    err = pthread_create(&tid, NULL, thr_alrm, NULL);
    if(err)
    {
        fprintf(stderr,"pthread_create():%s\n",strerror(errno));
        exit(1);
    }
   
    atexit(module_unload);
}

/*查找空的桶*/
static int get_free_pos_unlocked(void )
{
   // printf("进入查找空令牌桶\n");
    int i;
    for(i = 0 ;i < MYTBF_MAX; i++)
    {
        if(job[i] == NULL)
            return i;
       
    } 
    return -1;
}

/*令牌桶初始化：结构体赋值*/
mytbf_t *mytbf_init(int cps, int brust)
{
    struct mytbf_st *me;
    int pos;
   pthread_once(&init_once,module_load);
    /*动态模块单次初始化，确保module_load函数只被加载一次*/
   // module_load();
    /*创建一个桶*/
    me = malloc(sizeof(*me));
    if(me == NULL)
        return NULL;

    me->cps = cps;
    me->brust = brust;
    me->token = 0;

    pthread_mutex_init(&me->mut,NULL);
    pthread_cond_init(&me->cond,NULL);

    pthread_mutex_lock(&mut_job);
    pos = get_free_pos_unlocked();//查找空闲的线程去取令牌
    // printf("空桶：%d\n", pos);
    if(pos < 0)
    { //printf("pos <0\n");
        pthread_mutex_unlock(&mut_job);
        free(me);
        return NULL;
    }
    me->pos = pos;
    job[me->pos] = me;
    //printf("令牌桶地址：%p\n", me);
    pthread_mutex_unlock(&mut_job);
 
    return me;

}

static int min(int a, int b)
{
    return a > b ? b : a;
}

int mytbf_fetchtoken(mytbf_t *ptr, int size)
{
    int n;
    struct mytbf_st *me = ptr;
    /*没令牌*/
    pthread_mutex_lock(&me->mut);
    while(me->token <= 0)
        pthread_cond_wait(&me->cond, &me->mut);//等待

    n = min(me->token, size);//桶中令牌与取的令牌取小值
    me->token -= n;
    pthread_mutex_unlock(&me->mut);

    return n;
}

int mytbf_returntoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *me = ptr;

    pthread_mutex_lock(&me->mut);
    me->token += size;
    if(me->token > me->brust)
        me->token = me->brust;

    pthread_cond_broadcast(&me->cond);//发通知，打断正在等令牌的wait线程
    pthread_mutex_unlock(&me->mut);
    return 0;
}

/*销毁桶*/
int mytbf_destory(mytbf_t *ptr)
{
    struct mytbf_st *me = ptr;

    pthread_mutex_lock(&mut_job);
    job[me->pos] = NULL;
    pthread_mutex_unlock(&mut_job);

    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);
    free(ptr);
    return 0;

}
int mytbf_checktoken(mytbf_t *ptr)
{
    int token_left = 0;
    struct mytbf_st *me = ptr;
    pthread_mutex_lock(&me->mut);
    token_left = me->token;
    pthread_mutex_unlock(&me->mut);
    return token_left;
}