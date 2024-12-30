#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "btree.h"


/*------------Btree操作和KV协议函数声明------------*/

/*------初始化分配内存------*/

//创建单个节点，leaf参数指示是否为叶子节点
btree_node *btree_node_create(btree *T, int leaf);

//初始化m阶B树
int btree_init(int m, btree *T);

/*------初始化分配内存------*/


/*------释放内存------*/

//删除单个节点
void btree_node_destroy(btree_node *cur);

//递归删除给定节点作为根节点的子树
void btree_node_destroy_recurse(btree_node *cur);

//删除所有节点，释放内存
int btree_destroy(btree *T);

/*------释放内存------*/


/*------插入------*/

//根节点分裂
btree_node *btree_root_split(btree *T);

//索引为idx的孩子节点分裂
btree_node *btree_child_split(btree *T, btree_node *cur, int idx);

//插入元素
int btree_insert_key(btree *T, B_KEY_SUB_TYPE key, B_VALUE_SUB_TYPE value);

/*------插入------*/


/*------删除------*/
//删除导致当前节点元素不足时，若相邻节点的元素充足则借位，不重组则合并
//借位：将cur节点的idx_key元素下放到idx_dest孩子
btree_node *btree_borrow(btree_node *cur, int idx_key, int idx_dest);

//合并
btree_node *btree_merge(btree *T, btree_node *cur, int idx);

//找出当前节点索引为idx_key的元素的前驱节点
btree_node *btree_precursor_node(btree *T, btree_node *cur, int idx_key);

//找出当前节点索引为idx_key的元素的后继节点
btree_node *btree_successor_node(btree *T, btree_node *cur, int idx_key);

//删除元素
int btree_delete_key(btree *T, B_KEY_SUB_TYPE key);

/*------删除------*/


/*------查找------*/

//查找key
btree_node *btree_search_key(btree *T, B_KEY_SUB_TYPE key);

/*------查找------*/


/*------输出B树信息------*/

//打印当前节点信息
void btree_node_print(btree_node *cur);

//先序遍历给定节点为根节点的子树（递归实现）
void btree_traversal_node(btree *T, btree_node *cur);

//btree遍历
void btree_traversal(btree *T);

/*------输出B树信息------*/


/*------检查有效性------*/

//获取B树高度
int btree_depth(btree *T);

/*------检查有效性------*/

/*------------Btree操作和KV协议函数声明------------*/


/*------------Btree操作函数定义------------*/

//创建单个节点，leaf参数表示是否为叶子节点
btree_node *btree_node_create(btree *T, int leaf) {
    btree_node *new = (btree_node *)malloc(sizeof(btree_node));
    if(new == NULL) {
        perror("btree_node_create : malloc new btree node fail\n");
        return NULL;
    }

    //calloc分配每个字节都会初始化为0
    //键值数量不超过m-1，一次性分配连续空间用于存储键值
    new->keys = (B_KEY_TYPE)calloc(T->m - 1, sizeof(B_KEY_SUB_TYPE));
    if(new->keys == NULL) {
        perror("btree_node_create : keys calloc fail\n");
        free(new);
        new = NULL;
        return NULL;
    }

    //由于key-value是成对的作为B树节点的元素，因此value数量也是不超过m-1
    new->values = (B_VALUE_TYPE)calloc(T->m - 1, sizeof(B_VALUE_SUB_TYPE));
    if(new->values == NULL) {
        perror("btree_node_create : values calloc fail\n");
        free(new->keys);
        new->keys = NULL;
        free(new);
        new = NULL;
        return NULL;
    }
    
    //分配指向最多m个子树的指针数组
    new->children = (btree_node **)calloc(T->m, sizeof(btree_node *));
    if(new->children == NULL) {
        perror("btree_node_create : children calloc fail\n");
        free(new->values);
        new->values = NULL;
        free(new->keys);
        new->keys = NULL;
        free(new);
        new = NULL;
        return NULL;
    }

    new->kv_count = 0;
    new->leaf = leaf;
    return new;
}

//初始化m阶B树
int btree_init(int m, btree *T) {
    T->m = m;
    T->root_node = NULL;
    T->all_count = 0;
    return 0;
}

//删除单个节点
void btree_node_destroy(btree_node *cur) {
    if(cur->keys) {
        free(cur->keys);
        cur->keys = NULL;
    }
    if(cur->values) {
        free(cur->values);
        cur->values = NULL;
    }
    if(cur->children) {
        free(cur->children);
        cur->children = NULL;
    }
    if(cur) {
        free(cur);
        cur = NULL;
    }
}

