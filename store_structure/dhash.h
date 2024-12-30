#ifndef _DHASH_H
#define _DHASH_H

//虽说提供了两种数据类型的选择，但在实现中并未使用INT_INT类型的键值对，此类型条件判断简单且与CHAR_CHAR类型大同小异
#define KV_DHTYPE_INT_INT 0
#define KV_DHTYPE_CHAR_CHAR 1

#define ENABLE_DHASH_DEBUG 0

#define DHASH_INIT_TABLE_SIZE 512//初始哈希表长
#define DHASH_GROW_FACTOR 2//动态哈希表的扩展/收缩倍数

//键值对结构体
#if KV_DHTYPE_INT_INT 0
typedef int DH_KEY_TYPE;
typedef int DH_VALUE_TYPE;
#elif KV_DHTYPE_CHAR_CHAR 1
typedef int *DH_KEY_TYPE;
typedef int *DH_VALUE_TYPE;
#endif

//哈希表节点的定义
typedef struct dhash_node_s {
    DH_KEY_TYPE key;
    DH_VALUE_TYPE value;
} dhash_node_t;

//哈希表结构体
typedef struct dhash_table_s {
    struct dhash_node_s **nodes;//哈希表指针（指向哈希表节点指针数组）
    int max_size;//哈希表的最大容量
    int count;//哈希表中存储的元素总数
} dhash_table_t;

typedef dhash_table_t kv_dhash_t;//定义在KV协议中使用的哈希表类型


/*------------函数声明------------*/

//初始化
int kv_dhash_init(kv_dhash_t *kv_addr);

//销毁
int kv_dhash_desy(kv_dhash_t *kv_addr);

//插入指令
int kv_dhash_set(kv_dhash_t *kv_addr, char **tokens);

//查找指令
char *kv_dhash_get(kv_dhash_t *kv_addr, char **tokens);

//删除指令
int kv_dhash_delete(kv_dhash_t *kv_addr, char **tokens);

//计数指令
int kv_dhash_count(kv_dhash_t *kv_addr);

//存在指令
int kv_dhash_exist(kv_dhash_t *kv_addr, char **tokens);

/*------------函数声明------------*/
#endif