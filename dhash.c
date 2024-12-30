#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dhash.h"

/*------------dhash函数声明------------*/

//计算哈希值
static int dhash_function(DH_KEY_TYPE key, int size);

//创建哈希节点
dhash_node_t *dhash_node_create(DH_KEY_TYPE key, DH_VALUE_TYPE value);

//销毁哈希节点
int dhash_node_desy(dhash_node_t *node);

//初始化哈希表
int dhash_table_init(dhash_table_t *dhash, int size);

//销毁哈希表
int dhash_table_dey(dhash_table_t *dhash);

//插入元素
int dhash_table_insert(dhash_table_t *dhash, DH_KEY_TYPE key, DH_VALUE_TYPE value);

//查找元素
int dhash_table_search(dhash_table_t *dhash, DH_KEY_TYPE key);

//删除元素
int dhash_node_delete(dhash_table_t *dhash, DH_KEY_TYPE key);

//打印哈希表
int dhash_table_print(dhash_table_t *dhash);

/*------------dhash函数声明------------*/


/*------------函数定义------------*/

//计算哈希值
static int dhash_function(DH_KEY_TYPE key, int size) {
    unsigned long int sum = 0;
    for (int i = 0; i < strlen(key); i++) {
        sum = sum * 37 + key[i];//字节的ASCII值加权和计算方法
    }
    sum = sum % size;
    return (int)sum;
}

//创建哈希表节点
dhash_node_t *dhash_node_create(DH_KEY_TYPE key, DH_VALUE_TYPE value) {
    dhash_node_t *node = (dhash_node_t *)calloc(1, sizeof(dhash_node_t));
    if(!node) {
        return NULL;
    }
    //传入的是指针，独立拷贝操作
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
    return node;
}

//销毁哈希节点
int dhash_node_desy(dhash_node_t *node) {
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
    free(node);
    node = NULL;
    return 0;
}

//初始化哈希表
int dhash_table_init(dhash_table_t *dhash, int size) {
    if(dhash == NULL) {
        return -1;
    }
    dhash->nodes = (dhash_node_t **)calloc(size, sizeof(dhash_node_t *));
    if(dhash->nodes == NULL) {
    return -1;
    }
    dhash->max_size = size;
    dhash->count = 0;
    return 0;
}   

//销毁哈希表
int dhash_table_desy(dhash_table_t *dhash) {
    if(dhash == NULL) {
        return -1;
    }
    for (int i = 0; i < dhash->max_size; i++) {
        //销毁哈希表上所有的单个节点
        while(dhash->nodes[i] != NULL) {
            int ret = dhash_node_desy(dhash->nodes[i]);
            dhash->count--;
            dhash->nodes[i] = NULL;
            if(ret != 0) {
                return -1;
            }
        }
    }
    if(dhash->nodes) {
        free(dhash->nodes);
        dhash->nodes = NULL;
    }
    dhash->max_size = 0;
    dhash->count = 0;
    return 0;
}

//插入元素
int dhash_node_insert(dhash_table_t *dhash, DH_KEY_TYPE key, DH_VALUE_TYPE value) {
    if (!dhash || !key || !value) {
        return -1;
    }
    //检查是否需要扩展哈希表，若使用容量超过总容量1/2
    if(dhash->count > (dhash->max_size >> 1)) {
        dhash_table_t new_table;
        int ret = dhash_table_init(&new_table, dhash->max_size * DHASH_GROW_FACTOR);
        if(ret != 0) {
            return -1;
        }
        //将元素迁移到新的哈希表中
        for(int i = 0;i < dhash->max_size; i++) {
            if(dhash->nodes[i] != NULL) {
                ret = dhash_node_insert(&new_table, dhash->nodes[i]->key, dhash->nodes[i]->value);
                if(ret != 0) {
                    return ret;
                }
            }
        }
        //交换表头
        dhash->max_size *= DHASH_GROW_FACTOR;
        dhash->count = new_table.count;
        dhash_node_t **tmp_nodes = dhash->nodes;
        dhash->nodes = new_table.nodes;
        new_table.nodes = tmp_nodes;
        new_table.max_size /= DHASH_GROW_FACTOR;
        //新创建的表头交换为旧哈希表的标头被传入销毁函数借此销毁旧哈希表
        ret = dhash_table_desy(&new_table);
        if(ret != 0) {
            return ret;
        }
    }
    //扩容检查完毕，寻找插入新节点的位置
    int index = dhash_function(key, dhash->max_size);
    while(dhash->nodes[index] != NULL) {
        if(dhash->nodes[index]->key != NULL && strcmp(dhash->nodes[index]->key, key) != 0) {
            //循环寻找线性探测再散列可以利用的空位
            if(index == dhash->max_size - 1) {
                index = 0;//若遍历到哈希表末尾，重新回到哈希表开头
            }
            else {
                index++;
            }
        }
        //待插入元素已经存在于哈希表中
        else if(dhash->nodes[index]->key != NULL && strcmp(dhash->nodes[index]->key, key) == 0){
            return -2;
        }
    }
    //由于先检查是否需要扩容，因此while循环结束之后必定使index停留在一个空位
    //创建新的节点插入
    dhash->nodes[index] = dhash_node_create(key, value);
    if(dhash->nodes[index] == NULL) {
        return -1;
    }
    dhash->count++;
    return 0;
}

