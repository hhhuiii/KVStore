#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skiplist.h"

/*------------skiplist操作函数声明------------*/

//创建节点并初始化
//注意还有参数level指示新创建节点位于哪个层级
skiplist_node_t *skiplist_node_create(Z_KEY_TYPE key, Z_VALUE_TYPE value, int level);

//销毁节点
int skiplist_node_desy(skiplist_node_t *node);

//跳表初始化
int skiplist_init(skiplist_t *list, int max_level);

//销毁跳表
int skiplist_desy(skiplist_t *list);

//插入元素
int skiplist_node_insert(skiplist_t *list, Z_KEY_TYPE key, Z_VALUE_TYPE value);

//查找元素
skiplist_node_t *skiplist_node_search(skiplist_t *list, Z_KEY_TYPE key);

//删除元素
int skiplist_node_delete(skiplist_t *list, Z_KEY_TYPE key);

//打印跳表
int skiplist_print(skiplist_t *list);

/*------------skiplist操作函数声明------------*/


/*------------函数定义------------*/
//在所有函数的实现中，只考虑键值对类型为char*和char*类型的实现

//创建一个位于层级level的节点(在某个层创建新节点)
skiplist_node_t *skiplist_node_create(Z_KEY_TYPE key, Z_KEY_TYPE value, int level) {
    if(key == NULL || value == NULL || level <= 0) {//这里的level检查应该还需要小于max_level，或者留到需要创建新节点时再检查
        return NULL;
    }
    //创建节点
    skiplist_node_t *new_node = (skiplist_node_t *)calloc(1, sizeof(skiplist_node_t));
    if(new_node == NULL) {
        return NULL;
    }
    //初始化后继节点指针数组
    new_node->next = (skiplist_node_t **)calloc(level, sizeof(skiplist_node_t *));
    if(new_node->next == NULL) {
        free(new_node);
        new_node = NULL;
        return NULL;
    }
    //指针数组初始化
    for(int i = 0; i < level; i++) {
        new_node->next[i] = NULL;
    }
    //初始化键值对
    char *key_copy = (char *)calloc(strlen(key) + 1, sizeof(char));
    if(key_copy == NULL) {
        skiplist_node_desy(new_node);
        return NULL;
    }
    char *value_copy = (char *)calloc(strlen(value) + 1, sizeof(char));
    if(value_copy == NULL) {
        skiplist_node_desy(new_node);
        return NULL;
    }

    strncpy(key_copy, key, strlen(key) + 1);
    strncpy(value_copy, value, strlen(value));
    new_node->key = key_copy;
    new_node->value = value_copy;

    return new_node;
}

//销毁指定节点
int skiplist_node_desy(skiplist_node_t *node) {
    //已经是空节点
    if(node == NULL) {
        return -1;
    }
    if(node->value) {
        free(node->key);
        node->key = NULL;
    }
    if(node->value) {
        free(node->value);
        node->value = NULL;
    }
    if(node->next) {
        free(node->next);
        node->next = NULL;
    }

    free(node);
    node = NULL;
    return 0;
}

//跳表初始化
//跳表的初始化主要是初始化header指针，跳表的节点可以在使用过程中动态创建
int skiplist_init(skiplist_t *list, int max_level) {
    if(list == NULL || max_level <= 0) {
        return -1;
    }
    list->header = (skiplist_node_t *)calloc(1, sizeof(skiplist_node_t));
    if(list->header == NULL) {
        return -1;
    }
    list->header->next = (skiplist_node_t **)calloc(max_level, sizeof(skiplist_node_t *));
    if(list->header->next == NULL) {
        free(list->header);
        list->header = NULL;
        return -1;
    }
    for(int i = 0; i < max_level; i++) {
        list->header->next[i] = NULL;
    }

    list->header->key = NULL;
    list->header->value = NULL;
    //初始化头节点的其他元素
    list->count = 0;
    list->cur_level = 1;//初始时为一层
    list->max_level = max_level;
    return 0;
}

//销毁跳表
int skiplist_desy(skiplist_t *list) {
    if(list == NULL) {
        return -1;
    }
    skiplist_node_t *cur_node = list->header->next[0];
    skiplist_node_t *next_node = cur_node->next[0];
    while(cur_node != NULL) {
        //调整信息
        //头删法，依次删除header的下一个节点
        for(int i = 0; i < list->max_level; i++) {
            list->header->next[i] = cur_node->next[i];
        }
        list->count--;
        //开始删除
        next_node = cur_node->next[0];//更新next_node为下一个待删除节点
        if(skiplist_node_desy(cur_node) != 0) {//销毁当前正在删除的节点
            return -1;
        }
        cur_node = next_node;//继续删除下一个节点
    }
    //最后删除头节点
    if(list->header) {
        free(list->header->next);
        list->header->next = NULL;
        free(list->header);
        list->header = NULL;
    }
    list->count = 0;
    list->cur_level = 0;
    return 0;
}

