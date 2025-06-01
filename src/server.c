/*
    Include files
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
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
#if 0
    int thread_pool_flag = 0;
#endif

    PFM_ENSURE_RET(NULL != p_server, ERR_BAD_PARAM);
    PFM_ENSURE_RET(0 < server_thread_pool_size && 0 < server_thread_task_queue_size, ERR_BAD_PARAM);

    /* 初始化服务器线程池 */
    PFM_ENSURE_RET(ERR_NO_ERROR == thread_pool_init(&(p_server->thread_pool), server_thread_pool_size, server_thread_task_queue_size), ERR_SERVER_INIT);
#if 0
    thread_pool_flag = 1;
#endif
    DBG("server init thread pool with %d threads, %d tasks", server_thread_pool_size, server_thread_task_queue_size);

    /* 初始化其他参数 */
    p_server->socket_fd = -1;
    p_server->connect_head.fd = -1;
    p_server->connect_head.next = NULL;
    p_server->connect_count = 0;

    DBG_ALZ("server init done");
    return ERR_NO_ERROR;

#if 0
err:
    if(thread_pool_flag)    thread_pool_destroy(&(p_server->thread_pool));
    memset(p_server, 0, sizeof(server_t));

    return ERR_SERVER_INIT;
#endif
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