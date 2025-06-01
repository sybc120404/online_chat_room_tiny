/*
    Include files
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread_pool.h"
#include "debug_log.h"

/*
    Function definitions
*/

static void *thread_worker(void *p_pool)
{
    task_t *p_task = NULL;
    thread_pool_t *pool = NULL;

    PFM_ENSURE_RET(NULL != p_pool, NULL);
    pool = (thread_pool_t *)p_pool;

    while(1)    /* 循环处理任务 */
    {
        pthread_mutex_lock(pool->mutex);  /* 锁定线程池 */

        while(pool->task_queue->next == NULL && !pool->shutdown)
        {
            /* 等待任务条件变量 */
            pthread_cond_wait(pool->cond, pool->mutex);
        }

        /* 线程池销毁标志1 */
        if(pool->shutdown)
        {
            DBG_ALZ("Thread %ld exit for destroy thread pool", pthread_self());
            pthread_mutex_unlock(pool->mutex);
            break;
        }

        /* 取出任务 */
        p_task = pool->task_queue->next;
        pool->task_queue->next = p_task->next;
        pthread_mutex_unlock(pool->mutex);

        /* 执行任务，负责free */
        if(p_task && p_task->task_func)
        {
            DBG_ALZ("Thread %ld executing task", pthread_self());
            p_task->task_func(p_task->arg);
            free(p_task);
        }
    }

    return NULL;
}

/*
    function    线程池初始化
    in
                p_pool              线程池指针
                thread_count        工作线程数量
                task_queue_size     任务队列大小
    out
    ret         errCode
*/
ERR_CODE thread_pool_init(thread_pool_t *p_pool, int thread_count, int task_queue_size)
{
    int i = 0;

    /* 参数检查 */
    PFM_ENSURE_RET(NULL != p_pool, ERR_BAD_PARAM);
    PFM_ENSURE_RET(0 < thread_count, ERR_BAD_PARAM);
    PFM_ENSURE_RET(0 < task_queue_size, ERR_BAD_PARAM);

    /* 申请线程数组空间 */
    p_pool->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if(NULL == p_pool->pthreads)
    {
        DBG_ERR("malloc for pthreads");
        goto err;
    }
    memset(p_pool->pthreads, 0, sizeof(pthread_t) * thread_count);
    p_pool->free_thread_count = thread_count;

    /* 申请线程池锁空间 */
    p_pool->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if(NULL == p_pool->mutex)
    {
        DBG_ERR("malloc for pthread mutex");
        goto err;
    }
    /* 初始化线程池锁 */
    pthread_mutex_init(p_pool->mutex, NULL);

    /* 申请线程池条件变量空间 */
    p_pool->cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if(NULL == p_pool->cond)
    {
        DBG_ERR("malloc for pthread cond");
        goto err;
    }
    /* 初始化线程池条件变量 */
    pthread_cond_init(p_pool->cond, NULL);

    /* 任务队列，使用链表实现，线程池维护一个头节点 */
    p_pool->task_queue = (task_t *)malloc(sizeof(task_t));
    if(NULL == p_pool->task_queue)
    {
        DBG_ERR("malloc for task queue");
        goto err;
    }
    memset(p_pool->task_queue, 0, sizeof(task_t));

    p_pool->shutdown = 0;       /* 销毁标志0 */

    /* 线程池线程工作 */
    for(i = 0; i < thread_count; ++ i)
    {
        /* 创建线程 */
        if(pthread_create(&p_pool->pthreads[i], NULL, thread_worker, (void*)p_pool) != 0)
        {
            DBG_ERR("create thread %d failed", i);
            goto err;
        }
    }

    DBG_ALZ("create thread pool success");

    return ERR_NO_ERROR;
err:

    if(p_pool->pthreads)    free(p_pool->pthreads);
    if(p_pool->mutex)       free(p_pool->mutex);
    if(p_pool->task_queue)  free(p_pool->task_queue);
    if(p_pool->cond)        free(p_pool->cond);

    for(i = 0; i < thread_count; ++i)
    {
        if(p_pool->pthreads[i])
        {
            pthread_cancel(p_pool->pthreads[i]);  // 取消已创建的线程
        }
    }

    memset(p_pool, 0, sizeof(thread_pool_t));

    return ERR_THREAD_POOL_INIT;
}

