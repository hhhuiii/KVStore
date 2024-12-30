#ifndef _BTREE_H
#define _BTREE_H

//使用键值对的数据类型：根据使用的键值对类型选择将其中一个置1，另一个置0
#define KV_BTYPE_INT_INT 0//INT KEY; INT VALUE
#define KV_BTYPE_CHAR_CHAR 1//CHAR *KEY; VHAR *VALUE，此类型通过比较字典序使得节点内元素有序

//是否运行测试代码
#define ENABLE_BTREE_DEBUG 0

//键值对结构体
#if KV_BTYPE_INT_INT
typedef int * B_KEY_TYPE;//存储key的数据容器类型
typedef int * B_VALUE_TYPE;//存储value的数据容器类型
typedef int B_KEY_SUB_TYPE;//数据容器中单个key元素的类型
typedef int B_VALUE_SUB_TYPE;//数据容器中单个value元素的类型
#elif KV_BTYPE_CHAR_CHAR
typedef char ** B_KEY_TYPE;//存储key的数据容器类型
typedef char ** B_VALUE_TYPE;//存储value的数据结构类型
typedef char * B_KEY_SUB_TYPE;//数据容器中单个key元素的类型
typedef char * B_VALUE_SUB_TYPE;//数据容器中单个value元素的类型
#endif

//B树节点结构体
typedef struct _btree_node {
    B_KEY_TYPE keys;//存储节点中所有key的数据容器
    B_VALUE_TYPE values;//存储节点中所有value的数据容器
    struct _btree_node **children;//孩子节点指针
    int kv_count;//当前节点的当前键值对数量
    int leaf;//当前节点是否为叶子节点
}btree_node;

//B树结构体
typedef struct _btree {
    int m;//B树的阶数，在初始化时由用户传入
    int all_count;//B树中节点的总数
    struct _btree_node *root_node;//B树根节点
}btree;

typedef btree kv_btree_t;


/*------------KV功能函数声明------------*/
//初始化
int kv_btree_init(kv_btree_t *kv_addr, int m);

//销毁
int kv_btree_desy(kv_btree_t *kv_addr);

//插入指令
int kv_btree_set(kv_btree_t *kv_addr, char **tokens);

//查找指令
char *kv_btree_get(kv_btree_t *kv_addr, char **tokens);

//删除指令
int kv_btree_delete(kv_btree_t *kv_addr, char **tokens);

//计数指令
int kv_btree_count(kv_btree_t *kb_addr);

//存在指令
int kv_btree_exist(kv_btree_t *kv_addr, char **tokens);
/*------------KV功能函数声明------------*/
#endif