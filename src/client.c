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
*/
ERR_CODE client_input(IN client_t *p_client)
{
    char buffer[BUFFER_SIZE] = {0};

    PFM_ENSURE_RET(NULL != p_client, ERR_BAD_PARAM);

    printf("Enter message to send: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        perror("fgets");
        DBG_ERR("fgets failed");
        return ERR_CLIENT_INPUT;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';  // 去掉换行符
    }
    if (send(p_client->socket_fd, buffer, strlen(buffer), 0) == -1) {
        perror("send");
        DBG_ERR("send failed");
        return ERR_CLIENT_INPUT;
    }

    return ERR_NO_ERROR;
}

/*
    Main
*/

int main()
{
    struct sigaction sa = {};

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