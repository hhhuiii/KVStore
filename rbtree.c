#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "rbtree.h"

#define RED 1
#define BLACK 0

/*
    RBtree本质上是系列要求下的平衡二叉树，因此除了插入删除引发的树调整外，逻辑比Btree简单
*/

/*------------rbtree操作函数声明------------*/

/*------初始化和释放内存------*/

//初始化红黑树
int rbtree_malloc(rbtree *T);

//释放红黑树内存
void rbtree_free(rbtree *T);

/*------初始化和释放内存------*/


/*------插入------*/

//红黑树插入
int rbtree_insert(rbtree *T, RB_KEY_TYPE key, RB_VALUE_TYPE value);

//红黑树插入后的调整
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur);

/*------插入------*/


/*------删除------*/

//红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del);

//红黑树删除后的调整
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur);

/*------删除------*/


/*------查找------*/

//红黑树查找
rbtree_node *rbtree_search(rbtree *T, RB_KEY_TYPE key);

/*------查找------*/


/*------其他------*/

//在给定节点为根节点的子树中，找出key最小的节点
rbtree_node *rbtree_min(rbtree *T, rbtree_node *cur);

//在给定节点为根节点的子树中，找出key最大的节点
rbtree_node *rbtree_max(rbtree *T, rbtree_node *cur);

//当前节点的前驱节点
rbtree_node *rbtree_precursor_node(rbtree *T, rbtree_node *cur);

//当前节点的后继节点
rbtree_node *rbtree_successor_node(rbtree *T, rbtree_node *cur);

//红黑树节点左旋，无需修改颜色
void rbtree_left_rotate(rbtree *T, rbtree_node *x);

//红黑树节点右旋，无需修改颜色
void rbtree_right_rotate(rbtree *T, rbtree_node *y);

//红黑树的深度：不含叶子节点
int rbtree_depth(rbtree *T);

//红黑树的深度（递归实现）
int rbtree_depth_recursion(rbtree *T, rbtree_node *cur);

/*------其他------*/

/*------------rbtree操作函数声明------------*/


/*------------函数定义------------*/

//红黑树初始化
//这些赋值操作是红黑树的典型初始化赋值操作
//注意在初始化时仅初始化了nil_node
int rbtree_malloc(rbtree *T) {
    rbtree_node *nil_node = (rbtree_node *)malloc(sizeof(rbtree_node));
    //定义叶子节点和根节点的父节点
    nil_node->color = BLACK;
    nil_node->left = nil_node;
    nil_node->right = nil_node;
    nil_node->parent = nil_node;
    
    T->root_node = nil_node;
    T->nil_node = nil_node;
    T->count = 0;
    return 0;
}

//红黑树释放内存
/*
    红黑树释放内存操作中，仅释放nil_node是因为nil_node是红黑树中唯一的全局共享哨兵节点
    普通节点的释放通常需要在其他地方单独处理
*/
void rbtree_free(rbtree *T) {
    if(T->nil_node) {
        free(T->nil_node);
        T->nil_node = NULL;
    }
}

//在给定节点作为根节点的子树中，找出key最小的节点
//就和普通的二叉树查找key最小的节点操作一致
rbtree_node *rbtree_min(rbtree *T, rbtree_node *cur) {
    //当搜索到左孩子为nil_node时已经搜索到最底层最左边，退出循环
    while(cur->left != T->nil_node) {
        cur = cur->left;
    }
    return cur;
}

//在给定节点作为根节点的子树中，找出key最大的节点
//同寻找key最小节点
rbtree_node *rbtree_max(rbtree *T, rbtree_node *cur) {
    while(cur->right != T->nil_node) {
        cur = cur->right;
    }
    return cur;
}

