/*
    Include files
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>

#include "client.h"
#include "server.h"

/*
    Variables
*/

static client_t client = {};

/*
    Function definitions
*/

/*
    function    信号处理函数，用于中断时关闭客户端
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
        client_destroy(&client);
        exit(0);
    }
}

/*
    function    客户端上线，发送消息通知其他客户端，不做其他操作
    in          p_client    指向客户端对象
    out
    ret
*/
ERR_CODE client_online(IN client_t *p_client)
{
    msg_t msg = {};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    msg.protocol = MSG_TYPE_USER_ONLINE;
    msg.length = 0;  // 上线消息不需要数据

    // 发送上线消息
    if (send(p_client->socket_fd, (void*)&msg, sizeof(msg_t), 0) == -1) {
        perror("send");
        DBG_ERR("send failed");
        return ERR_CLIENT_INPUT;
    }

    DBG("client online");

    return ERR_NO_ERROR;
}

/*
    function    客户端下线，发送消息通知其他客户端，不做其他操作
    in          p_client    指向客户端对象
    out
    ret
*/
ERR_CODE client_offline(IN client_t *p_client)
{
    msg_t msg = {};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    msg.protocol = MSG_TYPE_USER_OFFLINE;
    msg.length = 0;  // 下线消息不需要数据

    // 发送下线消息
    if (send(p_client->socket_fd, (void*)&msg, sizeof(msg_t), 0) == -1) {
        perror("send");
        DBG_ERR("send failed");
        return ERR_CLIENT_INPUT;
    }

    DBG("client offline");

    return ERR_NO_ERROR;
}

/*
    function    客户端对象初始化
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_init(client_t *p_client)
{
    struct sockaddr_in server_addr = {};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    /* 创建客户端socket */
    p_client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == p_client->socket_fd)
    {
        perror("client socket create");
        DBG_ERR("create client socket failed");
        goto err;
    }
    DBG("client socket %d created", p_client->socket_fd);

    /* 连接服务器 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (-1 == connect(p_client->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("client connect");
        DBG_ERR("client connect failed");
        goto err;
    }

    DBG_ALZ("client connected to server %s:%d", "127.0.0.1", SERVER_PORT);

    return ERR_NO_ERROR;

err:

    DBG_ERR("client init failed");
    if (p_client->socket_fd != -1)
    {
        close(p_client->socket_fd);
        p_client->socket_fd = -1;
    }

    return ERR_CLIENT_INIT;
}

/*
    function    客户端对象销毁
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_destroy(client_t *p_client)
{
    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    client_offline(p_client);  // 发送下线消息
    
    if (p_client->socket_fd != -1)
    {
        close(p_client->socket_fd);
        p_client->socket_fd = -1;
        DBG("client socket %d closed", p_client->socket_fd);
    }

    DBG_ALZ("client destroyed");
    return ERR_NO_ERROR;
}

/*
    function    客户端注册
    in          p_client                        指向客户端对象
                user_name                      用户名
    out
    ret         errCode
*/
ERR_CODE client_register(IN client_t *p_client, IN const char *user_name)
{
    msg_t msg = {};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);
    PFM_ENSURE_RET(NULL != user_name && USER_NAME_SIZE > strlen(user_name), ERR_BAD_PARAM);

    msg.protocol = MSG_TYPE_USER_REGISTER;  // 设置消息类型为用户注册
    msg.length = strlen(user_name);
    memcpy(msg.data, user_name, msg.length);

    // 发送注册消息
    if (send(p_client->socket_fd, (void*)&msg, sizeof(msg_t), 0) == -1) {
        perror("send");
        DBG_ERR("send failed");
        return ERR_CLIENT_INPUT;
    }

    //client_online(&client);

    DBG("client registered with user name: %s", user_name);

    return ERR_NO_ERROR;
}

/*
    function    客户端输入消息
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_input(IN client_t *p_client)
{
    char buffer[BUFFER_SIZE - BUFFER_HEADER_SIZE] = {0};
    msg_t msg = {};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        perror("fgets");
        DBG_ERR("fgets failed");
        return ERR_CLIENT_INPUT;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';  // 去掉换行符
    }

    msg.protocol = MSG_TYPE_MSG;  // 设置消息类型
    msg.length = strlen(buffer);
    memcpy(msg.data, buffer, msg.length);

    // 发送消息
    if (send(p_client->socket_fd, (void*)&msg, sizeof(msg_t), 0) == -1) {
        perror("send");
        DBG_ERR("send failed");
        return ERR_CLIENT_INPUT;
    }

    return ERR_NO_ERROR;
}

/*
    function    客户端接受消息
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_receive(IN client_t *p_client)
{
    msg_t msg = {};
    ssize_t bytes_received = 0;

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    bytes_received = recv(p_client->socket_fd, (void*)&msg, sizeof(msg_t), 0);
    if (bytes_received == -1) {
        perror("recv");
        DBG_ERR("recv failed");
        return ERR_CLIENT_RECEIVE;
    } else if (bytes_received == 0) {
        DBG_ERR("server closed connection");
        PFM_ENSURE_RET(ERR_NO_ERROR == client_destroy(p_client), ERR_CLIENT_INIT);  /* 销毁客户端 */
    }

    DBG("client received message from server: %s", msg.data);

    switch(msg.protocol)
    {
        case MSG_TYPE_USER_OFFLINE:
        {
            printf(DBG_FMT_RED"%s\r\n"DBG_FMT_END, msg.data);
            break;
        }
        case MSG_TYPE_USER_ONLINE:
        {
            printf(DBG_FMT_GREEN"%s\r\n"DBG_FMT_END, msg.data);
            break;
        }
        case MSG_TYPE_MSG:
        {
            printf("%s\r\n", msg.data);
            break;
        }
        default:
        {
            return ERR_BAD_PARAM;
        }
    }

    return ERR_NO_ERROR;
}

static void* thread_worker(void *arg)
{
    while (1) {
        if (ERR_NO_ERROR != client_receive((client_t *)arg)) {
            DBG_ERR("client receive failed");
            break;
        }
    }

    return NULL;
}

/*
    Main
*/

int main(int argc, char *argv[])
{
    struct sigaction sa = {};

    PFM_ENSURE_RET(2 == argc, ERR_BAD_PARAM);
    PFM_ENSURE_RET(ERR_NO_ERROR == client_init(&client), ERR_CLIENT_INIT);

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);    // 清空信号掩码
    sa.sa_flags = 0;

    // 注册信号处理
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        DBG_ERR("sigaction failed");
        client_destroy(&client);
    }

    // 客户端注册
    if (ERR_NO_ERROR != client_register(&client, argv[1]))
    {
        DBG_ERR("client register failed");
        client_destroy(&client);
        return ERR_CLIENT_INIT;
    }

    /* 子线程处理服务端消息 */
    if (pthread_create(&client.thread_id, NULL, thread_worker, &client) != 0) {
        perror("pthread_create");
        DBG_ERR("create client thread failed");
        client_destroy(&client);
        return ERR_CLIENT_INIT;
    }

    printf("Hi [%s], weclome to tiny chat room.\r\n", argv[1]);
    while(1)
    {
        if(ERR_NO_ERROR != client_input(&client))
        {
            DBG_ERR("client input failed");
            break;
        }
    }

    PFM_ENSURE_RET(ERR_NO_ERROR == client_destroy(&client), ERR_CLIENT_INIT);

    return 0;
}