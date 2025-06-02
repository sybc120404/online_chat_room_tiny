/*
    Include files
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>

#include "server.h"

/*
    Variables
*/

static server_t server = {};

/*
    Function Definitions
*/

/*
    function    信号处理函数，用于中断时关闭服务器
    in          signum  信号编号
    out
    ret
*/
static void signal_handler(int signum)
{
    if(signum == SIGINT)
    {
        /* 信号处理函数避免printf等不可重入函数/异步信号不安全函数 */
        //DBG("Received signal %d, shutting down server...", signum);
        server_destory(&server);
        exit(0);
    }
}

/*
    function    向连接链表中新增连接，用于提交给线程池
    in          s_c     指向服务器连接参数
    out
    ret
*/
static void connect_list_add(void *s_c)
{
    connect_t *new_connect = NULL;
    server_connect_t *arg = (server_connect_t *)s_c;
    server_t *p_server = arg->p_server;
    int client_fd = arg->connect_fd;

    /* 分配新的连接结构 */
    new_connect = (connect_t *)malloc(sizeof(connect_t));
    if(NULL == new_connect)
    {
        DBG_ERR("malloc for new connect");
        return;
    }
    memset(new_connect, 0, sizeof(connect_t));

    /* 初始化新连接 */
    new_connect->fd = client_fd;
    new_connect->next = NULL;

    pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */

    /* 将新连接添加到服务器连接列表 */
    if(NULL == p_server->connect_head.next)
    {
        p_server->connect_head.next = new_connect;  /* 第一个连接 */
    }
    else
    {
        connect_t *ptr = p_server->connect_head.next;
        while(ptr->next)  /* 找到最后一个连接 */
        {
            ptr = ptr->next;
        }
        ptr->next = new_connect;  /* 添加到最后 */
    }
    p_server->connect_count++;  /* 增加连接计数 */

    pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */

    DBG_ALZ("add new connect fd %d to server, total connects: %d", client_fd, p_server->connect_count);

    return;
}

/*
    function    从连接链表中删除连接，用于提交给线程池
    in          s_c     指向服务器连接参数
    out
    ret
*/
static void connect_list_del(void *s_c)
{
    server_connect_t *arg = (server_connect_t *)s_c;
    server_t *p_server = arg->p_server;
    int connect_fd = arg->connect_fd;
    connect_t *ptr = NULL;
    connect_t *prev = NULL;

    pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */

    /* 从连接链表中删除 */
    ptr = p_server->connect_head.next;
    prev = &p_server->connect_head;  /* 头节点的前一个指针 */
    while(ptr)
    {
        if(ptr->fd == connect_fd)  /* 找到要删除的连接 */
        {
            prev->next = ptr->next;  /* 删除当前连接 */
            free(ptr);               /* 释放内存 */
            p_server->connect_count--;  /* 减少连接计数 */
            DBG("removed client %d from server, total connects: %d", connect_fd, p_server->connect_count);
            break;
        }
        prev = ptr;  /* 移动到下一个连接 */
        ptr = ptr->next;
    }

    pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */
}

/*
    function    从连接链表中查找指定节点，返回指针
    in          connect_fd   要查找的连接文件描述符
    out
    ret         ptr
*/
connect_t* connect_list_find(IN server_t *p_server, IN int connect_fd)
{
    connect_t *ptr = NULL;

    PFM_ENSURE_RET(NULL != p_server, NULL);
    PFM_ENSURE_RET(-1 != connect_fd, NULL);

    pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */

    ptr = server.connect_head.next;  /* 从头节点开始查找 */
    while(ptr)
    {
        if(ptr->fd == connect_fd)  /* 找到匹配的连接 */
        {
            pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */
            return ptr;
        }
        ptr = ptr->next;  /* 移动到下一个连接 */
    }

    pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */

    DBG_ERR("connect fd %d not found in server", connect_fd);
    return NULL;  /* 没有找到 */
}