//找出当前节点的前驱节点
//当前节点的前驱节点应该是其左子树中的最右下节点
//若没有左孩子，则向上搜索，直到某一结点是其父节点的右孩子，此时该节点的父节点即为前驱节点
rbtree_node *rbtree_precursor_node(rbtree *T, rbtree_node *cur) {
    //若当前节点有左孩子，直接向下搜索
    if(cur->left != T->nil_node) {
        cur = cur->left;
        while(cur->right != T->nil_node) {
            cur = cur->right;
        }
        return cur;
    }
    //若当前节点没有左孩子，向上搜索
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->left)) {
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    //若返回值为nil_node，已经搜索到根节点的父节点了，说明当前节点为第一个节点（没有前驱）
}

//找出当前节点的后继节点
//当前节点的后继节点应该是其右子树最左下的节点
//若当前节点没有右子树，则向上搜索，直到某一节点是其父节点的左孩子，此时该父节点即为后继节点
rbtree_node *rbtree_successor_node(rbtree *T, rbtree_node *cur) {
    //若当前节点有右孩子，直接向下搜索
    if(cur->right != T->nil_node) {
        cur = cur->right;
        while(cur->left != T->nil_node) {
            cur = cur->left;
        }
        return cur;
    }
    //若当前节点没有右孩子，向上搜索
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->right)) {
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    //若返回值为nil_node,已经搜索到根节点的父节点了，说明当前节点为第一个节点（没有前驱）
}

//将以x为根节点的红黑树节点左旋，不涉及颜色修改操作
void rbtree_left_rotate(rbtree *T, rbtree_node *x) {
    rbtree_node *y = x->right;

    x->right = y->left;
    if(y->left != T->nil_node) {
        y->left->parent = x;
    }
    //将x为根，y为x右孩子的子树左旋
    y->parent = x->parent;
    //若x原来为根节点，需要特别更新rbtree的根节点
    if(x->parent == T->nil_node) {
        T->root_node = y;
    }
    //x并非根节点
    //视x为其原父节点的左子树还是右子树进行相应操作
    else if(x->parent->left == x) {
        x->parent->left = y;
    }
    else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

//将以y为根节点的红黑树节点右旋，不涉及颜色修改操作
void rbtree_right_rotate(rbtree *T, rbtree_node *y) {
    rbtree_node *x = y->left;

    y->left = x->right;
    if(x->right != T->nil_node) {
        x->right->parent = y;
    }
    //将以y为根，x为其左孩子的子树右旋
    x->parent = y->parent;
    //若y原来为根节点，需要更新rbtree的根节点参数
    if(y->parent == T->nil_node) {
        T->root_node = x;
    }
    //若y非根节点
    //视y为其原父节点的左子树或右子树进行相应操作
    else if(y->parent->left == y) {
        y->parent->left = x;
    }
    else {
        y->parent->right = x;
    }
    x->right = y;
    y->parent = x;
}

//插入修复，最后需要调整cur节点为参与调整的子树的根节点
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur) {
    //父节点是黑色，无需调整
    //父节点是红色，有如下八种情况
    while(cur->parent->color == RED) {
        //获取叔节点
        rbtree_node *uncle;
        if(cur->parent->parent->left == cur->parent) {
            uncle = cur->parent->parent->right;
        }
        else {
            uncle = cur->parent->parent->left;
        }
        //若叔节点为红色，只需更新颜色
        //循环主要在这里起作用
        if(uncle->color == RED) {
            //叔节点为红色：祖父变红/父变黑/叔变黑、祖父节点成为新的当前节点
            cur->parent->parent->color = RED;
            cur->parent->color = BLACK;
            uncle->color = BLACK;
            cur = cur->parent->parent;//将祖父节点设置为当前节点后，递归向上调整
        }
        //若叔节点为黑色，需要变色+旋转
        else {
            //当前节点相对于祖父节点的位置有四种情况：LL/RR/LR/RL
            //依次判断四种情况

            //若cur节点的父节点为祖父节点的左子树（LR或LL情况）
            if(cur->parent->parent->left == cur->parent) {
                //LL:祖父变红/父变黑、祖父右旋，调整后该子树的根节点应该是原来的当前节点
                if(cur->parent->left == cur) {
                    cur->parent->parent->color = RED;
                    cur->parent->color = BLACK;
                    rbtree_right_rotate(T, cur->parent->parent);
                }
                //LR:祖父变红/父变红/当前节点变黑、父左旋，祖父右旋。调整后该子树的根节点应该是原来的祖父节点
                else {
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_left_rotate(T, cur);
                    rbtree_right_rotate(T, cur->parent->parent);
                }
            }
            
            //若cur节点的父节点为祖父节点的右子树（RR或RL情况）
            else {
                //RL:祖父变红/父变红/当前节点变黑、父右旋、祖父左旋。最后的当前节点应该是原来的祖父节点
                if(cur->parent->left == cur) {
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_right_rotate(T, cur);
                    rbtree_right_rotate(T, cur->parent->parent);
                }
                //RR:祖父变红/父变黑、祖父左旋，最后的当前节点应该是原来的当前节点
                else {
                    cur->parent->parent->color = RED;
                    cur->parent->color - BLACK;
                    rbtree_left_rotate(T, cur->parent->parent);
                }
            }
        }
    }
    //将根节点变为黑色
    T->root_node->color = BLACK;
}

