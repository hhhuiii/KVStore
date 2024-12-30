#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"

/*------------功能函数声明------------*/
//array遍历查找，返回的是一个KV对
kv_array_item_t *kv_array_search(kv_array_t *kv_addr, const char *key, kv_array_block_t *cur_blk);

//在末尾创建新的存储KV对的内存块，返回创建的块指针
kv_array_block_t *kv_array_create_block(kv_array_t *kv_addr);

//释放给定的块
int kv_array_free_block(kv_array_t *kv_addr, kv_array_block_t *blk);

//找到第一个空节点，若全满就调用块创建函数创建一个新的块
kv_array_item_t *kv_array_find_space(kv_array_t *kv_addr, kv_array_block_t *cur_blk);
/*------------功能函数声明------------*/


/*------------KV功能函数声明------------*/
// 初始化
// 返回值：0成功，-1失败
int kv_array_init(kv_array_t* kv_addr);

// 销毁
// 返回值：0成功，-1失败
int kv_array_desy(kv_array_t* kv_addr);

// 插入指令
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_array_set(kv_array_t* kv_addr, char** tokens);

// 查找指令
char* kv_array_get(kv_array_t* kv_addr, char** tokens);

// 删除指令，另外若当前块为空就释放当前块
// 返回值：0成功，-1失败，-2没有
int kv_array_delete(kv_array_t* kv_addr, char** tokens);

// 计数指令
int kv_array_count(kv_array_t* kv_addr);

// 存在指令
// 返回值：1存在，0没有
int kv_array_exist(kv_array_t* kv_addr, char** tokens);

/*------------KV功能函数声明------------*/


/*------------功能函数定义------------*/
//此处的cur_blk其实在下面的实现中是冗余的，可以尝试用于查找优化
kv_array_item_t *kv_array_search(kv_array_t *kv_addr, const char *key, kv_array_block_t *cur_blk) {
    if(kv_addr == NULL || key == NULL) {
        return NULL;
    }
    //从头部开始查找
    cur_blk = kv_addr->head;
    while(cur_blk != NULL) {
        for(int index = 0; index < kv_array_block_size; index++) {
            //若存储的记录有效且等于key，则找到该记录
            if(cur_blk->items[index].key != NULL && strcmp(cur_blk->items[index].key, key) == 0) {
                return &(cur_blk->items[index]);
            }
        }
        cur_blk = cur_blk->next;
    }
    //遍历所有的块都未找到
    return NULL;
}

//在链表最后创建一个新块
//在kv_addr为NULL时也使用，此情况下创建第一个内存块
kv_array_block_t *kv_array_create_block(kv_array_t *kv_addr) {
    if(kv_addr == NULL) {
        perror("Invalid kv_addr\n");
        return NULL;
    }
    kv_array_block_t *cur_blk = kv_addr->head;
    while(cur_blk != NULL && cur_blk->next != NULL) {
        cur_blk = cur_blk->next;
    }

    kv_array_block_t *end_blk = (kv_array_block_t *)calloc(1, sizeof(kv_array_block_t));
    if(end_blk == NULL) {
        free(end_blk);
        end_blk = NULL;
        perror("end_blk create fail\n");
        return NULL;
    }
    //新分配块的初始化
    end_blk->next = NULL;
    end_blk->count = 0;
    if(kv_addr->head != NULL) {
        cur_blk->next = end_blk;
    }
    else {//即cur_blk为NULL
        kv_addr->head = end_blk;
    }
    return end_blk;
}

//释放给定的块，但至少保持链表中有一个用于存储KV对的块
int kv_array_free_block(kv_array_t *kv_addr, kv_array_block_t *blk) {
    //当前块还有键值对时不能直接删除
    if(blk->count != 0) {
        perror("the block to release still has KVs\n");
        return -1;
    }
    //当前块是第一个块不删除
    if(kv_addr->head == blk) {
        perror("only one block left can't release\n");
        return -1;
    }
    //从头遍历找到当前块的前一个块
    kv_array_block_t *pre_blk = kv_addr->head;
    while(pre_blk->next != blk && pre_blk->next != NULL) {
        pre_blk = pre_blk->next;
    }
    if(pre_blk->next == NULL) {
        return -1;
    }
    pre_blk->next = blk->next;
    if(blk->items) {
        free(blk->items);
        blk->items = NULL;
    }
    blk->next = NULL;
    if(blk) {
        free(blk);
        blk = NULL;
    }
    return 0;
}

