#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shash.h"

/*------------函数声明------------*/

//计算哈希值
static int _hash(H_KEY_TYPE key, int size);

//创建哈希表节点
hashNode_t *hash_node_create(H_KEY_TYPE key, H_VALUE_TYPE value);

//销毁哈希节点
int hash_node_desy(hashNode_t *node);

//初始化哈希表
int hash_table_init(hashTable_t *hash);

//销毁哈希表
int hash_table_desy(hashTable_t *hash);

//插入元素（冲突：拉链法的头插法）
int hash_node_insert(hashTable_t *hash, H_KEY_TYPE key, H_VALUE_TYPE value);

//查找元素
hashNode_t *hash_node_search(hashTable_t *hash, H_KEY_TYPE key);

//删除元素
int hash_node_delete(hashTable_t *hash, H_KEY_TYPE key);

//打印哈希表
int hash_table_print(hashTable_t *hash);

/*------------函数声明------------*/


/*------------函数定义------------*/

//计算哈希值
static int _hash(H_KEY_TYPE key, int size) {
#if KV_HTYPE_INT_INT
    if(key < 0) {
        return -1;
    }
    return key % size;//直接返回键对哈希表长度取余
#elif KV_HTYPE_CHAR_CHAR
    if(key == NULL) {
        return -1;
    }
    int sum = 0;
    for(int i = 0; key[i] != 0; i++) {
        sum += key[i];
    }
    return sum % size;//所有字符的ASCII码累加后对哈希表长度取余
#endif
}

//创建哈希节点
hashNode_t *hash_node_create(H_KEY_TYPE key, H_VALUE_TYPE value) {
    hashNode_t *node = (hashNode_t *)calloc(1, sizeof(hashNode_t));
    if(node == NULL) {
        return NULL;
    }
#if KV_HTYPE_INT_INT
    node->key = key;
    node->value = value;
#elif KV_HTYPE_CHAR_CHAR
    char *key_copy = (char *)calloc(strlen(key) + 1, sizeof(char));
    if(key_copy == NULL) {
        free(node);
        node = NULL;
        return NULL;
    }
    char *value_copy = (char *)calloc(strlen(value) + 1, sizeof(char));
    if(value_copy == NULL) {
        free(key_copy);
        key_copy = NULL;
        free(node);
        node = NULL;
        return NULL;
    }
    strncpy(key_copy, key, strlen(key) + 1);
    strncpy(value_copy, value, strlen(value) + 1);
    node->key = key_copy;
    node->value = value_copy;
#endif
    node->next = NULL;
    return node;
}

//销毁哈希节点
int hash_node_desy(hashNode_t *node) {
    if(node == NULL) {
        return -1;
    }
    if(node->value) {
        free(node->value);
        node->value = NULL;
    }
    if(node->key) {
        free(node->key);
        node->key = NULL;
    }
    node->next = NULL;
    free(node);
    node = NULL;
    return 0;
}

//初始化哈希表
int hash_table_init(hashTable_t *hash) {
    if(hash == NULL) {
        return -1;
    }
    hash->nodes = (hashNode_t **)calloc(HASH_TABLE_SIZE, sizeof(hashNode_t *));
    if(hash->nodes == NULL) {
        return -1;
    }
    hash->table_size = HASH_TABLE_SIZE;//哈希表长使用预定义的表长
    hash->count = 0;
    return 0;
}

//销毁哈希表
int hash_table_desy(hashTable_t *hash) {
    if(hash == NULL) {
        return -1;
    }
    for (int i = 0; i < hash->table_size; i++) {
        //删除单个节点上的全部链表
        hashNode_t *node = hash->nodes[i];
        while(node != NULL) {//依次删除每个哈希索引对应的冲突节点链表
            hashNode_t *tmp = node;
            node = node->next;
            hash->nodes[i] = node;//头删法逐一释放节点
            hash->count--;
            if(hash_node_desy(tmp) != 0) {//单个节点未删除成功
                return -1;
            }
        }
    }
    if(hash->nodes) {
        free(hash->nodes);
        hash->nodes = NULL;
    }
    hash->table_size = 0;
    hash->count = 0;
    return 0;
}

