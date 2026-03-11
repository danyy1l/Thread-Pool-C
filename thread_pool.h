#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Thpool Thpool;

/**
 * @brief Initializes a new thread pool
 *
 * @param n_threads Number of available threads
 * @param capacity Max capacity of task queue
 * @return Created threadpool on success
 *         NULL on error
 */
Thpool *thpool_init(u64 n_threads, u64 capacity);

/**
 * @brief Adds a new task to the job queue
 *
 * @param tpool Pointer to thread pool
 * @param p_func_t Pointer to new task function
 * @param arg Pointer to args received by p_func_t
 * @return Returns 0 if successfully added task, -1 otherwise
 */
i32 thpool_add_task(Thpool *tpool, void (*p_func_t)(void *arg), void *arg);

/**
 * @brief Wait for all queued tasks to finish
 *
 * Wait for all tasks (queued and running to finish)
 * Once taskqueue is empty, calling thread will continue.
 *
 * @param tpool Pointer to threadpool
 * @return
 */
void thpool_wait(Thpool *tpool);

/**
 * @brief Pauses all threads immediately (idle and working)
 *
 * Once thpool_resume is called they return to previous state
 *
 * @param tpool Pointer to thread pool
 * @return
 */
void thpool_pause(Thpool *tpool);

/**
 * @brief Resumes all threads to their pre-pause state
 *
 * @param tpool Pointer to threadpool
 * @return
 */
void thpool_resume(Thpool *tpool);

/**
 * @brief Frees memory and destroys threadpool
 *
 * @param tpool Threadpool to destroy
 * @return
 */
void thpool_destroy(Thpool *tpool);

/**
 * @brief Returns number of working threads at calling time
 *
 * @param tpool Pointer to threadpool
 * @return Unsinged long with number of currently working threads
 */
u64 thpool_working_threads(Thpool *tpool);

/**
 * @brief Returns number of idle threads at calling time
 *
 * @param tpool Pointer to threadpool
 * @return Unsigned long with number of currently idle threads
 */
u64 thpool_free_threads(Thpool *tpool);

#ifdef __cplusplus
}
#endif

#endif
