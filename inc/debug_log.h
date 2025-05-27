#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

/*
    Include Files
*/

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
}ERR_CODE;

#endif