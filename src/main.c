/*
    主要包含reactor模式的实现
    调用kv存储引擎
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "kvstore.h"


#define max_buffer_len 1024//读写buffer长度
#define epoll_events_size 1024//epoll就绪集合大小
#define connblock_size 1024//单个连接块存储的连接数量
#define listen_port_count 1//监听端口数

/*------------回调函数声明------------*/
typedef int (*ZV_CALLBACK)(int fd, int events, void *arg);
//接收连接
int accept_cb(int fd, int events, void *arg);
//接收数据
int recv_cb(int fd, int events, void *arg);
//发送数据
int send_cb(int fd, int events, void *arg);
/*------------回调函数声明------------*/


/*------------数据结构定义------------*/
//描述单个连接的结构体，组织在内存连接块中
typedef struct zv_connect_s
{
    int fd;//本连接的客户端fd
    char rbuffer[max_buffer_len];
    size_t rcount;//读起始位置

    char wbuffer[max_buffer_len];
    size_t wcount;//写起始位置

    size_t next_len;//下一次读数据的长度
    //事件处理回调函数
    ZV_CALLBACK cb;
}zv_connect;

//存储连接的内存块的索引链表节点
typedef struct zv_connblock_index_s{
    struct zv_connect_s *block;//指向此链表节点所代表的内存块(连接数组)
    struct zv_connblock_index_s *next;//指向代表下一个内存块的链表节点
}zv_connblock_index;

//反应堆结构体
typedef struct zv_reactor_s{
    int epfd;//epoll文件描述符
    struct zv_connblock_index_s *blockheader;//连接块链表的第一个节点
    int blkcnt;//现有的连接块的总数
}zv_reactor;
/*------------数据结构定义------------*/


/*------------功能函数声明------------*/
//reactor初始化
int init_reactor(zv_reactor *reactor);
//reactor销毁
void destroy_reactor(zv_reactor *reactor);
//服务端初始化,将端口设置为listen状态
int init_server(int port);
//将本地的listenfd添加进epoll
int set_listener(zv_reactor *reactor, int listenfd, ZV_CALLBACK cb);
//尾插法创建一个新的连接块
int zv_create_connblock(zv_reactor *reactor);
//根据fd从连接块中找到连接所在位置，通过整除与取余的方式
zv_connect *zv_connect_idx(zv_reactor *reactor, int fd);
//运行KV存储协议
int kv_run_while(int argc, char *argv[]);
/*------------功能函数声明------------*/


/*------------功能函数实现------------*/
//reactor初始化实现
int init_reactor(zv_reactor *reactor) {
    if(reactor == NULL) {
        return -1;
    }
    memset(reactor, 0, sizeof(zv_reactor));
    reactor->epfd = epoll_create(1);//int size
    if(reactor->epfd <= 0) {
        perror("init reactor->epfd error\n");
        return -1;
    }

    //首先分配作为存储连接的内存块索引的链表节点
    reactor->blockheader = (zv_connblock_index *)calloc(1, sizeof(zv_connblock_index));//分配1个连续的长度为sizeof(zv_connblock)的内存空间
    if(reactor->blockheader == NULL) {
        perror("reactor init : index node calloc fail\n");
        return -1;
    }
    reactor->blockheader->next = NULL;
    
    //为索引链表节点指向的内存块分配能够存储connblock_size个连接的内存空间
    reactor->blockheader->block = (zv_connect *)calloc(connblock_size, sizeof(zv_connect));
    if(reactor->blockheader->block == NULL) {
        perror("reactor init : memory block calloc fail\n");
        return -1;
    }
    reactor->blkcnt = 1;
    return 0;
}

//reactor销毁
void destroy_reactor(zv_reactor *reactor) {
    if(reactor) {
        close(reactor->epfd);//关闭epoll
        zv_connblock_index *curblk = reactor->blockheader;
        zv_connblock_index *nextblk = reactor->blockheader;
        do {
            curblk = nextblk;
            nextblk = curblk->next;
            if(curblk->block) {
                free(curblk->block);
                curblk->block = NULL;
            }
            if(curblk) {
                free(curblk);
                curblk = NULL;
            }
        } while(nextblk != NULL);//释放连接块链表
    }
}

