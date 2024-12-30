#ifndef _KVSTORE_H
#define _KVSTORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//六种存储引擎的数据结构
#include "array.h"
#include "skiplist.h"
#include "btree.h"
#include "rbtree.h"
#include "shash.h"
#include "dhash.h"

#define MAX_TOKENS 32//用户指令最大的拆分数量

//KV存储引擎
kv_array_t kv_array;
kv_rbtree_t kv_rbtree;
kv_btree_t kv_btree;
kv_skiplist_t kv_skiplist;
kv_shash_t kv_shash;
kv_dhash_t kv_dhash;

//初始化存储引擎
int kv_engine_init(void);

//销毁存储引擎
int kv_engine_desy(void);

//实现kv存储协议
size_t kv_protocol(char *buffer, size_t max_buffer_len);

#endif