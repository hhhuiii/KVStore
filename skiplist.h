#ifndef __SKIPLIST_H
#define __SKIPLIST_H

#define KV_ZTYPE_INT_INT 0
#define KV_ZTYPE_CHAR_CHAR 1

#define ENABLE_SKIPLIST_DEBUG 0

//键值对类型
#if KV_ZTYPE_INT_INT
    typedef int Z_KEY_TYPE;
    typedef int Z_VALUE_TYPE;
#elif KV_ZTYPE_CHAR_CHAR
    typedef char *Z_KEY_TYPE;
    typedef char *Z_VALUE_TYPE;
#endif

//跳表单节点结构体
typedef struct skiplist_node_s {
    Z_KEY_TYPE key;
    Z_VALUE_TYPE value;
    struct skiplist_node_s **next;//不同层级的后继节点的指针构成的数组
}skiplist_node_t;

//跳表结构体
typedef struct skiplist_s {
    struct skiplist_node_s *header;//跳表首节点指针
    int count;//跳表元素数量
    int cur_level;//跳表当前的总层级
    int max_level;//跳表最大层级，由初始化确定
}skiplist_t;

typedef skiplist_t kv_skiplist_t;

/*------------KV函数声明------------*/

//初始化
int kv_skiplist_init(kv_skiplist_t *kv_addr, int m);

//销毁
int kv_skiplist_desy(kv_skiplist_t *kv_addr);

//插入指令
int kv_skiplist_set(kv_skiplist_t *kv_addr, char **tokens);

//查找指令
char *kv_skiplist_get(kv_skiplist_t *kv_addr, char **tokens);

//删除指令
int kv_skiplist_delete(kv_skiplist_t *kv_addr, char **tokens);

//计数指令
int kv_skiplist_count(kv_skiplist_t *kv_addr);

//存在指令
int kv_skiplist_exist(kv_skiplist_t *kv_addr, char **tokens);

/*------------KV函数声明------------*/
#endif