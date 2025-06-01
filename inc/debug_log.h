#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

/*
    Include Files
*/

#include <stdio.h>

/*
    Defines
*/

#define DBG_ON  /* 控制debug行为 */

#ifdef DBG_ON
/* 普通DBG */
#define DBG(...) do { \
    printf("[DEBUG] %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\r\n"); \
} while(0)
#else
#define DBG(...)    do{}while(0)
#endif
#define DBG_ERR(...)    do { \
    printf("[DEBUG_ERROR] %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\r\n"); \
} while(0)

#define DBG_ALZ(...)    do { \
    printf("[DEBUG_ALZ] %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); \
    printf("\r\n"); \
} while(0)

#define PFM_ENSURE_RET(expression, ret)    do   {   \
    if(!(expression))   \
    {   \
        DBG_ERR("ensure expression fail");  \
        return ret; \
    }   \
}while(0)

/*
    Type defines
*/

typedef enum
{
    ERR_NO_ERROR = 0,   /* ok */
    ERR_BAD_PARAM,      /* 参数错误 */
    ERR_NO_MEMORY,      /* 内存不足 */

    ERR_THREAD_POOL_INIT = 100,     /* 线程池初始化失败 */

    ERR_FILE_OPEN = 200,      /* 文件打开失败 */
}ERR_CODE;

/*
    Function declarations
*/

/*
    function    写入调试日志，等级alz
    in
                format          格式化字符串
    out
    ret         errCode
*/
ERR_CODE debug_log_alz(const char *format);

#endif