//删除给定节点作为根节点的子树(递归实现)
void btree_node_destroy_recurse(btree_node *cur) {
    //若当前节点为叶节点，直接删除自身即可
    if(cur->leaf == 1) {
        btree_node_destroy(cur);
    }
    //若非叶节点，则递归删除所有孩子节点
    else {
        for(int i = 0; i < cur->kv_count + 1; i++) {
            btree_node_destroy_recurse(cur->children[i]);
        }
    }
}

//释放btree内存(递归实现)
int btree_destroy(btree *T) {
    if(T) {
        //删除所有节点
        if(T->root_node != NULL) {
            btree_node_destroy_recurse(T->root_node);
        }
        //删除btree
        free(T);
        T = NULL;
    }
    return 0;
}

//根节点分裂
btree_node *btree_root_split(btree *T) {
    //创建兄弟节点
    btree_node *brother = btree_node_create(T, T->root_node->leaf);
    //注意根节点只有满了才会分裂，所以不要奇怪下面有些地方直接用T->m而不是root_node的kv_count
    int i = 0;
    for(i = 0; i < ((T->m - 1) >> 1); i++){//取所有关键字的中间靠左的元素作为分割，分割元素放入根节点，其他两部分放入两个子节点
#if KV_BTYPE_INT_INT
        brother->keys[i] = T->root_node->keys[i + (T->m>>1)];
        T->root_node->keys[i + (T->m>>1)] = 0;
#elif KV_BTYPE_CHAR_CHAR
        //复制原根节点的右半部分键值对（不包含处于分割位置的键值对）
        brother->keys[i] = T->root_node->keys[i + (T->m >> 1)];
        brother->values[i] = T->root_node->values[i + (T->m >> 1)];
        //将原根节点右半部分键值对置空
        T->root_node->keys[i + (T->m >> 1)] = NULL;
        T->root_node->values[i + (T->m >> 1)] = NULL;
#endif
        //分配孩子指针数组的右半部分给新节点
        brother->children[i] = T->root_node->children[i + (T->m >> 1)];
        //原根节点孩子指针数组右半部分置空
        T->root_node->children[i + (T->m >> 1)] = NULL;
        //更新当前拥有的键值对数目
        brother->kv_count++;
        T->root_node->kv_count--;
    }
    //原根节点中剩余最后一个指针还需要复制操作（原因来自for循环的终止条件）
    brother->children[brother->kv_count] = T->root_node->children[T->m - 1];
    T->root_node->children[T->m - 1] = NULL;

    //创建新的根节点
    btree_node *new_root = btree_node_create(T, 0);
#if KV_BTYPE_INT_INT
    new_root->keys[0] = T->root_node->keys[T->root_node->kv_count - 1];
    T->root_node->keys[T->root_node->kv_count - 1] = 0;
#elif KV_BTYPE_CHAR_CHAR
    //取分裂后的原根节点的最后一个元素作为新的根节点
    new_root->keys[0] = T->root_node->keys[T->root_node->kv_count - 1];
    T->root_node->keys[T->root_node->kv_count - 1] = NULL;
    T->root_node->values[T->root_node->kv_count - 1] = NULL;
#endif
    
    T->root_node->kv_count--;
    new_root->kv_count = 1;
    new_root->children[0] = T->root_node;
    new_root->children[1] = brother;
    T->root_node = new_root;

    return T->root_node;
}

//索引为idx的孩子节点分裂，idx为cur的孩子数组索引（指示需要进行分裂的孩子）
btree_node *btree_child_split(btree *T, btree_node *cur, int idx) {
    //创建孩子的兄弟节点，节点的分裂操作同根节点分裂操作
    btree_node *full_child = cur->children[idx];
    btree_node *new_child = btree_node_create(T, cur->children[idx]->leaf);
    int i = 0;
    for(i = 0; i < ((T->m - 1) >> 1); i++) {

#if KV_BTYPE_INT_INT
        new_child->keys[i] = full_child->keys[i + (T->m >> 1)];
        full_child->keys[i + (T->m >> 1)] = 0;
#elif KV_BTYPE_CHAR_CHAR
        //将full_child的右半部分分配给new_child
        new_child->keys[i] = full_child->keys[i + (T->m >> 1)];
        new_child->values[i] = full_child->values[i + (T->m >> 1)];
        full_child->keys[i + (T->m >> 1)] = NULL;
        full_child->values[i + (T->m >> 1)] = NULL;
#endif
        //分配孩子数组中的指针
        new_child->children[i] = full_child->children[i + (T->m >> 1)];
        full_child->children[i + (T->m >> 1)] = NULL;
        new_child->kv_count++;
        full_child->kv_count--;
    }
    //循环条件限制，单独复制最后一个孩子指针
    new_child->children[new_child->kv_count] = full_child->children[T->m - 1];
    full_child->children[T->m - 1] = NULL;

    //将cur的第idx+1到最后一个元素全部向后移，以容纳分裂得到的新节点new_child
    for(i = cur->kv_count; i > idx; i--) {
        cur->keys[i] = cur->keys[i - 1];
        cur->values[i] = cur->values[i - 1];
        cur->children[i] = cur->children[i - 1];
    }
    cur->children[idx + 1] = new_child;
    //将被分裂孩子节点中最大的元移动到父节点，注意节点中关键字的索引大小等于其左边孩子的索引大小
    cur->keys[idx] = full_child->keys[full_child->kv_count - 1];
    cur->values[idx] = full_child->values[full_child->kv_count - 1];

#if KV_BTYPE_INT_INT
    full_child->keys[full_child->kv_count - 1] = NULL;
    full_child->values[full_child->kv_count - 1] = NULL;
#elif KV_BTYPE_CHAR_CHAR
    //更新键值对计数
    cur->kv_count++;
    full_child->kv_count--;
#endif

    return cur;
}

