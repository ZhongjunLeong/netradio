#ifndef MYTBF_H__
#define MYTBF_H__
#define MYTBF_MAX  1024
typedef void mytbf_t;

mytbf_t *mytbf_init(int cps,int brust); //速率cps和上限brust
//取领牌
int mytbf_fetchtoken(mytbf_t *,int);
//还令牌
int mytbf_returntoken(mytbf_t *,int);

int mytbf_destory(mytbf_t *);

#endif
