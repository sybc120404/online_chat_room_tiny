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

/* 任务 */
typedef struct task_s
{
    void (*task_func)(void *arg);  /* 任务函数 */
    void *arg;                     /* 任务函数参数 */
    struct task_s* next;          /* 下一个任务指针 */
}task_t;

typedef struct thread_pool_s
{
    pthread_t *pthreads;  /* 线程数组 */
    int free_thread_count;     /* 线程数量 */

    task_t *task_queue;         /* 任务队列 */
    int task_queue_size;        /* 任务队列大小 */

    int shutdown;           /* 销毁标志 */

    pthread_mutex_t *mutex;    /* 线程池锁 */

    pthread_cond_t *cond;       /* 线程池任务条件变量 */
}thread_pool_t;

/*
    Function declarations
*/

/*
    function    线程池初始化
    in
                p_pool              线程池指针
                thread_count        工作线程数量
                task_queue_size     任务队列大小
    out
    ret         errCode
*/
ERR_CODE thread_pool_init(thread_pool_t *p_pool, int thread_count, int task_queue_size);

/*
    function    线程池销毁
    in
                p_pool              线程池指针
    out
    ret         errCode
*/
ERR_CODE thread_pool_destroy(thread_pool_t *p_pool);

/*
    function    线程池添加任务
    in
                p_pool              线程池指针
                task_func           任务函数
                arg                 任务函数参数
    out
    ret         errCode
*/
ERR_CODE thread_pool_add_task(thread_pool_t *p_pool, void (*task_func)(void *arg), void *arg);

#endif