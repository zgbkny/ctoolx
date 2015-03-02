#include <cx_thread_pool.h>




/**
 * @function:create a new thread pool with specify value
 */
cx_thread_pool_t *cx_thread_pool_create(int thread_min_num, int thread_max_num, int queue_max_size)
{

}

/**
 * @function:add task to the pool's queue
 */
int cx_thread_pool_add(cx_thread_pool_t *pool, void *(*function)(void *arg), void *arg)
{

}

/**
 * @function:destroy the pool
 */
int cx_thread_pool_destroy(cx_thread_pool_t *pool)
{

}

/**
 * @function:get all threads num
 */
int cx_thread_pool_get_all_threads_num(cx_thread_pool_t *pool)
{

}

/**
 * @function:get the busy threads num
 */
int cx_thread_pool_get_busy_threads_num(cx_thread_pool_t *pool)
{

}