//服务端初始化：将端口设置为listen状态，绑定成功后返回监听文件描述符
int init_server(int port) {
    //创建一个TCP套接字，AF_INET指定使用IPv4地址族，SOCK_STREAM面向连接的流套接字，0表默认协议，针对SOCK_STREAM操作系统会选择TCP协议
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //sockaddr_in结构体变量，用来存储服务器的地址信息（IP地址和端口号等信息）
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;//IPv4
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//任何IP都可以连接服务器
    serveraddr.sin_port = htons(port);//根据传入port指定端口号
    //将套接字绑定到一个具体的本地地址和端口
    if(-1 == bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr))) {
        perror("bind fail\n");
        close(sockfd);//绑定失败，释放监听文件描述符
        return -1;
    }
    //将端口设置为listen
    listen(sockfd, 10);
    printf("listen port : %d, sockfd = %d\n", port, sockfd);
    return sockfd;
}

//将本地的listenfd添加进epoll
int set_listener(zv_reactor *reactor, int listenfd, ZV_CALLBACK cb) {
    if(!reactor || !reactor->blockheader) {
        perror("set_listener:invalid reactor or reactor->blockheader\n");
        return -1;
    }
    //将服务端放进连接块
    reactor->blockheader->block[listenfd].fd = listenfd;
    reactor->blockheader->block[listenfd].cb = cb;//监听文件描述符触发的是读事件，回调函数是accept_cb
    //将服务端添加进epoll事件
    struct epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;//监听文件描述符关心的是读事件
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, listenfd, &ev);
    return 0;
}   

//尾插法创建一个新的连接块
int zv_create_connblock(zv_reactor *reactor) {
    if(!reactor) {
        return -1;
    }
    //初始化新的连接块
    zv_connblock_index *newblk = (zv_connblock_index *)calloc(1, sizeof(zv_connblock_index));
    if(newblk == NULL) {
        perror("new connblock : node calloc fail\n");
        return -1;
    }
    newblk->block = (zv_connect *)calloc(connblock_size, sizeof(zv_connect));
    if(newblk->block == NULL) {
        perror("new connblock : memory block calloc fail\n");
        return -1;
    }
    newblk->next = NULL;
    //将新分配的块索引插入到索引链表的最后一个节点
    zv_connblock_index *endblk = reactor->blockheader;
    while(endblk->next != NULL) {
        endblk = endblk->next;
    }
    endblk->next = newblk;
    reactor->blkcnt++;//连接块数目+1
    return 0;
}

//根据fd找到该连接位于哪个连接块的第几个连接，返回存储该连接的结构体
//若fd存在则返回其所属连接，若不存在则依照fd顺序存储的方式，返回一个其应该处于的空的连接结构体
zv_connect *zv_connect_idx(zv_reactor *reactor, int fd) {
    if(!reactor) {
        return NULL;
    }
    //由于文件描述符按序分配且连接按序存储，前3个文件描述符已经分配给stdin\stdout\stderr
    int blk_idx = (fd - 3) / connblock_size;
    while(blk_idx >= reactor->blkcnt) {
        zv_create_connblock(reactor);
    }
    zv_connblock_index *blk = reactor->blockheader;
    int index = 0;
    while(++index < blk_idx) {//注意index先自增后比较
        blk = blk->next;
    }
    return &(blk->block[(fd - 3) % connblock_size]);
}
/*------------功能函数实现------------*/


/*------------回调函数实现------------*/
/*参数
    fd:事件对应的文件描述符
    event:事件类型
    arg:reactor，注意进行强制类型转换
 */
//接收连接
int accept_cb(int fd, int event, void *arg) {
    //与客户端建立连接
    struct sockaddr_in clientaddr;//请求连接的客户端地址信息
    socklen_t len_sockaddr = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len_sockaddr);
    if(clientfd < 0) {
        perror("accept new connect fail\n");
        return -1;
    }
    //将连接存储到内存块
    zv_reactor *reactor = (zv_reactor *)arg;
    //由于此连接刚产生，不存在与内存块中，因此返回的是一个空的连接结构体
    //返回的连接结构体表示按照fd顺序存储连接，其应该存储在此返回的连接结构体中
    zv_connect *conn = zv_connect_idx(reactor, clientfd);
    conn->fd = clientfd;
    conn->cb = recv_cb;//所有连接都是默认先由客户端发送数据到服务器
    conn->next_len = max_buffer_len;
    conn->rcount = 0;
    conn->wcount = 0;
    //将其加入epoll实例
    struct epoll_event ev;
    ev.data.fd = clientfd;
    ev.events = EPOLLIN;
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, clientfd, &ev);
    printf("connect established, sockfd : %d, clientfd : %d\n", fd, clientfd);
}

