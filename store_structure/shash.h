#ifndef _SHASH_H
#define _SHASH_H

#define KV_HTYPE_INT_INT 0
#define KV_HTYPE_CHAR_CHAR 1

#define HASH_HTYPE_DEBUG 0//是否运行测试代码

#define HASH_TABLE_SIZE 1024//哈希表的最大长度（哈希表中索引个数）

#if KV_HTYPE_INT_INT
typedef int H_KEY_TYPE;
typedef int H_VALUE_TYPE;
#elif KV_HTYPE_CHAR_CHAR
typedef char *H_KEY_TYPE;
typedef char *H_VALUE_TYPE;
#endif

//哈希表节点(存储单个键值对)结构体
typedef struct hashNode_s {
    H_KEY_TYPE key;//键
    H_VALUE_TYPE value;//值
    struct hash_node_s *next;//同一哈希索引对应的哈希链表中的下一节点
} hashNode_t;

//哈希表结构体
typedef struct hashTable_s {
    hashNode_t **nodes;//哈希表主体
    int table_size;//哈希表长度
    int count;//哈希表中存储的元素总数
} hashTable_t;

typedef hashTable_t kv_shash_t;

/*------------KV功能函数声明------------*/

//初始化
int kv_hash_init(kv_shash_t *kv_addr);

//销毁
int kv_hash_desy(kv_shash_t *kv_addr);

//插入指令
int kv_hash_set(kv_shash_t *kv_addr, char **tokens);

//查找指令
char *kv_hash_get(kv_shash_t *kv_addr, char **tokens);

//删除指令
int kv_hash_delete(kv_shash_t *kv_addr, char **tokens);

//计数指令
int kv_hash_count(kv_shash_t *kv_addr);

//存在指令
int kv_hash_exist(kv_shash_t *kv_addr, char **tokens);

/*------------KV功能函数声明------------*/
#endif