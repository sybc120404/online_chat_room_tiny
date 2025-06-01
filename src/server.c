/*
    Include files
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "server.h"

/*
    Variables
*/

static server_t server = {};

/*
    Function Definitions
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
)
{
    int thread_pool_flag = 0;
    struct sockaddr_in server_addr;

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
    DBG_ALZ("create socket %d", p_server->socket_fd);

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
    DBG_ALZ("server bind socket %d to %d", p_server->socket_fd, SERVER_PORT);

    /* 监听socket */
    if(0 != listen(p_server->socket_fd, SERVER_CONNECT_SIZE))
    {
        DBG_ERR("listen socket failed");
        perror("socket listen");
        goto err;
    }
    DBG_ALZ("server listen socket %d", p_server->socket_fd);

    /* 初始化其他参数 */
    p_server->connect_head.fd = -1;
    p_server->connect_head.next = NULL;
    p_server->connect_count = 0;

    DBG_ALZ("server init done");
    return ERR_NO_ERROR;

err:
    if(thread_pool_flag)    thread_pool_destroy(&(p_server->thread_pool));
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
    DBG_ALZ("destory thread_pool");

    /* 释放连接内存，关闭连接 */
    if(0 != p_server->connect_count)
    {
        ptr = p_server->connect_head.next;
        while(ptr)
        {
            ptr_next = ptr->next;
            close(ptr->fd);
            DBG_ALZ("close connect fd %d", ptr->fd);
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
        DBG_ALZ("close socket_fd");
    }

    DBG_ALZ("server destory done");
    return ERR_NO_ERROR;
}

/*
    Main
*/

int main()
{
    PFM_ENSURE_RET(ERR_NO_ERROR == server_init(&server, SERVER_THREAD_POOL_SIZE, SERVER_THREAD_TASK_QUEUE_SIZE), ERR_SERVER_INIT);

    sleep(10);

    PFM_ENSURE_RET(ERR_NO_ERROR == server_destory(&server), ERR_SERVER_INIT);

    return 0;
}