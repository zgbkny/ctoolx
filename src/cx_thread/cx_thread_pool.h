/* @author:wangwei
 * @date:20150303
 */

#ifndef CTOOLX_CX_THREAD_POOL_H_
#define CTOOLX_CX_THREAD_POOL_H_

// include all header file which we need in thread pool


typedef struct {
    void                   *(*process)(void *);
    void                   *arg;
} cx_thread_pool_task_t;


typedef struct cx_thread_pool_s cx_thread_pool_t;

struct cx_thread_pool_s {
    pthread_mutex_t         lock;   // for thread pool
    pthread_mutex_t         lock_of_busy_count;
    pthread_cond_t          cond_of_not_full_queue;
    pthread_cond_t          cond_of_not_empty_queue;
    pthread_t              *threads;
    pthread_t               adjust_tid;
    cx_thread_pool_task_t  *task_queue;

    int                     min_num;
    int                     max_num;
    int                     live_num;
    int                     busy_num;
    int                     wait_exit_num;
    int                     queue_front;
    int                     queue_rear;
    int                     queue_size;
    int                     queue_max_size;
    bool                    shutdown;
};

/**
 * @function:create a new thread pool with specify value
 */
cx_thread_pool_t *cx_thread_pool_create(int thread_min_num, int thread_max_num, int queue_max_size);

/**
 * @function:add task to the pool's queue
 */
int cx_thread_pool_add(cx_thread_pool_t *pool, void *(*function)(void *arg), void *arg);

/**
 * @function:destroy the pool
 */
int cx_thread_pool_destroy(cx_thread_pool_t *pool);

/**
 * @function:get all threads num
 */
int cx_thread_pool_get_all_threads_num(cx_thread_pool_t *pool);

/**
 * @function:get the busy threads num
 */
int cx_thread_pool_get_busy_threads_num(cx_thread_pool_t *pool);



#endif /*CTOOLX_CX_THREAD_POOL_H_*/