//插入
int rbtree_insert(rbtree *T, RB_KEY_TYPE key, RB_VALUE_TYPE value) {
    //首先寻找插入位置
    rbtree_node *cur = T->root_node;
    rbtree_node *next = T->root_node;//next为探测节点，以cur作为其前驱节点，知道cur为某叶节点的父节点
    //插入的位置一定是叶子节点
    while(next != T->nil_node) {
        cur = next;
        //比当前键大，继续搜索右子树
        if(strcmp(key, cur->key) > 0) {
            next = cur->right;
        }
        //比当前键小，继续搜索左子树
        else if(strcmp(key, cur->key) < 0) {
            next = cur->left;
        }
        //待插入键已经存在
        else if(strcmp(key, cur->key) == 0) {
            return -2;
        }
    }
    //while循环结束后，若未在树中找到该key，必定是已经搜索到根节点
    //next指向的是叶节点，因而cur指向某一底层节点，新的键可能插入其左子树，可能插入其右子树

    //创建新节点，熟悉的指针拷贝操作
    rbtree_node *new = (rbtree_node *)malloc(sizeof(rbtree_node));
    if(new == NULL) {
        return -1;
    }
    char *key_copy = (char *)malloc(strlen(key) + 1);
    if(key_copy == NULL) {
        free(new);
        new = NULL;
        return -1;
    }
    strncpy(key_copy, key, strlen(key) + 1);
    char *value_copy = (char *)malloc(strlen(value) + 1);
    if(value_copy == NULL) {
        free(key_copy);
        key_copy = NULL;
        free(new);
        new = NULL;
        return -1;
    }
    strncpy(value_copy, value, strlen(value) + 1);
    new->key = key_copy;
    new->value = value_copy;

    //将新节点插入到叶子节点
    if(cur == T->nil_node) {
        T->root_node = new;
    }
    //比cur的键大，插入到cur的右子树
    else if(strcmp(new->key, cur->key) > 0) {
        cur->right = new;
    }
    //比cur的键小，插入到cur的左子树
    else {
        cur->left = new;
    }
    //更新系列指针
    new->parent = cur;
    new->left = T->nil_node;//设置新节点的左右叶子节点
    new->right = T->nil_node;
    new->color = RED;//新插入的节点初始均为红色
    T->count++;
    //调整红黑树使红色节点不相邻
    rbtree_insert_fixup(T, new);//插入修复操作
    return 0;
}

