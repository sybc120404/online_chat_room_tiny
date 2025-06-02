#ifndef SERVER_H
#define SERVER_H

/*
    Include files
*/

#include "thread_pool.h"

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
    Typedefs
*/

/*
    Defines
*/

/* 线程池参数 */
#define SERVER_THREAD_POOL_SIZE         (5)   /* 服务器线程池大小 */
#define SERVER_THREAD_TASK_QUEUE_SIZE   (10)  /* 服务器线程池任务队列大小 */

/* 服务器最大连接限制 */
#define SERVER_CONNECT_SIZE             (5)

/* socket相关参数 */
#define SERVER_PORT                     (9090) /* 服务器监听端口 */
#define USER_NAME_SIZE               (32)  /* 用户名大小 */
#define BUFFER_HEADER_SIZE              (32)  /* 消息头部大小 */
#define BUFFER_SIZE                     (1024) /* socket读写缓冲区大小 */

/* epoll相关参数 */
#define SERVER_EPOLL_EVENT_SIZE         (10)  /* epoll事件数量 */

typedef enum
{
    MSG_TYPE_MSG = 0,  /* 消息类型 */
    MSG_TYPE_USER_REGISTER, /* 用户注册类型 */
    MSG_TYPE_USER_OFFLINE, /* 用户下线类型 */
    MSG_TYPE_USER_ONLINE,   /* 用户上线类型 */
}msg_type_t;

/* 服务器-客户端通信缓冲区结构 */
typedef struct msg_s
{
    msg_type_t protocol : 8;  /* 协议类型 */
    int length : 24;   /* 消息长度 */
    char data[BUFFER_SIZE - BUFFER_HEADER_SIZE]; /* 消息数据 */
}msg_t;

/* 服务器与客户端连接结构 */
typedef struct connect_s
{
    int fd;
    char user_name[USER_NAME_SIZE]; /* 用户名 */
    msg_t msg;
    struct connect_s *next;
}connect_t;

/* 服务器结构 */
typedef struct server_s
{
    thread_pool_t thread_pool;  /* 服务器线程池 */
    int socket_fd;              /* socket通信文件描述符 */
    connect_t connect_head;     /* 连接队列头 */
    int connect_count;          /* 当前连接的数量 */
    int epoll_fd;            /* epoll文件描述符 */
    pthread_mutex_t mutex;      /* 服务器互斥锁 */
}server_t;

/* 服务器-连接 */
typedef struct server_connect
{
    server_t *p_server;
    int connect_fd;
}server_connect_t;

/*
    Function declarations
*/

/*
    function    服务器对象初始化
    in          p_server                        指向服务器对象
                server_thread_pool_size         服务器线程池线程数量
                server_thread_task_queue_size   服务器线程池任务队列大小
    out
    ret         errCode
*/
ERR_CODE server_init(
    IN OUT server_t *p_server, 
    int server_thread_pool_size, 
    int server_thread_task_queue_size
);

/*
    function    服务器对象销毁
    in          p_server                        指向服务器对象
    out
    ret         errCode
*/
ERR_CODE server_destory(IN server_t *p_server);


#endif