/*
    function    处理客户端消息
    in          s_c     指向服务器连接参数
    out
    ret
*/
static void handle_client_msg(void *s_c)
{
    server_connect_t *arg = (server_connect_t *)s_c;
    server_t *p_server = arg->p_server;
    int connect_fd = arg->connect_fd;
    connect_t *p_connect = NULL;
    connect_t *ptr = NULL;
    msg_t msg = {};
    char buffer_tmp[BUFFER_SIZE*2] = {};

    PFM_ENSURE_RET(NULL != p_server, );
    PFM_ENSURE_RET(-1 != connect_fd, );

    /* 查找连接 */
    p_connect = connect_list_find(p_server, connect_fd);
    if(NULL == p_connect)
    {
        DBG_ERR("connect fd %d not found in server", connect_fd);
        return;
    }

    switch(p_connect->msg.protocol)
    {
        case MSG_TYPE_USER_REGISTER:    /* 处理用户注册 */
        {
            DBG("handle user register from fd %d: %s", connect_fd, p_connect->msg.data);
            memcpy(p_connect->user_name, p_connect->msg.data, USER_NAME_SIZE);
            break;
        }
        case MSG_TYPE_MSG:  /* 处理普通消息 */
        {
            DBG("handle client msg from fd %d, user_name %s: %s", connect_fd, p_connect->user_name, p_connect->msg.data);

            /* 封装消息 */
            msg.protocol = MSG_TYPE_MSG;
            snprintf(buffer_tmp, sizeof(buffer_tmp), "[%s] %s", p_connect->user_name, p_connect->msg.data);
            memcpy(msg.data, buffer_tmp, BUFFER_SIZE-BUFFER_HEADER_SIZE-1);
            msg.data[BUFFER_SIZE-BUFFER_HEADER_SIZE-1] = '\0';
            msg.length = strlen(msg.data);
            /* 广播给其他客户端 */
            pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */
            ptr = p_server->connect_head.next;  /* 从头节点开始遍历 */
            while(ptr)
            {
                if(ptr->fd != connect_fd)  /* 不发送给自己 */
                {
                    send(ptr->fd, (void*)&msg, sizeof(msg_t), 0);
                }
                ptr = ptr->next;
            }
            pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */
            break;
        }
        case MSG_TYPE_USER_OFFLINE: /* 处理下线消息 */
        {
            DBG("handle user offline from fd %d, %s", connect_fd, p_connect->user_name);

            /* 封装消息 */
            msg.protocol = MSG_TYPE_USER_OFFLINE;
            snprintf(buffer_tmp, sizeof(buffer_tmp), "[%s] offline", p_connect->user_name);
            memcpy(msg.data, buffer_tmp, BUFFER_SIZE-BUFFER_HEADER_SIZE-1);
            msg.data[BUFFER_SIZE-BUFFER_HEADER_SIZE-1] = '\0';
            msg.length = strlen(msg.data);

            pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */
            ptr = p_server->connect_head.next;  /* 从头节点开始遍历 */
            while(ptr)
            {
                if(ptr->fd != connect_fd)  /* 不发送给自己 */
                {
                    send(ptr->fd, (void*)&msg, sizeof(msg_t), 0);
                }
                ptr = ptr->next;
            }
            pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */

            break;
        }
        case MSG_TYPE_USER_ONLINE: /* 处理上线消息 */
        {
            DBG("handle user online from fd %d, %s", connect_fd, p_connect->user_name);

            /* 封装消息 */
            msg.protocol = MSG_TYPE_USER_ONLINE;
            snprintf(buffer_tmp, sizeof(buffer_tmp), "[%s] online", p_connect->user_name);
            memcpy(msg.data, buffer_tmp, BUFFER_SIZE-BUFFER_HEADER_SIZE-1);
            msg.data[BUFFER_SIZE-BUFFER_HEADER_SIZE-1] = '\0';
            msg.length = strlen(msg.data);

            pthread_mutex_lock(&(p_server->mutex));  /* 锁定服务器互斥锁 */
            ptr = p_server->connect_head.next;  /* 从头节点开始遍历 */
            while(ptr)
            {
                if(ptr->fd != connect_fd)  /* 不发送给自己 */
                {
                    send(ptr->fd, (void*)&msg, sizeof(msg_t), 0);
                }
                ptr = ptr->next;
            }
            pthread_mutex_unlock(&(p_server->mutex));  /* 解锁服务器互斥锁 */

            break;
        }
        default:
        {
            DBG_ERR("unknown message type %d from client fd %d", p_connect->msg.protocol, connect_fd);
            break;
        }
    }
    
    free(s_c);  /* 释放服务器连接参数内存 */
    return;
}

