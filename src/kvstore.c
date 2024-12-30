#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kvstore.h"

//列举kv存储协议中的所有指令
typedef enum kv_cmd_e {
    KV_CMD_START = 0,//指令起始

    //ARRAY
    KV_CMD_SET = KV_CMD_START,//插入
    KV_CMD_GET,//查找
    KV_CMD_DELETE,//删除
    KV_CMD_COUNT,//计数
    KV_CMD_EXIST,//存在

    //RBTREE
    KV_CMD_RBSET,//插入
    KV_CMD_RBGET,//查找
    KV_CMD_RBDELETE,//删除
    KV_CMD_RBCOUNT,//计数
    KV_CMD_RBEXIST,//存在

    //BTREE
    KV_CMD_BSET,//插入
    KV_CMD_BGET,//查找
    KV_CMD_BDELETE,//删除
    KV_CMD_BCOUNT,//计数
    KV_CMD_BEXIST,//存在

    //SHASH
    KV_CMD_SHSET,//插入
    KV_CMD_SHGET,//查找
    KV_CMD_SHDELETE,//删除
    KV_CMD_SHCOUNT,//计数
    KV_CMD_SHEXIST,//存在

    //DHASH
    KV_CMD_DHSET,//插入
    KV_CMD_DHGET,//查找
    KV_CMD_DHDELETE,//删除
    KV_CMD_DHCOUNT,//计数
    KV_CMD_DHEXIST,//存在

    //SKIPLIST
    KV_CMD_SKSET,//插入
    KV_CMD_SKGET,//查找
    KV_CMD_SKDELETE,//删除
    KV_CMD_SKCOUNT,//计数
    KV_CMD_SKEXIST,//存在

    //CMD
    KV_CMD_ERORR,//指令格式错误
    KV_CMD_QUIT,//退出
    KV_CMD_END,//指令结束
} kv_cmd;

//用户指令结构体
typedef struct kv_user_argc_s {
    const char *cmd;//当前指令
    const int argc;//当前指令包含的参数数量
} kv_user_argc;

//列出用户可以使用的所有指令
//此数组的存储顺序要和上面的枚举类型保持一致
const kv_user_argc KV_COMMAND[] = {
    {"SET", 2},
    {"GET", 1},
    {"DELETE", 1},
    {"COUNT", 0},
    {"EXIST", 1},

    {"RBSET", 2},
    {"RBGET", 1},
    {"RBDELETE", 1},
    {"RBCOUNT", 0},
    {"RBEXIST", 1},

    {"BSET", 2},
    {"BGET", 1},
    {"BDELETE", 1},
    {"BCOUNT", 0},
    {"BEXIST", 1},

    {"SHSET", 2},
    {"SHGET", 1},
    {"SHDELETE", 1},
    {"SHCOUNT", 0},
    {"SHEXIST", 1},

    {"DHSET", 2},
    {"DHGET", 1},
    {"DHDELETE", 1},
    {"DHCOUNT", 0},
    {"DHEXIST", 1},

    {"SKSET", 2},
    {"SKGET", 1},
    {"SKDELETE", 1},
    {"SKCOUNT", 0},
    {"SKEXIST", 1},
};

//枚举给客户端返回信息
typedef enum zv_res_t {
    KV_RES_OK = 0,
    KV_RES_AL_HAVE,
    KV_RES_FAIL,

    KV_RES_NO_KEY,

    KV_RES_TRUE,
    KV_RES_FALSE,

    KV_RES_ERROR,
} zv_res;

//枚举具体返回信息
const char *RES_MSG[] = {
    "OK\r\n",
    "ALREADY HAVE THIS KEY\r\n",
    "TRUE\r\n",
    "FALSE\r\n",
    "ERROR COMMAND\r\n"
};

//初始化kv存储引擎
int kv_engine_init(void) {
    int ret = 0;
    ret += kv_array_init(&kv_array);
    ret += kv_rbtree_init(&kv_rbtree);
    ret += kv_btree_init(&kv_btree, 6);
    ret += kv_shash_init(&kv_shash);
    ret += kv_dhash_init(&kv_dhash);
    ret += kv_skiplist_init(&kv_skiplist, 6);
    return ret;
}

