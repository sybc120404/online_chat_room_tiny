/*
    Include files
*/

#include "thread_pool.h"
#include "debug_log.h"

/*
    Function definitions
*/

/*
    function    线程池初始化
    in
                p_pool            线程池指针
                thread_count    工作线程数量
    out
    ret         errCode
*/
ERR_CODE thread_pool_init(thread_pool_t *p_pool, int thread_count)
{
    /* 参数检查 */
    PFM_ENSURE_RET(NULL != p_pool, ERR_BAD_PARAM);
    PFM_ENSURE_RET(0 < thread_count, ERR_BAD_PARAM);

    /* 申请线程数组空间 */
    p_pool->pthreads = malloc(sizeof(pthread_t) * thread_count);
    if(NULL == p_pool->pthreads)
    {
        DBG_ERR("malloc for pthreads");
        goto err;
    }
    memset(p_pool->pthreads, 0, sizeof(pthread_t) * thread_count);
    p_pool->thread_count = thread_count;

    /* 初始化内存池锁 */
    pthread_mutex_init(&(p->pthread_mutex_t), NULL);

    p_pool->shutdown = 0;

    return ERR_NO_ERROR;
err:

    if(p_pool->pthreads)    free(p_pool->pthreads);
    return ERR_THREAD_POOL_INIT;
}