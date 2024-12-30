#ifndef _RBTREE_H
#define _RBTREE_H

#define KV_RBTYPE_INT_VOID 0
#define KV_RBTYPE_CHAR_CHAR 1

#define ENABLE_RBTREE_DEBUG 0

//键值对的类型
#if KV_RBTYPE_INT_VOID
typedef int RB_KEY_TYPE;
typedef int RB_VALUE_TYPE;
#elif KV_RBTYPE_CHAR_CHAR
typedef char *RB_KEY_TYPE;
typedef char *RB_VALUE_TYPE;
#endif

//红黑树节点结构体
typedef struct _rbtree_node_s {
    RB_KEY_TYPE key;
    RB_VALUE_TYPE value;

    struct _rbtree_node_s *left;//左孩子指针
    struct _rbtree_node_s *right;//右孩子指针
    struct _rbtree_node_s *parent;//父节点指针

    unsigned char color;
} rbtree_node;

//红黑树结构体
typedef struct _rbtree_s {
    struct _rbtree_node_s *root_node;//根节点
    struct _rbtree_node_s *nil_node;//空节点/叶子节点或根节点的父节点
    int count;//节点总数
} rbtree;

typedef rbtree kv_rbtree_t;

/*------------KV函数声明------------*/

//初始化
int kv_rbtree_init(kv_rbtree_t *kv_addr);

//销毁
int kv_rbtree_desy(kv_rbtree_t *kv_addr);

//插入指令
int kv_rbtree_set(kv_rbtree_t *kv_addr, char **tokens);

//查找指令
char *kv_rbtree_get(kv_rbtree_t *kv_addr, char **tokens);

//删除指令
int kv_rbtree_delete(kv_rbtree_t *kv_addr, char **tokens);

//计数指令
int kv_rbtree_count(kv_rbtree_t *kv_addr);

//存在指令
int kv_rbtree_exist(kv_rbtree_t *kv_addr, char **tokens);

/*------------KV函数声明------------*/
#endif