//插入元素，先判断是否需要分裂，插入操作必然发生在叶子节点
int btree_insert_key(btree *T, B_KEY_SUB_TYPE key, B_VALUE_SUB_TYPE value) {
    btree_node *cur = T->root_node;
    //判断输入是否合法
#if KV_BTYPE_INT_INT
    if(key <= 0) {
        return -1;
    }
#elif KV_BTYPE_CHAR_CHAR
    if(key == NULL || value == NULL) {
        perror("btree_insert_key : Invalid input\n");
        return -1;
    }
#endif
    //向空的B树插入元素，首先创建B树
    if(cur == NULL) {
        btree_node *new = btree_node_create(T, 1);
#if KV_BTYPE_INT_INT
        new->keys[0] = key;
        new->values[0] = value;
#elif KV_BTYPE_CHAR_CHAR
        //由于传入的是指针类型，进行参数赋值操作
        //拷贝操作使B树拥有自己的独立拷贝，保证安全性和完整性
        char *key_copy = (char *)malloc(strlen(key) + 1);
        if(key_copy == NULL) {
            perror("key_copy : malloc fail\n");
            return -1;
        }
        strncpy(key_copy, key, strlen(key) + 1);
        char *value_copy = (char *)malloc(strlen(value) + 1);
        if(value_copy == NULL) {
            perror("value_copy : malloc fail\n");
            free(key_copy);
            key_copy = NULL;
            return -1;
        }
        strncpy(value_copy, value, str(value) + 1);
        new->keys[0] = key_copy;
        new->values[0] = value_copy;
#endif
        new->kv_count = 1;
        T->root_node = new;
        T->all_count++;
    }
    //向非空B树插入元素，涉及分裂检查和可能的分裂操作
    //从根节点逐步找到元素要插入的叶子节点，若插入会引起分裂，先分裂，再插入元素
    //按概念是应该先插入后分裂，但实操无法操作，通过先分裂后插入的操作实现先插入后分裂的效果
    else {
        //根节点已满，执行提前分裂操作
        if(cur->kv_count == T->m - 1) {
            cur = btree_root_split(T);//执行根节点分裂操作，cur接收返回的新根节点
        }
        //从根节点开始寻找要插入的叶子节点，在查找路径上的非叶节点都首先进行检查，是否已满，若已满则提前分裂
        while(cur->leaf == 0) {//直到遍历到叶子节点循环终止
            int next_index = 0;//指示当前应当插入的孩子索引
            int i = 0;
            for(i = 0; i < cur->kv_count; i++) {
#if KV_BTYPE_INT_INT
                if(key == cur->keys[i]) {
                    return -2;
                }
                else if(key < cur->keys[i]) {
                    next_index = i;
                    break;
                }
                else if(i == cur->kv_count - 1) {
                    next_index = cur->kv_count;
                }
#elif KV_BTYPE_CHAR_CHAR
                if(strcmp(key, cur->keys[i]) == 0) {
                    printf("Insert key:%s - value:%s : already existed\n", key, value);
                    return -2;
                }
                else if(strcmp(key, cur->keys[i]) < 0) {//key小于cur->keys[i]，应当插入该键的左子树中
                    next_index = i;
                    break;
                }
                else if(i == cur->kv_count - 1) {//如果所有键都小于key，应当插入末尾即索引为kv_count处
                    next_index = cur->kv_count;
                }
#endif
            }
            //检查孩子节点是否需要分裂，若满，执行提前分裂
            if(cur->children[next_index]->kv_count == T->m - 1) {
                cur = btree_child_split(T, cur, next_index);//当前节点的索引为next_index的孩子节点需要分裂
            }
            //若未满，继续向叶节点搜索下一层
            else {
                cur = cur->children[next_index];
            }
        }
        //循环结束后，已经到达叶节点，从根节点到叶节点的路径上的节点已满的已经提前分裂，接下来就是将新的键值对插入到叶节点中
        //将新元素插入到叶子节点
        int i = 0;
        int pos = 0;//指示新的键值对应该插入的位置索引
        for (i = 0; i < cur->kv_count; i++) {
#if KV_BTYPE_INT_INT
            if(key == cur->keys[i]) {
                printf("Insert key:%s - value:%s : already existed\n", key, value);
                return -2;
            }
            else if(key < cur->keys[i]) {
                pos = i;
                break;
            }
            else if(i == cur->kv_count - 1) {
                pos = cur->kv_count;
            }
#elif KV_BTYPE_CHAR_CHAR
            if(strcmp(key, cur->keys[i]) == 0) {
                printf("Insert key:%s - value:%s : already existed\n", key, value);
                return -2;
            }
            else if(strcmp(key, cur->keys[i]) < 0) {
                pos = i;
                break;
            }
            else if(i == cur->kv_count - 1) {
                pos = cur->kv_count;//必定有空位插入，因为在遍历到当前叶节点的父节点时已经检查过该叶节点是否已满，若没有空位，将会进行提前分裂
            }
#endif
        }
        //插入元素，只有当key-value为char *_char *类型时候需要进行参数独立拷贝
#if KV_BTYPE_CHAR_CHAR
        char *key_copy = (char *)malloc(strlen(key) + 1);
        if(key_copy == NULL) {
            perror("key_copy : malloc failed\n");
            return -1;
        }
        strncpy(key_copy, key, strlen(key) + 1);

        char *value_copy = (char *)malloc(strlen(value) + 1);
        if(value_copy == NULL) {
            perror("value_copy : malloc failed\n");
            free(key_copy);
            key_copy = NULL;
            return -1;
        }
        strncpy(value_copy, value, strlen(value) + 1);
#endif
        if(pos == cur->kv_count) {//若插入末尾，不需要移动元素
#if KV_BTYPE_INT_INT
            cur->keys[cur->kv_count] = key;
            cur->values[cur->kv_count] = value;
#elif KV_BTYPE_CHAR_CHAR
            cur->keys[cur->kv_count] = key_copy;
            cur->values[cur->kv_count] = value_copy;
#endif
        }
        else {//若在中间插入，将其后元素依次后移
            for(i = cur->kv_count; i > pos; i--) {
                cur->keys[i] = cur->keys[i - 1];
                cur->values[i] = cur->values[i - 1];
            }
#if KV_BTYPE_INT_INT
            cur->keys[pos] = key;
            cur->values[pos] = value;
#elif KV_BTYPE_CHAR_CHAR
            cur->keys[pos] = key_copy;
            cur->values[pos] = value_copy;
#endif
        }
        T->all_count++;
        cur->kv_count++;
    }
    return 0;
}