static ERR_CODE handler_new_connection(IN server_t *p_server, IN int socket_fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = 0;
    struct epoll_event ev = {};
    server_connect_t s_c = {};

    PFM_ENSURE_RET(NULL != p_server, ERR_BAD_PARAM);
    PFM_ENSURE_RET(-1 != socket_fd, ERR_BAD_PARAM);

    client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_len);
    if(-1 == client_fd)
    {
        DBG_ERR("accept new connection failed");
        perror("accept");
        goto err;
    }
    DBG_ALZ("accepted new connection from %s:%d, fd %d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

    /* 设置新连接为非阻塞 */
    if(-1 == fcntl(client_fd, F_SETFL, O_NONBLOCK))
    {
        DBG_ERR("set client socket to non-blocking failed");
        perror("fcntl set non-blocking");
        goto err;
    }
    DBG("set client socket %d to non-blocking", client_fd);

    /* 将新连接添加到epoll */
    ev.events = EPOLLIN | EPOLLET;  /* 可读事件 */
    ev.data.fd = client_fd;  /* 新连接的文件描述符 */
    if(-1 == epoll_ctl(p_server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev))
    {
        DBG_ERR("add new connect fd %d to epoll failed", client_fd);
        perror("epoll ctl add");
        goto err;
    }

    s_c.p_server = p_server;
    s_c.connect_fd = client_fd;
    connect_list_add((void*)&s_c);

    return ERR_NO_ERROR;

err:
    DBG_ERR("handle new connection failed");
    if(-1 != client_fd) close(client_fd);
    return ERR_SERVER_NEW_CONNECT;
}