//删除修复
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur) {
    //cur是黑色，cur不是根节点才会进入循环
    while((cur->color = BLACK) && (cur != T->root_node)) {
        rbtree_node *brother = T->nil_node;//cur节点的兄弟节点
        if(cur->parent->left == cur) {
            brother = cur->parent->right;
        }
        else {
            brother = cur->parent->left;
        }
        //兄弟节点为红色（此时父节点必为黑），父变红/兄弟变黑、父单旋、当前节点下一循环
        if(brother->color == RED) {
            cur->parent->color = RED;
            brother->color = BLACK;
            //cur为左子树，左旋
            if(cur->parent->left == cur) {
                rbtree_left_rotate(T, cur->parent);
            }
            //cur为右子树，右旋
            else {
                rbtree_right_rotate(T, cur->parent);
            }
        }
        //兄弟节点为黑色
        else {
            //兄弟节点没有红色子节点，父变黑/兄弟变红、看情况是否结束循环
            if((brother->left->color == BLACK) && (brother->right->color == BLACK)) {
                //若父原来为黑，父节点作为新的当前节点进入下一循环，递归向上调整
                if(brother->parent->color == BLACK) {
                    cur = cur->parent;
                }
                else {
                    cur = T->root_node;
                }
                brother->parent->color = BLACK;
                brother->color = RED;
            }
            //兄弟节点有红色子节点：LL/LR/RR/RL
            else if(brother->parent->left == brother) {
                //LL:红子变黑/兄弟变父色/父变黑、父右旋、结束循环
                if(brother->left->color == RED) {
                    brother->left->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                //LR:红子变父色/父变黑、兄弟左旋/父右旋、结束循环
                else {
                    brother->right->color = brother->parent->color;
                    cur->parent->color = BLACK;
                    rbtree_left_rotate(T, brother);
                    rbtree_right_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }
            else {
                //RR:红子变黑/兄弟变父色/父变黑、父左旋、结束循环
                if(brother->right->color == RED) {
                    brother->right->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_left_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                //RL:红子变父色/父变黑、兄弟右旋/父左旋、结束循环
                else {
                    brother->left->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother);
                    rbtree_left_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }
        }
    }
    cur->color = BLACK;
}

//红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del) {
    if(del != T->nil_node) {
        /*
            1.标准的BST删除操作：最后都会转换成删除只有一个子节点或没有子节点的节点
            2.若删除节点为黑色，需要进行调整
        */
        rbtree_node *del_r = T->nil_node;//实际删除的节点
        rbtree_node *del_r_child = T->nil_node;//实际删除节点的子节点（最后处理时，可能记录的是唯一的子节点，可能记录的是叶子节点，用于替代实际被删除节点的位置）
        //找出实际删除的节点
        //实际删除的节点最多只有一个子节点，或者没有子节点（必然在不包含叶子节点的最后两层中）
        if((del->left == T->nil_node) || (del->right == T->nil_node)) {
            //如果要删除的节点本身只有一个孩子或没有孩子，则实际删除的节点就是该节点
            del_r = del;
        }
        else {
            //如果要删除的节点有两个孩子，就转而删除其后继或前驱节点（此处为后继）
            //通过寻找其后继或者前驱节点（后继或前驱节点必定最多只有一个子节点），可将其转为上面的实际删除节点最多只有一个子节点的情况
            del_r = rbtree_successor_node(T, del);
        }

        //删除节点的孩子，没有孩子记录空节点，用于替换实际删除的节点
        if(del_r->left != T->nil_node) {
            del_r_child = del_r->left;
        }
        else {
            del_r_child = del_r->right;
        }
        
        //将实际要删除的节点删除

        del_r_child->parent = del_r->parent;
        //实际被删除节点的父节点的孩子指针更新
        //若实际被删除节点就是根节点
        if(del_r->parent == T->nil_node) {
            T->root_node = del_r_child;
        }
        //若实际被删除节点是其父节点的左孩子
        else if(del_r->parent->left == del_r) {
            del_r->parent->left = del_r_child;
        }
        //若实际被删除节点是其父节点的右孩子
        else {
            del_r->parent->right = del_r_child;
        }

        //节点替换，转换为删除替代节点
        if(del != del_r) {
            free(del->key);
            del->key = del_r->key;
            free(del->value);
            del->value = del_r->value;
        }

        //最后看是否需要调整

        //若替换节点为黑色，需要进行删除调整
        //节点为黑色但并非根节点时需要调整
        if(del_r->color == BLACK) {
            rbtree_delete_fixup(T, del_r_child);
        }

        //调整空节点的父节点，空节点的父节点为nil_node
        if(del_r_child == T->nil_node) {
            del_r_child->parent = T->nil_node;
        }
        //释放替代删除节点
        free(del_r);
        T->count--;
    }
}

//查找
rbtree_node *rbtree_search(rbtree *T, RB_KEY_TYPE key) {
    rbtree_node *cur = T->root_node;
    while(cur != T->nil_node) {
        if(strcmp(cur->key, key) > 0) {
            cur = cur->left;
        }
        else if(strcmp(cur->key, key) < 0) {
            cur = cur->right;
        }
        else if(strcmp(cur->key, key) == 0) {
            return cur;
        }
    }
    return T->nil_node;
}

//中序遍历给定节点为根节点的子树（递归实现）
void rbtree_traversal_node(rbtree *T, rbtree_node *cur) {
    if(cur != T->nil_node) {
        rbtree_traversal_node(T, cur->left);
        if(cur->color == RED) {//同时打印节点颜色
            printf("Key:%s\tColor:Red\n", cur->key);
        }
        else {
            printf("Key:%s\tColor:Black\n", cur->key);
        }
        rbtree_traversal_node(T, cur->right);
    }
}

//中序遍历整个红黑树
void rbtree_traversal(rbtree *T) {
    rbtree_traversal_node(T, T->root_node);
}

//递归计算红黑树的深度（不包括叶子节点），深度指最大深度
int rbtree_depth_recursion(rbtree *T, rbtree_node *cur) {
    if(cur == T->nil_node) {
        return 0;
    }
    else {
        int left = rbtree_depth_recursion(T, cur->left);
        int right = rbtree_depth_recursion(T, cur->right);
        return ((left > right) ? left : right) + 1;//返回两者中较深的深度
    }
}

//返回红黑树的深度
int rbtree_depth(rbtree *T) {
    return rbtree_depth_recursion(T, T->root_node);
}

//获取输入数字的十进制显示宽度
int decimal_width(int num_in) {
    int width = 0;
    while(num_in != 0) {
        num_in = num_in / 10;
        width++;
    }
    return width;
}

/*------------函数定义------------*/


/*------------KV函数定义------------*/

//初始化
int kv_rbtree_init(kv_rbtree_t *kv_addr) {
    return rbtree_malloc(kv_addr);
}

//销毁
int kv_rbtree_desy(kv_rbtree_t *kv_addr) {
    rbtree_free(kv_addr);
    return 0;
}

//插入指令
int kv_rbtree_set(rbtree *kv_addr, char **tokens) {
    return rbtree_insert(kv_addr, tokens[1], tokens[2]);
}

//查找指令
char *kv_rbtree_get(rbtree *kv_addr, char **tokens) {
    rbtree_node *node = rbtree_search(kv_addr, tokens[1]);
    if(node != kv_addr->nil_node) {
        return node->value;
    }
    else {
        return NULL;
    }
}

//删除指令
int kv_rbtree_delete(rbtree *kv_addr, char **tokens) {
    rbtree_node *node = rbtree_search(kv_addr, tokens[1]);
    if(node == NULL) {
        return -2;
    }
    rbtree_delete(kv_addr, node);
    return 0;
}

//计数指令
int kv_rbtree_count(rbtree *kv_addr) {
    return kv_addr->count;
}

//存在指令
int kv_rbtree_exist(rbtree *kv_addr, char **tokens) {
    return (rbtree_search(kv_addr, tokens[1]) != NULL);
}

/*------------KV函数定义------------*/