//借位，将cur节点的idx_key元素下放到idx_dest孩子，cur为发生借位的连个节点的父节点
btree_node *btree_borrow(btree_node *cur, int idx_key, int idx_dest) {
    //因为操作是将idx_key元素下放到idx_dest孩子，因此idx_dest孩子要么是下放元素的左子树，要么是其右子树
    //若发生借位节点为cur中下放的元素的左子树，选择下放元素的右子树（因为其左子树存在，则其右子树必定存在）作为借位源节点
    //若发生借位节点为cur中下放的元素的右子树，选择下放元素的左子树作为借位节点
    int idx_sour = (idx_key == idx_dest) ? idx_dest + 1 : idx_key;
    btree_node *node_dest = cur->children[idx_dest];//目的节点：借位的孩子节点
    btree_node *node_sour = cur->children[idx_sour];//源节点：被借位的孩子节点
    if(idx_key == idx_dest) {//借位的孩子节点是cur的左子树
        //先将idx_key索引的元素下放到node_dest
        node_dest->keys[node_dest->kv_count] = cur->keys[idx_key];
        node_dest->values[node_dest->kv_count] = cur->values[idx_key];
        //注意孩子的转移操作：源节点的第一个孩子将被转移为目的节点增加的元素的右孩子，因此索引为node_dest->kv_count + 1
        node_dest->children[node_dest->kv_count + 1] = node_sour->children[0];
        node_dest->kv_count++;
        //将右子树的第一个元素上传到cur节点中替代被下放的元素
        cur->keys[idx_key] = node_sour->keys[0];
        cur->values[idx_key] = node_sour->values[0];
        //由于索引为0的元素被取走，将源节点中后面的元素依次向前移
        for(int i = 0; i < node_sour->kv_count - 1; i++) {
            node_sour->keys[i] = node_sour->keys[i + 1];
            node_sour->values[i] = node_sour->values[i + 1];
            node_sour->children[i] = node_sour->children[i + 1];
        }
        //孩子指针比节点中元素多一个，需要单独移动多出来的一个孩子指针
        node_sour->children[node_sour->kv_count - 1] = node_sour->children[node_sour->kv_count];
#if KV_BTYPE_INT_INT
        node_sour->keys[node_sour->kv_count - 1] = 0;
#elif KV_BTYPE_CHAR_CHAR
        //移动元素后的清理操作
        node_sour->keys[node_sour->kv_count - 1] = NULL;
        node_sour->values[node_sour->kv_count - 1] = NULL;
        node_sour->children[node_sour->kv_count] = NULL;
#endif
        node_sour->kv_count--;
    }
    else {//借位的孩子节点是idx_key索引的元素的右子树
        //cur下放的元素将放在idx_dest节点的第一个位置，将所有元素和孩子右移
        //孩子指针比元素多一个首先单独操作最右边的孩子指针
        node_dest->children[node_dest->kv_count + 1] = node_dest->children[node_dest->kv_count];
        //右移操作，一定有空位，因为元素不足才发生的借位
        for(int i = node_dest->kv_count; i > 0; i--) {
            node_dest->keys[i] = node_dest->keys[i - 1];
            node_dest->values[i] = node_dest->values[i - 1];
            node_dest->children[i] = node_dest->children[i - 1];
        }
        //将idx_key索引的元素下放到idx_dest孩子
        node_dest->keys[0] = cur->keys[idx_key];
        node_dest->values[0] = cur->values[idx_key];
        //将源节点的最后一个孩子指针转为目的节点的第一个孩子指针
        node_dest->children[0] = node_sour->children[node_sour->kv_count];
        node_dest->kv_count++;
        //将源节点的最后一个孩子上交给cur节点
        cur->keys[idx_key] = node_sour->keys[node_sour->kv_count - 1];
        cur->values[idx_key] = node_sour->values[node_sour->kv_count - 1];
#if KV_BTYPE_INT_INT
        node_sour->keys[node_sour->kv_count - 1] = 0;
#elif KV_BTYPE_CHAR_CHAR
        //源节点元素上交和孩子指针转移后的清理
        node_sour->keys[node_sour->kv_count - 1] = NULL;
        node_sour->values[node_sour->kv_count - 1] = NULL;
        node_sour->children[node_sour->kv_count] = NULL;
#endif
        node_sour->kv_count--;
    }
    return node_dest;//返回的是完成借位的目的节点
}

