#ifndef _ARRAY_H
#define _ARRAY_H

#define kv_array_block_size 32//单个块存储的最多键值对数量

//键值对结构体
typedef struct kv_array_item_s {
    char *key;
    char *value;
}kv_array_item_t;

//存储键值对的内存块
typedef struct kv_array_block_s {
    int count;//当前块存储的Key-Value数量，最大为kv_array_block_size
    kv_array_item_t *items;//存储Key-Value的数组指针
    struct kv_array_block_s *next;//指向下一内存块
}kv_array_block_t;

//array链表的头节点
typedef struct kv_array_s {
    struct kv_array_block_s *head;//头节点指针
    int count;//存储的所有Key-Value对数量
}kv_array_t;

/*------------KV功能函数声明------------*/
//传入参数为链表的头节点
//初始化
int kv_array_init(kv_array_t *kv_addr);

//销毁
int kv_array_desy(kv_array_t *kv_addr);

//插入指令：有就报错，没有就创建
int kv_array_set(kv_array_t *kv_addr, char **tokens);

//查找指令
char *kv_array_get(kv_array_t *kv_addr, char **tokens);

//删除指令
int kv_array_delete(kv_array_t *kv_addr, char **tokens);

//计数指令
int kv_array_count(kv_array_t *kv_addr);

//检查存在与否
int kv_array_exist(kv_array_t *kv_addr, char **tokens);

/*------------KV功能函数声明------------*/
#endif