#ifndef CLIENT_H
#define CLIENT_H

/*
    Include files
*/

#include "debug_log.h"

/*
    Typedefs
*/

typedef struct client_s
{
    int socket_fd;        /* 客户端socket文件描述符 */
}client_t;

/*
    Function declarations
*/

/*
    function    客户端对象初始化
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_init(client_t *p_client);

/*
    function    客户端对象销毁
    in          p_client                        指向客户端对象
    out
    ret         errCode
*/
ERR_CODE client_destroy(client_t *p_client);

#endif