static ERR_CODE handler_read_event(IN server_t *p_server, IN int connect_fd)
{
    char buffer[BUFFER_SIZE] = {};
    ssize_t bytes_read = 0;
    connect_t *p_connect = NULL;
    server_connect_t *s_c = NULL;
    
    PFM_ENSURE_RET(NULL != p_server, ERR_BAD_PARAM);
    PFM_ENSURE_RET(-1 != connect_fd, ERR_BAD_PARAM);

    bytes_read = read(connect_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';

        p_connect = connect_list_find(p_server, connect_fd);  /* 确保连接存在 */
        if(NULL != p_connect)
        {
            memset(&p_connect->msg, 0, sizeof(msg_t));  /* 清空缓冲区 */
            memcpy(&p_connect->msg, buffer, sizeof(msg_t));
        }

        s_c = (server_connect_t *)malloc(sizeof(server_connect_t));

        if(NULL == s_c)
        {
            DBG_ERR("malloc for server connect");
            return ERR_NO_MEMORY;
        }
        memset(s_c, 0, sizeof(server_connect_t));

        s_c->p_server = p_server;
        s_c->connect_fd = connect_fd;

        /* 线程池处理数据 */
        thread_pool_add_task(&(p_server->thread_pool), handle_client_msg, (void*)s_c);
    }
    else if (bytes_read == 0)       /* 客户端关闭连接 */
    {
        close(connect_fd);

        s_c = (server_connect_t *)malloc(sizeof(server_connect_t));
        if(NULL == s_c)
        {
            DBG_ERR("malloc for server connect");
            return ERR_NO_MEMORY;
        }
        memset(s_c, 0, sizeof(server_connect_t));
        s_c->p_server = p_server;
        s_c->connect_fd = connect_fd;
        thread_pool_add_task(&(p_server->thread_pool), connect_list_del, (void *)&s_c);    /* 链表操作交给线程池处理 */
        DBG_ALZ("client %d closed connection", connect_fd);
    }
    else
    {
        DBG_ERR("read from client %d failed", connect_fd);
    }

    return ERR_NO_ERROR;
}

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
)
{
    int thread_pool_flag = 0;
    struct sockaddr_in server_addr = {};
    struct epoll_event ev = {};

    PFM_ENSURE_RET(NULL != p_server, ERR_BAD_PARAM);
    PFM_ENSURE_RET(0 < server_thread_pool_size && 0 < server_thread_task_queue_size, ERR_BAD_PARAM);

    /* 初始化服务器线程池 */
    PFM_ENSURE_RET(ERR_NO_ERROR == thread_pool_init(&(p_server->thread_pool), server_thread_pool_size, server_thread_task_queue_size), ERR_SERVER_INIT);
    thread_pool_flag = 1;
    DBG("server init thread pool with %d threads, %d tasks", server_thread_pool_size, server_thread_task_queue_size);

    /* 创建socket */
    p_server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == p_server->socket_fd)
    {
        DBG_ERR("create socket failed");
        perror("socket create");
        goto err;
    }
    DBG("create socket %d", p_server->socket_fd);

    /* 设置socket选项，允许地址重用 */
    int opt = 1;
    setsockopt(p_server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    DBG("set socket %d option SO_REUSEADDR", p_server->socket_fd);

    /* 设置socket为非阻塞 */
    if(-1 == fcntl(p_server->socket_fd, F_SETFL, O_NONBLOCK))
    {
        DBG_ERR("set socket to non-blocking failed");
        perror("fcntl set non-blocking");
        goto err;
    }
    DBG("set socket %d to non-blocking", p_server->socket_fd);

    /* 绑定socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);        /* 绑定所有接口上 */
    server_addr.sin_port = htons(SERVER_PORT);
    if(0 != bind(p_server->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        DBG_ERR("bind socket failed");
        perror("socket bind");
        goto err;
    }
    DBG("server bind socket %d to %d", p_server->socket_fd, SERVER_PORT);

    /* 监听socket */
    if(0 != listen(p_server->socket_fd, SERVER_CONNECT_SIZE))
    {
        DBG_ERR("listen socket failed");
        perror("socket listen");
        goto err;
    }
    DBG("server listen socket %d", p_server->socket_fd);

    /* 初始化epoll */
    p_server->epoll_fd = epoll_create(10);
    if(-1 == p_server->epoll_fd)
    {
        DBG_ERR("create epoll failed");
        perror("epoll create");
        goto err;
    }
    DBG("create epoll fd %d", p_server->epoll_fd);

    /* 将socket添加到epoll */
    ev.events = EPOLLIN | EPOLLET;  /* 可读事件 */
    ev.data.fd = p_server->socket_fd;  /* 监听socket的文件描述符 */
    if(-1 == epoll_ctl(p_server->epoll_fd, EPOLL_CTL_ADD, p_server->socket_fd, &ev))
    {
        DBG_ERR("add socket to epoll failed");
        perror("epoll ctl add");
        goto err;
    }
    DBG("add socket %d to epoll fd %d", p_server->socket_fd, p_server->epoll_fd);

    /* 初始化互斥锁 */
    pthread_mutex_init(&(p_server->mutex), NULL);
    DBG("initialize server mutex");

    /* 初始化其他参数 */
    p_server->connect_head.fd = -1;
    p_server->connect_head.next = NULL;
    p_server->connect_count = 0;

    DBG_ALZ("server init done");
    return ERR_NO_ERROR;

err:
    DBG_ERR("server init failed, cleaning up resources");

    if(thread_pool_flag)    thread_pool_destroy(&(p_server->thread_pool));
    if(-1 != p_server->socket_fd)       close(p_server->socket_fd);
    if(-1 != p_server->epoll_fd)         close(p_server->epoll_fd);
    pthread_mutex_destroy(&(p_server->mutex));
    memset(p_server, 0, sizeof(server_t));

    return ERR_SERVER_INIT;
}

/*
    function    服务器对象销毁
    in          p_server                        指向服务器对象
    out
    ret         errCode
*/
ERR_CODE server_destory(IN server_t *p_server)
{
    connect_t *ptr = NULL;
    connect_t *ptr_next = NULL;

    PFM_ENSURE_RET(NULL != p_server, ERR_BAD_PARAM);

    /* 销毁线程池 */
    thread_pool_destroy(&(p_server->thread_pool));
    DBG("destory thread_pool");

    /* 释放连接内存，关闭连接 */
    if(0 != p_server->connect_count)
    {
        ptr = p_server->connect_head.next;
        while(ptr)
        {
            ptr_next = ptr->next;
            close(ptr->fd);
            DBG("close connect fd %d", ptr->fd);
            free(ptr);
            ptr = ptr_next;
        }
    }
    p_server->connect_count = 0;
    p_server->connect_head.fd = -1;
    p_server->connect_head.next = NULL;

    /* 关闭socket描述符 */
    if(-1 != p_server->socket_fd)
    {
        close(p_server->socket_fd);
        p_server->socket_fd = -1;
        DBG("close socket_fd");
    }

    /* 关闭epoll描述符 */
    if(-1 != p_server->epoll_fd)
    {
        close(p_server->epoll_fd);
        p_server->epoll_fd = -1;
        DBG("close epoll_fd");
    }

    /* 销毁互斥锁 */
    pthread_mutex_destroy(&(p_server->mutex));

    memset(p_server, 0, sizeof(server_t));  /* 清空服务器对象 */

    DBG_ALZ("server destory done");
    return ERR_NO_ERROR;
}

/*
    Main
*/

int main()
{
    struct sigaction sa = {};
    struct epoll_event events[SERVER_EPOLL_EVENT_SIZE] = {};
    int events_num = 0;
    int i = 0;

    PFM_ENSURE_RET(ERR_NO_ERROR == server_init(&server, SERVER_THREAD_POOL_SIZE, SERVER_THREAD_TASK_QUEUE_SIZE), ERR_SERVER_INIT);

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);    // 清空信号掩码
    sa.sa_flags = 0;

    // 注册信号处理
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        DBG_ERR("sigaction failed");
        server_destory(&server);
    }

    while(1)
    {
        /* 监听epoll事件 */
        events_num = epoll_wait(server.epoll_fd, events, SERVER_EPOLL_EVENT_SIZE, -1);
        for(i = 0; i < events_num; ++i)
        {
            if(events[i].data.fd == server.socket_fd)  /* 新连接 */
            {
                handler_new_connection(&server, events[i].data.fd);
            }
            else if(events[i].events & EPOLLIN)         /* 处理可读事件 */
            {
                handler_read_event(&server, events[i].data.fd);
            }
            else if(events[i].events & EPOLLRDHUP)  /* 处理连接关闭事件 */
            {
                DBG_ERR("client fd %d closed connection", events[i].data.fd);
                pthread_mutex_lock(&(server.mutex));  /* 锁定服务器互斥锁 */
                connect_list_del((void *)&(server_connect_t){.p_server = &server, .connect_fd = events[i].data.fd});
                pthread_mutex_unlock(&(server.mutex));  /* 解锁服务器互斥锁 */
                close(events[i].data.fd);  /* 关闭连接 */
                epoll_ctl(server.epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);  /* 从epoll中删除 */
            }
            else
            {
                DBG_ERR("unknown epoll event %d for fd %d", events[i].events, events[i].data.fd);
            }
        }
    }

    PFM_ENSURE_RET(ERR_NO_ERROR == server_destory(&server), ERR_SERVER_INIT);

    return 0;
}