//查找元素
int dhash_node_search(dhash_table_t *dhash, DH_KEY_TYPE key) {
    int index = dhash_function(key, dhash->max_size);
    for (int i = 0; i < dhash->max_size; i++) {
        if(dhash->nodes[index] != NULL) {
            if(strcmp(dhash->nodes[index]->key, key) == 0) {
                break;
            }
            //根据处理冲突的方式，循环向后查找
            if(index == dhash->max_size - 1) {
                index = 0;
            }
            else {
                index++;
            }
        }
    }
    //若已经搜索到空位了，且当前位置键值不等于需要查找的键，未找到
    if(dhash->nodes[index] == NULL || (strcmp(dhash->nodes[index]->key, key) != 0)) {
        return -1;
    }
    else {
        return index;
    }
}

//删除元素
int dhash_node_delete(dhash_table_t *dhash, DH_KEY_TYPE key) {
    //首先查看是否需要缩小哈希表，缩小倍率同扩容倍率，若使用空间小于1/4总容量，缩容，但是又不能小于初始化容量
    if((dhash->count < (dhash->max_size >> 2)) && (dhash->max_size > DHASH_INIT_TABLE_SIZE)) {
        dhash_table_t new_table;
        int ret = dhash_table_init(&new_table, dhash->max_size / DHASH_GROW_FACTOR);
        if(ret != 0) {
            return -1;
        }
        for (int i = 0; i < dhash->max_size; i++) {
            if(dhash->nodes[i] != NULL) {
                ret = dhash_node_insert(&new_table, dhash->nodes[i]->key, dhash->nodes[i]->value);
                if(ret != 0) {
                    return ret;
                }
            }
        }
        //交换表头，并删除原哈希表
        dhash->max_size /= DHASH_GROW_FACTOR;
        dhash->count = new_table.count;
        dhash_node_t **tmp_nodes = new_table.nodes;
        dhash->nodes = new_table.nodes;
        new_table.nodes = tmp_nodes;
        ret = dhash_table_desy(&new_table);
        if(ret != 0) {
            return ret;
        }
    }
    //查找元素
    int index = dhash_node_search(dhash, key);
    if(index < 0) {
        return -2;
    }
    else {
        int ret = dhash_node_desy(dhash->nodes[index]);
        dhash->nodes[index] = NULL;
        dhash->count--;
        return ret;
    }
}

//打印哈希表
int dhash_table_print(dhash_table_t *dhash) {
    if(dhash == NULL) {
        return -1;
    }
    for(int i=0; i<dhash->max_size; i++){
        dhash_node_t* cur_node = dhash->nodes[i];
        if(cur_node != NULL){
            printf("idx %d:", i);
            printf(" key=%s", cur_node->key);
            printf("\n");
        }
    }
    return 0;
}

/*------------函数定义------------*/


/*------------KV函数定义------------*/

//初始化
int kv_dhash_init(kv_dhash_t *kv_addr) {
    if(kv_addr == NULL) {
        return -1;
    }
    return dhash_table_init(kv_addr, DHASH_INIT_TABLE_SIZE);
}

//销毁
int kv_dhash_desy(kv_dhash_t *kv_addr) {
    if(kv_addr == NULL) {
        return -1;
    }
    return dhash_table_desy(kv_addr);
}

//插入指令
int kv_dhash_set(dhash_table_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL || tokens[2] == NULL) {
        return -1;
    }
    return dhash_node_insert(kv_addr, tokens[1], tokens[2]);
}

//查找指令
char *kv_dhash_get(kv_dhash_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL) {
        return NULL;
    }
    int index = dhash_node_search(kv_addr, tokens[1]);
    if(index >= 0) {
        return kv_addr->nodes[index];
    }
    return NULL;
}

//删除指令
int kv_dhash_delete(kv_dhash_t *kv_addr, char **tokens) {
    return dhash_node_delete(kv_addr, tokens[1]);
}

//计数指令
int kv_dhash_count(kv_dhash_t *kv_addr) {
    return kv_addr->count;
}

//存在指令
int kv_dhash_exist(kv_dhash_t *kv_addr, char **tokens) {
    return (dhash_node_search(kv_addr, tokens[1]) >= 0);
}

/*------------KV函数定义------------*/