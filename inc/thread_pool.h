#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/*
    Include files
*/

#include <pthread.h>
#include "debug_log.h"

/*
    Typedefs    
*/

typedef struct thread_pool_s
{
    pthread_t *pthreads;  /* 线程数组 */
    int thread_count;     /* 线程数量 */

    int shutdown;           /* 销毁标志 */

    pthread_mutex_t mutex;     /* 内存池锁 */
}thread_pool_t;

/*
    Function declarations
*/

/*
    function    线程池初始化
    in
                p_pool            线程池指针
                thread_count    工作线程数量
    out
    ret         errCode
*/
ERR_CODE thread_pool_init(thread_pool_t *p_pool, int thread_count);

#endif