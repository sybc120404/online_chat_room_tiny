# online_chat_room_tiny

一个简易网络聊天室，C语言实现

内核版本    6.11.0-26-generic

gcc版本     13.3.0

## 项目相关

项目构建：在项目根目录下执行
```
make server
```

生成`server`为服务端的可执行文件

```
make client
```

生成`client`为服务端的可执行文件

执行
```
make clean
```

清理工程

项目运行：一个终端执行`./server`，其他终端执行`./client [user_name]`，然后就可以在client端进行聊天

## 整体架构

```
.
├── inc
│   ├── client.h
│   ├── debug_log.h
│   ├── server.h
│   └── thread_pool.h
├── LICENSE
├── Makefile
├── obj
├── README.md
└── src
    ├── client.c
    ├── debug_log.c
    ├── server.c
    └── thread_pool.c
```

- client和server通过socket进行通信
- server使用epoll管理socket连接的描述符
- server处理一些链表之类的耗时操作，交给线程池处理

### 线程池

代码参考[thread_pool](src/thread_pool.c)

### 调试

查看[dbg](inc/debug_log.h)