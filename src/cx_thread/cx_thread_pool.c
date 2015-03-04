#include <cx_thread_pool.h>

#define dd printf


static void *cx_thread_pool_worker(void *thread_pool);

static void *cx_thread_pool_adjust(void *thread_pool);


static int cx_thread_pool_free(cx_thread_pool_t *pool);

/**
 * @function:create a new thread pool with specify value
 */
cx_thread_pool_t *cx_thread_pool_create(int thread_min_num, int thread_max_num, int queue_max_size)
{
    cx_thread_pool_t            *pool = NULL;
    size_t                       i = 0;
    // check args 

    do {
        if (pool = (cx_thread_pool_t *)malloc(sizeof(cx_thread_pool_t))) == NULL) {
            dd("malloc thread pool fail");
            break;
        }    
        pool->min_num = thread_min_num;
        pool->max_num = thread_max_num;
        pool->busy_num = 0;
        pool->live_num = thread_min_num;
        pool->queue_size = 0;
        pool->queue_max_size = queue_max_size;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = FALSE;

        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_max_num);
        if (pool->threads == NULL) {
            dd("malloc threads failed");
            break;
        }
        memset(pool->threads, 0, sizeof(pool->threads) * thread_max_num);

        pool->task_queue = (cx_thread_pool_task_t *)malloc(sizeof(cx_thread_pool_task_t) * queue_max_size);
        if (pool->task_queue == NULL) {
            dd("malloc task queue failed");
            break;
        }

        if (pthread_mutex_init(&(pool->lock), NULL) != 0
            || pthread_mutex_init(&(pool->lock_of_busy_count), NULL) != 0
            || pthread_cond_init(&(pool->cond_of_not_full_queue), NULL) != 0
            || pthread_cond_init(&(pool->cond_of_not_empty_queue), NULL) != 0) {
            dd("init the lock or cond fail");
            break;
        }

        for (i = 0; i < thread_min_num; i++) {
            pthread_create(&(pool->threads[i], NULL, cx_thread_pool_worker, (void *)pool));
        }

        pthread_create(&(pool->adjust_tid), NULL, cx_thread_pool_adjust, (void *)pool);
        return pool;

    } while (0);
    
    cx_thread_pool_free(pool);
    
    return NULL;
}

/**
 * @function:add task to the pool's queue
 */
int cx_thread_pool_add(cx_thread_pool_t *pool, void *(*function)(void *arg), void *arg)
{
    pthread_mutex_lock(&(pool->lock));
    
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


static void *cx_thread_pool_worker(void *thread_pool)
{

}

static void *cx_thread_pool_adjust(void *thread_pool)
{

}


static int cx_thread_pool_free(cx_thread_pool_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    if (pool->task_queue) {
        free(pool->task_queue);
    }

    if (pool->threads) {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->lock_of_busy_count));
        pthread_mutex_destroy(&(pool->lock_of_busy_count));
        pthread_cond_destroy(&(pool->cond_of_not_full_queue));
        pthread_cond_destroy(&(pool->cond_of_not_empty_queue));
    }
    free(pool);
    pool = NULL;
    return 0;
}