//销毁kv存储引擎
int kv_engine_desy(void) {
    int ret = 0;
    ret += kv_array_desy(&kv_array);
    ret += kv_rbtree_desy(&kv_rbtree);
    ret += kv_btree_desy(&kv_btree);
    ret += kv_shash_desy(&kv_shash);
    ret += kv_dhash_desy(&kv_dhash);
    ret += kv_skiplist_desy(&kv_skiplist);
    return ret;
}

//按照空格拆分用户指令，返回拆分后的指令数量
int kv_split_tokens(char **tokens, char *msg) {
    int count = 0;//解析的指令数量
    char *ptr;
    tokens[count] = strtok_r(msg, " ", &ptr);
    while(tokens[count] != NULL) {
        count++;
        tokens[count] = strtok_r(NULL, " ", &ptr);
    }
    return count--;
}

//解析用户指令，判断用户输入的是哪一个kv_cmd
int kv_parser_cmd(char **tokens, int num_tokens) {
    if(tokens == NULL || tokens[0] == NULL || num_tokens == 0) {
        return KV_CMD_ERORR;
    }
    int index = 0;
    for(index = 0; index < KV_CMD_ERORR; index++) {
        if(strcmp(tokens[0], KV_COMMAND[index].cmd) == 0) {
            if(KV_COMMAND[index].argc == num_tokens - 1) {
                break;
            }
            else {
                return KV_CMD_ERORR;
            }
        }
    }
    return index;//若都不匹配，返回KV_CMD_ERROR
}

//set指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_set(char *buffer, int ret) {
    size_t msg_len = 0;
    //成功
    if(ret == 0) {
        msg_len = strlen(RES_MSG[KV_RES_OK]);
        strncpy(buffer, RES_MSG[KV_RES_OK], msg_len);
    }
    //已经存在
    else if(ret == -2) {
        msg_len = strlen(RES_MSG[KV_RES_AL_HAVE]);
        strncpy(buffer, RES_MSG[KV_RES_AL_HAVE], msg_len);
    }
    //失败
    else {
        msg_len = strlen(RES_MSG[KV_RES_FAIL]);
        strncpy(buffer, RES_MSG[KV_RES_FAIL], msg_len);
    }
    return msg_len;
}

//get指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_get(char *buffer, size_t max_buffer_len, char *value) {
    size_t msg_len = 0;
    if(value == NULL) {
        msg_len = strlen(RES_MSG[KV_RES_NO_KEY]);
        strncpy(buffer, RES_MSG[KV_RES_NO_KEY], msg_len);
    }
    else {
        msg_len = snprintf(buffer, max_buffer_len, "%s\r\n", value);
    }
    return msg_len;
}

//delete指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_delete(char *buffer, int ret) {
    size_t msg_len = 0;
    if(ret == -2) {
        msg_len = strlen(RES_MSG[KV_RES_NO_KEY]);
        strncpy(buffer, RES_MSG[KV_RES_NO_KEY], msg_len);
    }
    else if(ret == 0) {
        msg_len = strlen(RES_MSG[KV_RES_OK]);
        strncpy(buffer, RES_MSG[KV_RES_OK], msg_len);
    }
    else {
        msg_len = strlen(RES_MSG[KV_RES_FAIL]);
        strncpy(buffer, RES_MSG[KV_RES_FAIL], msg_len);
    }
    return msg_len;
}

//count指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_count(char *buffer, int count) {
    size_t msg_len = 0;//返回信息的长度
    char s_cnt[10];
    msg_len = snprintf(s_cnt, sizeof(s_cnt), "%d\r\n", count);
    strncpy(buffer, s_cnt, msg_len);
    return msg_len;
}

//exist指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_exist(char *buffer, int ret) {
    size_t msg_len = 0;
    if(ret == 1) {
        msg_len = strlen(RES_MSG[KV_RES_TRUE]);
        strncpy(buffer, RES_MSG[KV_RES_TRUE], msg_len);
    }
    else if(ret == 0) {
        msg_len = strlen(RES_MSG[KV_RES_FALSE]);
        strncpy(buffer, RES_MSG[KV_RES_FALSE], msg_len);
    }
    else {
        msg_len = strlen(RES_MSG[KV_RES_ERROR]);
        strncpy(buffer, RES_MSG[KV_RES_ERROR], msg_len);
    }
    return msg_len;
}