//插入元素
int hash_node_insert(hashTable_t *hash, H_KEY_TYPE key, H_VALUE_TYPE value) {
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return -1;
#elif KV_HTYPE_CHAR_CHAR
    if(!hash || !key || !value) {
        return -1;
    }
#endif
    //计算哈希索引
    int index = _hash(key, hash->table_size);
    hashNode_t *node = hash->nodes[index];
    //检查该键是否已经存在于哈希表中
    while(node != NULL) {
#if KV_HTYPE_INT_INT
        if(node->key == key) return -2;
#elif KV_HTYPE_CHAR_CHAR
        if(strcmp(node->key, key) == 0) {
            return -2;
        }
#endif
        node = node->next;
    }
    //若不存在于哈希表中，创建新的节点插入哈希表
    hashNode_t *new_node = hash_node_create(key, value);
    if(new_node == NULL) {
        return -1;
    }
    //头插法插入新节点
    new_node->next = hash->nodes[index];
    hash->nodes[index] = new_node;
    hash->count++;
    return 0;
}

//查找元素
hashNode_t *hash_node_search(hashTable_t *hash, H_KEY_TYPE key) {
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return NULL;
#elif KV_HTYPE_CHAR_CHAR
    if (!hash || !key) return NULL;
#endif
    int index = _hash(key, hash->table_size);
    hashNode_t *node = hash->nodes[index];
    while(node != NULL) {
#if KV_HTYPE_INT_INT
        if(node->key == key) return node;
#elif KV_HTYPE_CHAR_CHAR
        if(strcmp(node->key, key) == 0) {
            return node;
        }
#endif
        node = node->next;
    }
    //循环结束未找到
    return NULL;
}

//删除元素
int hash_delete_node(hashTable_t *hash, H_KEY_TYPE key) {
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return -1;
#elif KV_HTYPE_CHAR_CHAR
    if (!hash || !key) return -1;
#endif
    int index = _hash(key, hash->table_size);
    hashNode_t *node = hash->nodes[index];
    //首先处理第一个元素
    if(node == NULL) {
        return -2;
    }
#if KV_HTYPE_INT_INT
    if (cur_node->key == key)
#elif KV_HTYPE_CHAR_CHAR
    if (strcmp(node->key, key) == 0)
#endif
    {
        hashNode_t *next_node = node->next;
        hash->nodes[index] = next_node;
        if(hash_node_desy(node) == 0) {
            hash->count--;
            return 0;
        }
        else {
            return -1;
        }
    }
    //第一个元素不是需要删除的元素，处理链表
    else {
        hashNode_t *pre_node = hash->nodes[index];//删除操作设置前驱指针
        node = pre_node->next;
        while(node != NULL) {
#if KV_HTYPE_INT_INT
            if (cur_node->key == key) break;
#elif KV_HTYPE_CHAR_CHAR
            if (strcmp(node->key, key) == 0) break;
#endif
            pre_node = node;
            node = node->next;
        }
        //没找到要删除的元素
        if(node == NULL) {
            return -2;
        }
        else {
            pre_node->next = node->next;
            hash->count--;
            hash_node_desy(node);
        }
    }
}

//打印哈希表
int hash_table_print(hashTable_t *hash) {
    if(hash == NULL) {
        return -1;
    }
    for(int i = 0; i < hash->table_size; i++) {
        hashNode_t *node = hash->nodes[i];
        if(node == NULL) {
            printf("index : %d", i);
            while(node != NULL) {
#if KV_HTYPE_INT_INT
                printf(" %d", node->key);
#elif KV_HTYPE_CHAR_CHAR
                printf(" %s", node->key);
#endif
                node = node->next;
            }
            printf("\n");
        }
    }
    return 0;
}

//KV初始化
int kv_hash_init(kv_shash_t *kv_addr) {
    if(kv_addr == NULL) {
        return -1;
    }
    return hash_table_init(kv_addr);
}

//KV销毁
int kv_hash_desy(kv_shash_t *kv_addr) {
    if(kv_addr == NULL) {
        return -1;
    }
    return hash_table_desy(kv_addr);
}

//插入指令
int kv_hash_set(kv_shash_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens==NULL || tokens[1]==NULL || tokens[2]==NULL) {
        return -1;
    }
    return hash_node_insert(kv_addr, tokens[1], tokens[2]);
}

//查找指令
char *kv_hash_get(kv_shash_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL) {
        return NULL;
    }
    hashNode_t *node = hash_node_search(kv_addr, tokens[1]);
    if(node == NULL) {
        return NULL;
    }
    return node->value;
}

//删除指令
int kv_hash_delete(kv_shash_t *kv_addr, char **tokens) {
    return hash_node_delete(kv_addr, tokens[1]);
}

//计数指令
int kv_hash_count(kv_shash_t *kv_addr) {
    return kv_addr->count;
}

//存在指令
int kv_hash_exist(kv_shash_t *kv_addr, char **tokens) {
    return (hash_node_search(kv_addr, tokens[1]) != NULL);
}

/*------------函数定义------------*/