/*
    function    线程池销毁
    in
                p_pool              线程池指针
    out
    ret         errCode
*/
ERR_CODE thread_pool_destroy(thread_pool_t *p_pool)
{
    PFM_ENSURE_RET(NULL != p_pool, ERR_BAD_PARAM);

    if(p_pool->shutdown)
    {
        DBG_ERR("thread pool already shutdown");
        return ERR_NO_ERROR;  /* 已经销毁 */
    }

    /* 设置销毁标志 */
    p_pool->shutdown = 1;

    /* 唤醒所有线程 */
    pthread_cond_broadcast(p_pool->cond);

    for(int i = 0; i < p_pool->free_thread_count; ++i)
    {
        if(p_pool->pthreads[i])
        {
            pthread_join(p_pool->pthreads[i], NULL);  /* 等待线程结束 */
        }
    }

    /* 释放资源 */
    if(p_pool->pthreads)    free(p_pool->pthreads);
    if(p_pool->mutex)       free(p_pool->mutex);
    if(p_pool->task_queue)  free(p_pool->task_queue);
    if(p_pool->cond)        free(p_pool->cond);
    memset(p_pool, 0, sizeof(thread_pool_t));  /* 清空线程池 */

    DBG_ALZ("thread pool destroyed");

    return ERR_NO_ERROR;
}

/*
    function    线程池添加任务
    in
                p_pool              线程池指针
                task_func           任务函数
                arg                 任务函数参数
    out
    ret         errCode
*/
ERR_CODE thread_pool_add_task(thread_pool_t *p_pool, void (*task_func)(void *arg), void *arg)
{
    task_t *p_task = NULL;

    /* 参数检查 */
    PFM_ENSURE_RET(NULL != p_pool, ERR_BAD_PARAM);
    PFM_ENSURE_RET(NULL != task_func, ERR_BAD_PARAM);

    /* 申请任务空间 */
    p_task = (task_t *)malloc(sizeof(task_t));
    if(NULL == p_task)
    {
        DBG_ERR("malloc for task");
        return ERR_NO_MEMORY;
    }
    memset(p_task, 0, sizeof(task_t));

    /* 设置任务 */
    p_task->task_func = task_func;
    p_task->arg = arg;

    /* 锁定线程池 */
    pthread_mutex_lock(p_pool->mutex);

    /* 将任务添加到队列 */
    p_task->next = p_pool->task_queue->next;
    p_pool->task_queue->next = p_task;

    /* 唤醒一个线程处理任务 */
    pthread_cond_signal(p_pool->cond);

    pthread_mutex_unlock(p_pool->mutex);

    DBG_ALZ("task added to thread pool");

    return ERR_NO_ERROR;
}

/*

========================

*/

//#define THREAD_POOL_TEST

#ifdef THREAD_POOL_TEST

void test_task(void *arg)
{
    int id = *(int *)arg;
    DBG_ALZ("Task %d is running by Thread %ld", id, pthread_self());
    sleep(1);  // 模拟任务执行时间
    DBG_ALZ("Task %d completed", id);
    return;
}

int main()
{
    thread_pool_t pool = {};
    thread_pool_init(&pool, 5, 10);

    for(int i = 0; i < 10; ++i)
    {
        int *task_id = (int *)malloc(sizeof(int));
        *task_id = i;
        thread_pool_add_task(&pool, test_task, task_id);
    }

    thread_pool_destroy(&pool);

    return 0;
}
#endif