//实现完整的kv存储引擎
size_t kv_protocol(char *buffer, size_t max_buffer_len) {
    char *tokens[MAX_TOKENS] = {NULL};//用户指令拆分后的指令数组
    int num_tokens = kv_split_tokens(tokens, buffer);//拆分用户指令

    int user_cmd = kv_parser_cmd(tokens, num_tokens);//解析用户指令
    size_t msg_len = 0;//返回缓冲区的有效字符串长度
    switch (user_cmd)
    {
        case KV_CMD_SET:{
            int ret = kv_array_set(&kv_array, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        
        case KV_CMD_GET:{
            char *value = kv_array_get(&kv_array, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }

        case KV_CMD_DELETE:{
            int ret = kv_array_delete(&kv_array, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_COUNT:{
            int count = kv_array_count(&kv_array);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_EXIST:{
            int ret = kv_array_exist(&kv_array, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }

        case KV_CMD_RBSET:{
            int ret = kv_rbtree_set(&kv_rbtree, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }

        case KV_CMD_RBGET:{
            char *value = kv_rbtree_get(&kv_rbtree, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }

        case KV_CMD_RBDELETE:{
            int ret = kv_rbtree_delete(&kv_rbtree, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_RBCOUNT:{
            int count = kv_rbtree_count(&kv_rbtree);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_RBEXIST:{
            int ret = kv_rbtree_exist(&kv_rbtree, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }

        case KV_CMD_BSET:{
            int ret = kv_btree_set(&kv_btree, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }

        case KV_CMD_BGET:{
            char *value = kv_btree_get(&kv_btree, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }

        case KV_CMD_BDELETE:{
            int ret = kv_btree_delete(&kv_btree, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_BCOUNT:{
            int count = kv_btree_count(&kv_btree);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_BEXIST:{
            int ret = kv_btree_exist(&kv_btree, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }

        case KV_CMD_SHSET:{
            int ret = kv_shash_set(&kv_shash, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }

        case KV_CMD_SHGET:{
            char *value = kv_shash_get(&kv_shash, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }

        case KV_CMD_SHDELETE:{
            int ret = kv_shash_delete(&kv_shash, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_SHCOUNT:{
            int count = kv_shash_count(&kv_shash);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_SHEXIST:{
            int ret = kv_shash_exist(&kv_shash, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }

        case KV_CMD_DHSET:{
            int ret = kv_dhash_set(&kv_dhash, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }

        case KV_CMD_DHGET:{
            char *value = kv_dhash_get(&kv_dhash, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }

        case KV_CMD_DHDELETE:{
            int ret = kv_dhash_delete(&kv_dhash, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_DHCOUNT:{
            int count = kv_dhash_count(&kv_dhash);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_DHEXIST:{
            int ret = kv_dhash_exist(&kv_dhash, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }

        case KV_CMD_SKSET:{
            int ret = kv_skiplist_set(&kv_skiplist, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }

        case KV_CMD_SKGET:{
            char *value = kv_skiplist_get(&kv_skiplist, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        
        case KV_CMD_SKDELETE:{
            int ret = kv_skiplist_delete(&kv_skiplist, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }

        case KV_CMD_SKCOUNT:{
            int count = kv_skiplist_count(&kv_skiplist);
            msg_len = kv_setbuffer_count(buffer, count);
            break;
        }

        case KV_CMD_SKEXIST:{
            int ret = kv_skiplist_exist(&kv_skiplist, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        
        case KV_CMD_ERORR:{
            msg_len = strlen(RES_MSG[KV_RES_ERROR]);
            strncpy(buffer, RES_MSG[KV_RES_ERROR], msg_len);
            break;
        }

        default:{
            msg_len = strlen(RES_MSG[KV_RES_ERROR]);
            strncpy(buffer, RES_MSG[KV_RES_ERROR], msg_len);
        }
    }
    return msg_len;
}