//合并：将cur节点的idx元素下放合并该元素的左右子树
btree_node *btree_merge(btree *T, btree_node *cur, int idx) {
    btree_node *left = cur->children[idx];//cur中索引为idx的元素的左子树
    btree_node *right = cur->children[idx + 1];//cur中索引为idx的元素的右子树
    //首先将cur中索引为idx的元素下放到left，由于元素都不足引发的合并，因此left必有空位接收
    left->keys[left->kv_count] = cur->keys[idx];
    left->values[left->kv_count] = cur->values[idx];
    left->kv_count++;
    //cur中索引为idx的元素下放后，将后面的元素向前移填补空位
    for(int i = idx; i < cur->kv_count - 1; i++) {
        cur->keys[i] = cur->keys[i + 1];
        cur->values[i] = cur->values[i + 1];
        //idx索引的元素的右孩子将合并入左孩子，直接覆盖该孩子指针
        cur->children[i + 1] = cur->children[i + 2];
    }
#if KV_BTYPE_INT_INT
    cur->keys[cur->kv_count - 1] = 0;
    cur->values[cur->kv_count - 1] = 0;
#elif KV_BTYPE_CHAR_CHAR
    //cur节点元素左边移后的清理操作
    cur->keys[cur->kv_count - 1] = NULL;
    cur->values[cur->kv_count - 1] = NULL;
#endif
    cur->children[cur->kv_count] = NULL;
    cur->kv_count--;
    //将右孩子的所有元素并入左孩子
    //合并一定不会超出容量，因为两者都是因为元素不充足才被合并
    for(int i = 0; i < right->kv_count; i++) {
        left->keys[left->kv_count] = right->keys[i];
        left->values[left->kv_count] = right->values[i];
        left->children[left->kv_count] = right->children[i];
        left->kv_count++;
    }
    //多出一个孩子指针，单独操作
    left->children[left->kv_count] = right->children[right->kv_count];
    //销毁右孩子
    btree_node_destroy(right);
    //若cur节点为B树的根节点且当前为空，销毁
    if(T->root_node == cur && cur->kv_count == 0) {
        btree_node_destroy(cur);
        T->root_node = NULL;
    }
    //若原来的根节点被销毁，返回的left将作为新的根节点
    return left;//返回合并后的节点指针
}

