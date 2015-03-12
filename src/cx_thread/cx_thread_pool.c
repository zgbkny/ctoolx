#include <cx_thread_pool.h>

#define dd printf

#define DEFAULT_TIME         1
#define DEFAULT_THREAD_VARY  1
#define MIN_WAIT_TASK_NUM    1

static void *cx_thread_pool_worker(void *thread_pool);

static void *cx_thread_pool_adjust(void *thread_pool);

static bool is_thread_alive(pthread_t tid);


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
        if ((pool = (cx_thread_pool_t *)malloc(sizeof(cx_thread_pool_t))) == NULL) {
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
            pthread_create(&(pool->threads[i]), NULL, cx_thread_pool_worker, (void *)pool);
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
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) {
        // queue full wait
        pthread_cond_wait(&(pool->cond_of_not_full_queue), &(pool->lock));
    }

    // add a task to queue
    if (pool->task_queue[pool->queue_rear].arg != NULL) {
        free(pool->task_queue[pool->queue_rear].arg);
        pool->task_queue[pool->queue_rear].arg = NULL;
    }
    pool->task_queue[pool->queue_rear].process = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
    pool->queue_size++;

    // queue not empty now
    pthread_cond_signal(&(pool->cond_of_not_empty_queue));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

/**
 * @function:destroy the pool
 */
int cx_thread_pool_destroy(cx_thread_pool_t *pool)
{
    int          i = 0;
    if (pool == NULL) {
        return -1;
    }

    pool->shutdown = TRUE;

    // join adjust thread
    pthread_join(pool->adjust_tid, NULL);

    // wake up the waiting thread
    pthread_cond_broadcast(&(pool->cond_of_not_empty_queue));
    for (i = 0; i < pool->max_num; i++) {
        if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
            pthread_join(pool->threads[i], NULL);
        }
    }
    cx_thread_pool_free(pool);
    return 0;
}

/**
 * @function:get all threads num
 */
int cx_thread_pool_get_all_threads_num(cx_thread_pool_t *pool)
{
    int             all_count_num = 0;
    pthread_mutex_lock(&(pool->lock));
    all_count_num = pool->live_num;
    pthread_mutex_unlock(&(pool->lock));
    return all_count_num;
}

/**
 * @function:get the busy threads num
 */
int cx_thread_pool_get_busy_threads_num(cx_thread_pool_t *pool)
{
    int             all_count_num = 0;
    pthread_mutex_lock(&(pool->lock));
    all_count_num = pool->busy_num;
    pthread_mutex_unlock(&(pool->lock));
    return all_count_num;
}


static void *cx_thread_pool_worker(void *thread_pool)
{
    cx_thread_pool_t            *pool = (cx_thread_pool_t *)thread_pool;
    cx_thread_pool_task_t        task;
    while (TRUE) {
        /*if we need to wait must lock*/
        pthread_mutex_lock(&(pool->lock));
        while ((pool->queue_size == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->cond_of_not_empty_queue), &(pool->lock));
            if (pool->wait_exit_num > 0) {
                pool->wait_exit_num--;
                if (pool->live_num > pool->min_num) {
                    pool->live_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }
        }

        // shutdown
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // get a task from queue
        task.process = pool->task_queue[pool->queue_front].process;
        task.arg = pool->task_queue[pool->queue_front].arg;
        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
        pool->queue_size--;

        // now queue must be not full
        pthread_cond_broadcast(&(pool->cond_of_not_full_queue));
        pthread_mutex_unlock(&(pool->lock));

        // work now
        pthread_mutex_lock(&(pool->lock_of_busy_count));
        pool->busy_num++;
        pthread_mutex_unlock(&(pool->lock_of_busy_count));
        
        (*(task.process))(task.arg);

        pthread_mutex_lock(&(pool->lock_of_busy_count));
        pool->busy_num--;
        pthread_mutex_unlock(&(pool->lock_of_busy_count));
    }

    pthread_exit(NULL);
    return NULL;
}

static void *cx_thread_pool_adjust(void *thread_pool)
{
    cx_thread_pool_t            *pool = (cx_thread_pool_t *)thread_pool;
    int                          queue_size = 0;
    int                          live_num = 0;
    int                          busy_num = 0;
    int                          i = 0, add = 0;


    while (!pool->shutdown) {
        sleep(DEFAULT_TIME);
        pthread_mutex_lock(&(pool->lock));
        queue_size = pool->queue_size;
        live_num = pool->live_num;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->lock_of_busy_count));
        busy_num = pool->busy_num;
        pthread_mutex_unlock(&(pool->lock_of_busy_count));

        if (queue_size >= MIN_WAIT_TASK_NUM && live_num < pool->max_num) {
            // add thread
            pthread_mutex_lock(&(pool->lock));
            for (i = 0; i < pool->max_num && add < DEFAULT_THREAD_VARY
                        && pool->live_num < pool->max_num; i++) {
                if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
                    pthread_create(&(pool->threads[i]), NULL, cx_thread_pool_worker, (void *)pool);
                    add++;
                    pool->live_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        if ((busy_num * 2) < live_num && live_num > pool->min_num) {
            // del thread
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_num = DEFAULT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));
            // wake up thread to exit
            for (i = 0; i < DEFAULT_THREAD_VARY; i++) {
                pthread_cond_signal(&(pool->cond_of_not_empty_queue));
            }
        }
    }
}

static bool is_thread_alive(pthread_t tid)
{
    int     kill_rc = pthread_kill(tid, 0);
    if (kill_rc == ESRCH) {
        return FALSE;
    }
    return TRUE;
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