//插入元素：若发生冲突，选择头插法
int skiplist_node_insert(skiplist_t *list, Z_KEY_TYPE key, Z_VALUE_TYPE value) {
    //首先寻找新节点应该插入的位置
    skiplist_node_t *update[list->max_level];//搜索插入位置时的查找路径
    skiplist_node_t *pre = list->header;
    /*
        查找的过程：首先从当前跳表所具有的最高层开始搜索
        因为较高层一次next操作可以跳过更多元素
    */
    for(int i = list->cur_level - 1; i >= 0; i--) {
        while(pre->next[i] != NULL && strcmp(pre->next[i]->key, key) < 0) {//若当前层的下一个节点依旧小于key，继续循环
            pre = pre->next[i];
        }
        update[i] = pre;//update存储的是每一层中最后一个小于目标key的节点
    }
    if(pre->next[0] != NULL && strcmp(pre->next[0]->key, key) == 0) {//该key已经存在于跳表中
        return -2;
    }
    else {//可以插入，且已经找到插入位置，插入位置即为pre节点的下一个位置
        int newlevel = 1;
        //确定新节点的层数（以一定概率，保证层数越高，节点越少）
        while ((rand() % 2) && newlevel < list->max_level){//rand() % 2即以不断以0.5的概率增加新节点所在的层数，直到某次rand() % 2取0或者达到最大层数终止循环
            newlevel++;
        }
        //创建待插入的新节点
        skiplist_node_t *new_node = skiplist_node_create(key, value, newlevel);
        if(new_node == NULL) {
            return -1;
        }
        //完善当前层级之上的查找路径
        if(newlevel > list->cur_level) {
            for(int i = list->cur_level; i < newlevel; i++) {
                update[i] = list->header;
            }
            list->cur_level = newlevel;
        }
        //更新新节点的前后指向
        for(int i = 0; i < newlevel; i++) {
            new_node->next[i] = update[i]->next[i];
            update[i]->next[i] = new_node;
        }
        list->count++;
        return 0;
    }
}

//查找元素，在插入新元素函数中亦有实现
skiplist_node_t *skiplist_node_search(skiplist_t *list, Z_KEY_TYPE key) {
    //从头节点开始找
    skiplist_node_t *pre = list->header;
    for(int i = list->cur_level - 1; i >= 0; i--) {
        while(pre->next[i] != NULL && strcmp(pre->next[i]->key, key) < 0) {
            pre = pre->next[i];
        }
    }
    //判断下一个元素是否是当前key
    if(pre->next[0] != NULL && strcmp(pre->next[0]->key, key) == 0) {
        return pre->next[0];
    }
    return NULL;
}

//删除元素
int skiplist_node_delete(skiplist_t *list, Z_KEY_TYPE key) {
    skiplist_node_t *update[list->max_level];
    skiplist_node_t *pre = list->header;
    //首先进行查找操作
    for(int i = list->cur_level - 1; i >= 0; i--){
        while(pre->next[i] != NULL && strcmp(pre->next[i]->key, key) < 0) {
            pre = pre->next[i];
        }
        update[i] = pre;
    }

    //待删除节点存在于跳表中，删除节点并更新指向信息
    if(pre->next[0] != NULL && strcmp(pre->next[0]->key, key) == 0) {
        skiplist_node_t *node_del = pre->next[0];//node_del记载待删除元素
        for(int i = 0; i < list->cur_level; i++) {//更新指向信息
            if(update[i]->next[i] == node_del) {//只要记载的指针指向待删除节点，就将其指向待删除节点的下一个节点
                update[i]->next[i] = node_del->next[i];
            }
        }
        int ret = skiplist_node_desy(node_del);//销毁待删除节点
        if(ret == 0) {//销毁成功
            list->count--;
            for(int i = 0; i < list->max_level; i++) {//不断更新当前的最大层数
                if(list->header->next[i] == NULL) {
                    list->cur_level = i;
                    break;
                }
            }
        }
        return ret;
    }
    //待删除节点不存在于跳表中
    else {
        return -2;
    }
}

//打印跳表
//从最高层开始打印，打印每一层的所有节点
int skiplist_print(skiplist_t *list) {
    if(list == NULL) {
        return -1;
    }
    skiplist_node_t *pre;
    for(int i = list->cur_level; i >= 0; i--) {
        printf("level : %d", i);
        pre = list->header->next[i];
        while(pre != NULL) {
            printf(" %s", pre->key);
            pre = pre->next[i];
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}



//KV初始化
int kv_skiplist_init(kv_skiplist_t *kv_addr, int m) {
    if(kv_addr == NULL || m <= 0) {
        return -1;
    }
    return skiplist_init(kv_addr, m);
}

//销毁
int kv_skiplist_desy(kv_skiplist_t *kv_addr) {
    if(kv_addr == NULL) {
        return -1;
    }
    return skiplist_desy(kv_addr);
}

//插入指令
int kv_skiplist_set(kv_skiplist_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL || tokens[2] == NULL) {
        return -1;
    }
    return skiplist_node_insert(kv_addr, tokens[1], tokens[2]);
}

//查找指令
char *kv_skiplist_get(kv_skiplist_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL) {
        return NULL;
    }
    skiplist_node_t *node = skiplist_node_search(kv_addr, tokens[1]);
    if(node != NULL) {
        return node->value;
    }
    return NULL;
}

//删除指令
int kv_skiplist_delete(kv_skiplist_t *kv_addr, char **tokens) {
    if(kv_addr == NULL || tokens == NULL || tokens[1] == NULL) {
        return -1;
    }
    return skiplist_node_delete(kv_addr, tokens[1]);
}

//计数指令
int kv_skiplist_count(kv_skiplist_t *kv_addr) {
    return kv_addr->count;
}

//存在指令
int kv_skiplist_exist(kv_skiplist_t *kv_addr, char **tokens) {
    return (skiplist_node_search(kv_addr, tokens[1]) != NULL);
}

/*------------函数定义------------*/