//找出当前节点索引为idx_key的元素的前驱节点
btree_node *btree_precursor_node(btree *T, btree_node *cur, int idx_key) {
    if(cur->leaf == 0) {
        //搜索cur的左子树
        cur = cur->children[idx_key];
        //左子树最右下的节点就是其前驱节点（第一个比idx_key索引的元素小的元素所在的节点）
        while(cur->leaf == 0) {
            cur = cur->children[cur->kv_count];
        }
    }
    return cur;//返回该前驱节点
}

//找出当前节点索引为idx_key的元素的后继节点
btree_node *btree_successor_node(btree *T, btree_node *cur, int idx_key) {
    //若cur非叶节点
    if(cur->leaf == 0) {
        //搜索cur的右子树
        cur = cur->children[idx_key + 1];
        //右子树最左下的节点就是其后继节点（第一个比idx_key索引的元素大的元素所在节点）
        while(cur->leaf == 0) {
            cur = cur->children[0];
        }
    }
    return cur;//返回该后继节点
}

//删除元素（key所指示的键值对），检查是否需要先合并或借位，再删除，删除必定发生在叶子节点
int btree_delete_key(btree *T, B_KEY_SUB_TYPE key) {
#if KV_BTYPE_INT_INT
    if(T->root_node != NULL && key > 0)
#elif KV_BTYPE_CHAR_CHAR
    //树非空，键有效
    if(T->root_node != NULL && key != NULL)
#endif
    {
        btree_node *cur = T->root_node;
        //当未搜索到叶节点时循环
        while(cur->leaf == 0) {
            int idx_next = 0;//下一步应该搜索的孩子指针的索引
            int idx_bro = 0;//下一步搜索的孩子的兄弟节点的索引（动态选择：可能成为被借位或合并的对象）
            int idx_key = 0;
#if KV_BTYPE_INT_INT
            if(key < cur->keys[0])
#elif KV_BTYPE_CHAR_CHAR
            //边界1：比当前节点中最小的元素还小
            if(strcmp(key, cur->keys[0]) < 0)
#endif
            {
                //下一步搜索cur->keys[0]的左子树
                idx_next = 0;
                idx_bro = 1;//只能选择右兄弟
            }
#if KV_BTYPE_INT_INT
            else if(key > cur->keys[cur->num - 1])
#elif KV_BTYPE_CHAR_CHAR
            //边界2：比当前节点中最大的元素还大
            else if(strcmp(key, cur->keys[cur->kv_count - 1]) > 0)
#endif
            {
                //下一步搜索keys[cur->kv_count]的右子树
                idx_next = cur->kv_count;
                idx_bro = cur->kv_count - 1;//只能选择左兄弟
            }
            //非边界情况：节点中存在一个元素恰好比它小，另一个元素恰好比它大，首先找到该位置
            //此时兄弟节点的选择需要视情况考虑
            else {
                //
                for(int i = 0; i < cur->kv_count; i++) {
#if KV_BTYPE_INT_INT
                    if(key == cur->keys[i])
#elif KV_BTYPE_CHAR_CHAR
                    if(strcmp(key, cur->keys[i]) == 0)//要删除的元素在非叶节点中找到，不可直接删除，终止搜索，转换为删除叶节点上的元素
#endif
                    {
                        //选择向元素数量更少的孩子节点搜索删除对象，并让元素数量更多的节点成为兄弟节点
                        //选择向元素数量更少的孩子节点搜索，可能触发借位操作，如此能够顺便将父节点中待删除元素转移到子节点
                        if(cur->children[i]->kv_count <= cur->children[i + 1]->kv_count) {
                            idx_next = i;
                            idx_bro = i + 1;
                        }
                        else {
                            idx_next = i + 1;
                            idx_bro = i;
                        }
                        break;
                    }
#if KV_BTYPE_INT_INT
                    else if((i < cur->kv_count - 1) && (key > cur->keys[i]) && (key < cur->keys[i + 1]))
#elif KV_BTYPE_CHAR_CHAR
                    else if((i < cur->kv_count - 1) && (strcmp(key, cur->keys[i]) > 0) && (strcmp(key, cur->keys[i + 1]) < 0))//要删除的元素不再当前节点，继续向叶节点搜索
#endif
                    {
                        idx_next = i + 1;//只能向child[i + 1]孩子搜索，此时可以选择children[i]或children[i + 2]作为兄弟节点
                        //选择节点更多的作为兄弟，降低借位或合并操作的可能性
                        if(cur->children[i]->kv_count > cur->children[i + 2]->kv_count) {
                            idx_bro = i;
                        }
                        else {
                            idx_bro = i + 2;
                        }
                        break;
                    }
                }
            }
            //idx_key是以idx_next和idx_bro为左右子树的元素索引
            idx_key = (idx_next < idx_bro) ? idx_next : idx_bro;
            //下一步要搜索的子节点处于最少元素状态
            if(cur->children[idx_next]->kv_count <= ((T->m >> 1) - 1)) {
                //若兄弟节点够借，借位
                if(cur->children[idx_bro]->kv_count >= (T->m >> 1)) {
                    cur = btree_borrow(cur, idx_key, idx_next);
                }
                //若兄弟节点不够借，合并
                else {
                    cur = btree_merge(T, cur, idx_key);
                }
            }
#if KV_BTYPE_INT_INT
            else if(cur->keys[idx_key] == key)
#elif KV_BTYPE_CHAR_CHAR
            //在cur中找到待删除元素
            else if(strcmp(cur->keys[idx_key], key) == 0)
#endif
            {
                btree_node *pre;
                B_KEY_SUB_TYPE tmp;
                //若在左子树中找到待删除的元素，不能直接删除，而是需要将其逐步送到叶子节点
                //逐步送到叶子节点不可以采取借位的方式，而是找到前驱后者后继元素进行替换
                if(idx_key == idx_next) {
                    pre = btree_precursor_node(T, cur, idx_key);
                    //找到idx_key索引的待删除元素的前驱节点，与待删除元素替换
                    tmp = pre->keys[pre->kv_count - 1];
                    pre->keys[pre->kv_count - 1] = cur->keys[idx_key];
                    cur->keys[idx_key] = tmp;

                    tmp = pre->values[pre->kv_count - 1];
                    pre->values[pre->kv_count - 1] = cur->values[idx_key];
                    cur->values[idx_key] = tmp;
                }
                //若在右子树中找到待删除的元素
                else {
                    pre = btree_successor_node(T, cur, idx_key);
                    //找到idx_key索引的待删除元素的后继节点，与待删除元素替换
                    tmp = pre->keys[0];
                    pre->keys[0] = cur->keys[idx_key];
                    cur->keys[idx_key] = tmp;
                    
                    tmp = pre->values[0];
                    pre->values[0] = cur->values[idx_key];
                    cur->values[idx_key] = tmp;
                }
                cur = cur->children[idx_next];
            }
            //若依旧没找到待删除元素，继续向子树搜索
            else {
                cur = cur->children[idx_next];
            }
        }
        //while循环结束，已经搜索到叶子节点，在叶子节点中寻找待删除元素
        for(int i = 0; i < cur->kv_count; i++) {
#if KV_BTYPE_INT_INT
            if(cur->keys[i] == key)
#elif KV_BTYPE_CHAR_CHAR
            if(strcmp(cur->keys[i], key) == 0)//找到待删除元素，直接删除，注意是否需要销毁节点
#endif      
            {
                if(cur->kv_count == 1) {//此时是根节点且只剩下一个元素，因为非根节点都有元素数量限制
                    btree_node_destroy(cur);
                    T->root_node = NULL;
                    T->all_count = 0;
                }
                else {
                    if(i != cur->kv_count - 1) {
                        //执行元素删除前移覆盖
                        for(int j = i; j < (cur->kv_count - 1); j++) {
                            cur->keys[j] = cur->keys[j + 1];
                            cur->values[j] = cur->values[j + 1];
                        }
                    }
#if KV_BTYPE_INT_INT
                    cur->keys[cur->kv_count - 1] = 0;
#elif KV_BTYPE_CHAR_CHAR
                    //清理操作
                    cur->keys[cur->kv_count - 1] = NULL;
                    cur->values[cur->kv_count - 1] = NULL;
#endif
                    cur->kv_count--;
                    T->all_count--;
                }
                return 0;
            }
            else if(i == cur->kv_count - 1) {//元素不存在于B树中
                return -2;
            }
        }
    }
    return -1;
}