//找到一个可以存储KV对的空位，若全满则创建一个新的块
kv_array_item_t *kv_array_find_space(kv_array_t *kv_addr, kv_array_block_t *cur_blk) {
    cur_blk = kv_addr->head;
    //若目前没有用于存储KV对的块,创建一个新块
    if(cur_blk == NULL) {
        cur_blk = kv_array_create_block(kv_addr);
        if(cur_blk == NULL) {
            perror("find space : create new block fail\n");
            return NULL;
        }
        return &(cur_blk->items[0]);
    }
    //从头开始查找
    kv_array_block_t *last_blk = kv_addr->head;//记录cur_blk上一个位置，在需要插入创建的新块时不用再从头遍历
    while(cur_blk != NULL) {
        for(int index = 0; index < kv_array_block_size; index++) {
            if(cur_blk->items[index].key == NULL && cur_blk->items[index].value == NULL) {
                return &(cur_blk->items[index]);
            }
        }
        last_blk = cur_blk;
        cur_blk = cur_blk->next;
    }
    //现有的所有块中都没有空位，创建新块
    cur_blk = kv_array_create_block(kv_addr);
    if(cur_blk == NULL) {
        perror("find space : create new block fail\n");
        return NULL;
    }
    last_blk->next = cur_blk;
    return &(cur_blk->items[0]);
}
/*------------功能函数定义------------*/


/*------------KV功能函数定义------------*/
//初始化array
int kv_array_init(kv_array_t *kv_addr) {
    kv_array_block_t *cur_blk = kv_array_create_block(kv_addr);
    if(cur_blk == NULL) {
        perror("array init : create new block fail\n");
        return -1;
    }
    kv_addr->head = cur_blk;
    kv_addr->count = 0;
    return 0;
}

//销毁array
int kv_array_desy(kv_array_t *kv_addr) {
    kv_array_block_t *cur_blk = kv_addr->head;
    kv_array_block_t *next_blk = cur_blk->next;
    while(cur_blk != NULL) {
        next_blk = cur_blk->next;
        if(cur_blk->items) {
            free(cur_blk->items);
            cur_blk->items = NULL;
        }
        if(cur_blk) {
            free(cur_blk);
            cur_blk = NULL;
        }
        cur_blk = next_blk;
    }
    return 0;
}

//插入指令
//tokens字符指针数组包含指令中的各个字段
int kv_array_set(kv_array_t *kv_addr, char **tokens) {
    //array地址无效或set指令两个参数无效，返回-1
    if(kv_addr == NULL || tokens[1] == NULL || tokens[2] == NULL) {
        perror("command set : invalid parameters\n");
        return -1;
    }
    //若已经存在该key，直接返回
    if(kv_array_search(kv_addr, tokens[1], NULL) != NULL) {
        printf("key already existed!\n");
        return -2;
    }
    //复制key
    char *key_copy = (char *)malloc(strlen(tokens[1]) + 1);//包含字符串末尾的空字符\0
    if(key_copy == NULL) {
        perror("set command : key kcopy fail\n");
        return -1;
    }
    strncpy(key_copy, tokens[1], strlen(tokens[2]) + 1);
    //复制value
    char *value_copy = (char *)malloc(strlen(tokens[2]) + 1);
    if(value_copy == NULL) {
        perror("set command : value kcopy fail\n");
        free(key_copy);//若value分配失败，释放key_copy
        key_copy = NULL;
        return -1;
    }
    strncpy(value_copy, tokens[2], strlen(tokens[2]) + 1);
    //找到array中第一个空KV条目，执行set
    kv_array_block_t blk;
    kv_array_item_t *item = kv_array_find_space(kv_addr, &blk);
    if(item == NULL) {
        perror("set fail : find_space for new KV fail\n");
        return -1;
    }
    item->key = key_copy;
    item->value = value_copy;
    kv_addr->count++;
    //返回的空闲条目所在块指针成为blk
    blk.count++;
    return 0;
}

//查找指令
char *kv_array_get(kv_array_t * kv_addr, char **tokens) {
    kv_array_item_t *item = kv_array_search(kv_addr, tokens[1], NULL);
    if(item != NULL) {
        return item->value;
    }
    else {
        return NULL;
    }
}

//删除指令，若删除KV条目使当前块为空则释放当前块
int kv_array_delete(kv_array_t *kv_addr, char **tokens) {
    kv_array_block_t blk;
    kv_array_item_t *item = kv_array_search(kv_addr, tokens[1], &blk);
    if(item == NULL) {
        printf("key %s not exist\n", tokens[1]);
        return -2;
    }
    else {
        if(item->value) {
            free(item->value);
            item->value = NULL;
        }
        if(item->key) {
            free(item->key);
            item->key = NULL;
        }
        kv_addr->count--;
        blk.count--;//blk作为引用传入函数，其会被修改为指向当前删除条目的块
        //当前块空且当前块不是除头节点外的最后一个块，回收当前块
        if(blk.count == 0 && kv_addr->head != &blk) {
            kv_array_free_block(kv_addr, &blk);
        }
        return 0;
    }
}

//计数指令
int kv_array_count(kv_array_t *kv_addr) {
    return kv_addr->count;
}

//存在指令
int kv_array_exist(kv_array_t *kv_addr, char **tokens) {
    return (kv_array_search(kv_addr, tokens[1], NULL) != NULL);
}
/*------------KV功能函数定义------------*/