//接收数据
int recv_cb(int fd, int event, void *arg) {
    zv_reactor *reactor = (zv_reactor *)arg;
    zv_connect *conn = zv_connect_idx(reactor, fd);
    int recv_len = recv(fd, conn->rbuffer + conn->rcount, conn->next_len, 0);

    if(recv_len < 0) {//发生错误
        perror("recv error\n");
        close(fd);
        exit(0);
    }
    else if(recv_len == 0) {//对端已关闭连接
        //清除对应连接结构体
        conn->fd = -1;
        conn->rcount = 0;
        conn->wcount = 0;
        //从epoll监听事件中移除
        epoll_ctl(reactor->epfd, EPOLL_CTL_DEL, fd, NULL);
        //关闭连接
        close(fd);
        printf("close connection : clientfd : %d\n", fd);
    }
    else if(recv_len > 0) {//接收到有效数据
        conn->rcount += recv_len;//更新读起始位置
        conn->rcount = kv_protocol(conn->rbuffer, max_buffer_len);
        //将kv存储的回复消息拷贝给wbuffer
        memset(conn->wbuffer, '\0', max_buffer_len);
        //拷贝接收缓冲区到当前接收起始位置的数据
        memcpy(conn->wbuffer, conn->rbuffer, conn->rcount);
        //更新发送缓冲区的当前写起始位置
        conn->wcount = conn->rcount;
        //清空接收缓冲区
        memset(conn->rbuffer, 0, max_buffer_len);
        //接收缓冲区写指针重置
        conn->rcount = 0;

        //事件切换，网络中读和写一般是交替发生
        //将监听事件更改为epoll写事件
        conn->cb = send_cb;
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLOUT;
        epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);
    }
    return 0;
}

//发送数据，这里的实现只是简单的echo，将接收的数据直接回送
int send_cb(int fd, int event, void *arg) {
    zv_reactor *reactor = (zv_reactor *)arg;
    zv_connect *conn = zv_connect_idx(reactor, fd);
    int send_len = send(fd, conn->wbuffer, conn->wcount, 0);
    if(send_len < 0) {
        perror("send fail\n");
        close(fd);
        return -1;
    }
    memset(conn->wbuffer, 0, conn->next_len);
    conn->wcount -= send_len;//发送缓冲区中的数据随发送逐渐减少

    //事件切换，将监听事件切换为epoll读事件
    conn->cb = recv_cb;
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);
    return 0;
}
/*------------回调函数实现------------*/


/*------------主程序运行相关------------*/
//运行KV存储协议解析接收的数据并生成响应信息
int kv_run_while(int argc, char *argv[]) {
    //创建管理连接的reactor
    zv_reactor *reactor = (zv_reactor *)malloc(sizeof(zv_reactor));
    init_reactor(reactor);
    //服务端初始化，输入的第一个参数是一个端口号
    int start_port = atoi(argv[1]);
    //可以同时监听多个端口，但当前设置为仅监听一个端口
    for(int i = 0; i < listen_port_count; i++) {
        int sockfd = init_server(start_port + i);
        set_listener(reactor, sockfd, accept_cb);//将sockfd添加进epoll
    }
    //开始监听事件
    printf("reactor and port init done, listening---\n");
    //就绪事件集合，epoll_wait会将就绪事件按序写入此集合
    struct epoll_event events[epoll_events_size] = {0};
    while(1) {
        //等待事件发生
        int nready = epoll_wait(reactor->epfd, events, epoll_events_size, -1);
        if(nready == -1) {
            perror("epoll_wait fail\n");
            break;
        }
        else if(nready == 0) {
            continue;
        }
        else if(nready > 0) {
            //处理所有就绪事件，epoll_wait函数捕获的就绪事件会在events数组中按序存储，遍历前nready个元素即可
            for(int i = 0;i < nready; i++) {
                int connfd = events[i].data.fd;
                zv_connect *conn = zv_connect_idx(reactor, connfd);
                //事件处理
                //下面两个回调中events[i].events参数未被使用
                if(EPOLLIN & events[i].events) {
                    conn->cb(connfd, events[i].events, reactor);
                }
                if(EPOLLOUT & events[i].events) {
                    conn->cb(connfd, events[i].events, reactor);
                }
            }
        }
    }
    destroy_reactor(reactor);
    return 0;
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        perror("request a port as input\n");
        return -1;
    }
    //初始化存储引擎
    kv_engine_init();
    //运行KV存储
    kv_run_while(argc, argv);
    //销毁存储引擎
    kv_engine_desy();

    return 0;
}
/*------------主程序运行相关------------*/