//打印当前节点信息
void btree_node_print(btree_node *cur) {
    if(cur == NULL) {
        printf("NULL\n");
    }
    else {
        printf("leaf : %d, kv_count:%d, key : |", cur->leaf, cur->kv_count);
        for(int i = 0; i < cur->kv_count; i++) {
#if KV_BTYPE_INT_INT
            printf("%d | %d", cur->keys[i], cur->values[i]);
#elif KV_BTYPE_CHAR_CHAR
            printf("%s | %s", cur->keys[i], cur->values[i]);
#endif
        }
        printf("\n");
    }
}

//先序遍历给定节点为根节点的子树（递归实现）
void btree_traversal_node(btree *T, btree_node *cur) {
    btree_node_print(cur);

    if(cur->leaf == 0) {
        for(int i = 0; i < cur->kv_count; i++) {
            btree_traversal_node(T, cur->children[i]);
        }
    }
}

//整个btree的先序遍历（调用给定节点的先序遍历方法）
void btree_traversal(btree *T) {
    if(T->root_node != NULL) {
        btree_traversal_node(T, T->root_node);
    }
    else {
        printf("no element in the tree\n");
    }
}

//查找key，返回其所在节点
#if KV_BTYPE_INT_INT
btree_node* btree_search_key(btree *T, B_KEY_SUB_TYPE key){
    if(key > 0){
        btree_node *cur = T->root_node;
        // 先寻找是否为非叶子节点
        while(cur->leaf == 0){
            if(key < cur->keys[0]){
                cur = cur->children[0];
            }else if(key > cur->keys[cur->kv_count-1]){
                cur = cur->children[cur->kv_count];
            }else{
                for(int i=0; i<cur->kv_count; i++){
                    if(cur->keys[i] == key){
                        return cur;
                    }else if((i<cur->kv_count-1) && (key > cur->keys[i]) && (key < cur->keys[i+1])){
                        cur = cur->children[i+1];
                    }
                }
            }
        }
        // 在寻找是否为叶子节点
        if(cur->leaf == 1){
            for(int i=0; i<cur->kv_count; i++){
                if(cur->keys[i] == key){
                    return cur;
                }
            }
        }
    }
    // 都没找到返回NULL
    return NULL;
}
#elif KV_BTYPE_CHAR_CHAR
btree_node *btree_search_key(btree *T, B_KEY_SUB_TYPE key) {
    if(key != NULL) {
        btree_node *cur = T->root_node;
        while(cur->leaf == 0) {
            if(strcmp(key, cur->keys[0]) < 0) {
                cur = cur->children[0];
            }
            else if(strcmp(key, cur->keys[cur->kv_count - 1]) > 0) {
                cur = cur->children[cur->kv_count];
            }
            else {
                for(int i = 0; i < cur->kv_count; i++) {
                    if(strcmp(cur->keys[i], key) == 0) {
                        return cur;
                    }
                    else if((i < cur->kv_count - 1) && (strcmp(key, cur->keys[i]) > 0) && (strcmp(key, cur->keys[i + 1]) < 0)) {
                        cur = cur->children[i + 1];
                    }
                }
            }
        }

        if(cur->leaf == 1) {
            for(int i = 0; i < cur->kv_count; i++) {
                if(strcmp(cur->keys[i], key) == 0) {
                    return cur;
                }
            }
        }
    }
    return NULL;
}
#endif

