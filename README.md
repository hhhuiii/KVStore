# Designing different data structures as storage engines for KV storage servers
均为纯C语言实现
## 在服务端使用了6中不同类型的数据结构设计存储引擎
btree

rbtree

skiplist

array

shash

dhash

## 说明
src中存放客户端和服务端以及性能测试相关的代码，另一个是用作存储引擎的6种数据结构的接口定义和具体实现

需要两台linux机器分别作为客户端和服务端

## 简介
是仿照Redis的客户端服务端交互方式，实现一个内存型数据库，没有持久化功能

默认的键值对都是char*类型，实现预期包括通过客户端和服务端的通信，发送相应指令并接收处理后的消息