//获取btree的高度
int btree_depth(btree *T) {
    int depth = 0;
    btree_node *cur = T->root_node;
    while(cur != NULL) {
        depth++;
        cur = cur->children[0];
    }
    return depth;
}

/*------------Btree操作函数定义------------*/


/*------------KV协议函数定义------------*/

//初始化
int kv_btree_init(kv_btree_t *kv_addr, int m) {
    return btree_init(m, kv_addr);
}

//销毁
int kv_btree_desy(kv_btree_t *kv_addr) {
    return btree_destroy(kv_addr);
}

//插入指令
int kv_btree_set(kv_btree_t *kv_addr, char **tokens) {
    return btree_insert_key(kv_addr, tokens[1], tokens[2]);
}

//查找指令
char *kv_btree_get(kv_btree_t *kv_addr, char **tokens) {
    btree_node *node = btree_search_key(kv_addr, tokens[1]);
    if(node != NULL) {
        for(int i = 0; i < node->kv_count; i++) {
            if(strcmp(node->keys[i], tokens[1]) == 0) {
                return node->values[i];
            }
        }
    }
    return NULL;
}

//删除指令
int kv_btree_delete(kv_btree_t *kv_addr, char **tokens) {
    return btree_delete_key(kv_addr, tokens[1]);
}

//计数指令
int kv_btree_count(kv_btree_t *kv_addr) {
    return kv_addr->all_count;
}

//存在指令
int kv_btree_exist(kv_btree_t *kv_addr, char **tokens) {
    return (btree_search_key(kv_addr, tokens[1]) != NULL);
}
/*------------KV协议函数定义------------*/