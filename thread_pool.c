#include "thread_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(DEBUG)
/**
 * @brief Imprimir error y terminar el programa
 *
 * @param msg Cadena con mensaje de error
 */
#define die(msg)                                                               \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)
#else
#define die(msg) ((void)0)
#endif

typedef void (*thread_task_t)(void *arg);

typedef struct _task {
  thread_task_t func; // Task function pointer
  void *arg;          // Task function arguments
} task;

typedef struct _taskqueue {
  task *task_list; // Pointer to task array
  u64 front;       // Front position in circular queue
  u64 rear;        // Rear position in circular queue
  u64 capacity;    // Maximum task capacity
} taskqueue;       // 32 bytes --> 0 padding

struct _Thpool {
  pthread_t *workers;       // Pointer to thread array
  u64 n_threads_alive;      // Total # of threads
  u64 n_working;            // # of working threads
  taskqueue task_queue;     // Task queue
  pthread_mutex_t lock;     // Lock
  pthread_cond_t work_cond; // Work Condition variable
  pthread_cond_t wait_cond; // Wait Condition variable
  u64 done;                 // Boolean for job end indication
  u64 pause;                // Boolean for work pause indication
}; // 208 total bytes, aligned to multiple of 8 --> 0 padding bytes

/*********************** FUNCTION DECLARATIONS ***************************/

/* JOBQUEUE */
static i32 taskqueue_init(taskqueue *tq, u64 capacity);
static i32 taskqueue_is_full(taskqueue *tq);
static i32 taskqueue_is_empty(taskqueue *tq);
static i32 taskqueue_push(taskqueue *tq, task new_task);
static i32 taskqueue_pop(taskqueue *tq, task *out);
static void taskqueue_clear(taskqueue *tq);
static void taskqueue_destroy(taskqueue *tq);

/* THREAD POOL */
static void *worker_thread(void *arg);

/*********************** TASK QUEUE IMPLEMENTATION ***************************/

static i32 taskqueue_init(taskqueue *tq, u64 capacity) {
  tq->front = 0;
  tq->rear = 0;
  tq->capacity = capacity + 1;

  tq->task_list = (task *)malloc(sizeof(task) * tq->capacity);
  if (tq->task_list == NULL)
    return ERR;

  return OK;
}

static i32 taskqueue_is_full(taskqueue *tq) {
  return (tq->rear + 1) % tq->capacity == tq->front;
}

static i32 taskqueue_is_empty(taskqueue *tq) { return tq->front == tq->rear; }

static i32 taskqueue_push(taskqueue *tq, task new_task) {
  if (taskqueue_is_full(tq))
    return ERR;

  tq->task_list[tq->rear] = new_task;
  tq->rear = (tq->rear + 1) % tq->capacity;

  return OK;
}

static i32 taskqueue_pop(taskqueue *tq, task *out) {
  if (taskqueue_is_empty(tq))
    return ERR;

  *out = tq->task_list[tq->front];
  tq->front = (tq->front + 1) % tq->capacity;
  return OK;
}

static void taskqueue_clear(taskqueue *tq) {
  task foo;
  while (taskqueue_pop(tq, &foo) == OK) {
    /* Nothing */
  }
}

static void taskqueue_destroy(taskqueue *tq) {
  if (tq == NULL)
    return;

  taskqueue_clear(tq);
  if (tq->task_list) {
    free(tq->task_list);
    tq->task_list = NULL;
  }
}

/*********************** THPOOL IMPLEMENTATION ***************************/

Thpool *thpool_init(u64 n_threads, u64 queue_capacity) {
  /* Make a new thread pool */
  Thpool *ptr = (Thpool *)malloc(sizeof(Thpool));
  if (ptr == NULL) {
    die("thpool_init(): Couldn't allocate memory for thread pool");
  }

  /* Create thread(worker) array  */
  ptr->workers = malloc(n_threads * sizeof(pthread_t));
  if (ptr->workers == NULL) {
    free(ptr);
    die("thpool_init(): Couldn't allocate memory for threads");
  }

  ptr->n_threads_alive = n_threads;
  ptr->n_working = 0;
  ptr->done = 0;
  ptr->pause = 0;

  /* Initialzie task queue, ring buffer sacrifices one position so we malloc 1
   * more space*/
  if (taskqueue_init(&(ptr->task_queue), queue_capacity) == ERR) {
    free(ptr->workers);
    free(ptr);
    die("thpool_init(): Couldn't initialize task queue");
  }

  /* Initialize lock, work and wait condition */
  pthread_mutex_init(&(ptr->lock), NULL);
  pthread_cond_init(&(ptr->work_cond), NULL);
  pthread_cond_init(&(ptr->wait_cond), NULL);

  /* Initialize all threads (create and detach) */
  for (u64 i = 0; i < n_threads; i++) {
    if (pthread_create(&(ptr->workers[i]), NULL, worker_thread, ptr) != 0) {
      free(ptr->workers);
      free(ptr);
      die("thpool_init(): Couldn't create threads");
    }
  }

  return ptr;
}

static void *worker_thread(void *arg) {
  Thpool *tpool = (Thpool *)arg;

  task my_task;

  while (1) {
    /* Thread closes mutex to lock external access to pool */
    pthread_mutex_lock(&(tpool->lock));

    /* If no task in queue but pool is active, unlock and put thread to sleep */
    while ((taskqueue_is_empty(&(tpool->task_queue)) || tpool->pause == 1) &&
           tpool->done == 0) {
      pthread_cond_wait(&(tpool->work_cond), &(tpool->lock));
    }

    /* If finished using pool, unlock and break loop */
    if (tpool->done == 1 && taskqueue_is_empty(&(tpool->task_queue))) {
      pthread_mutex_unlock(&(tpool->lock));
      break;
    }

    /* Have task: pop task, increment workers and unlock before starting work */
    if (taskqueue_pop(&(tpool->task_queue), &my_task) == OK) {
      tpool->n_working++;

      /* Free mutex for other threads to begin other work */
      pthread_mutex_unlock(&(tpool->lock));

      /* Begin task */
      if (my_task.func != NULL)
        my_task.func(my_task.arg);

      /* Finished task, close mutex again to begin next task */
      pthread_mutex_lock(&(tpool->lock));
      tpool->n_working--;

      if (tpool->n_working == 0 && taskqueue_is_empty(&(tpool->task_queue))) {
        pthread_cond_signal(&(tpool->wait_cond));
      }
    }

    pthread_mutex_unlock(&(tpool->lock));
  }

  return NULL;
}

i32 thpool_add_task(Thpool *tpool, void (*p_func_t)(void *arg), void *arg) {
  /* Lock before any interaction with task queue */
  pthread_mutex_lock(&(tpool->lock));

  if (tpool->done == 1) {
    pthread_mutex_unlock(&(tpool->lock));
    die("thpool_add_task(): pool is shutting down");
    return ERR;
  }

  if (taskqueue_is_full(&(tpool->task_queue))) {
    pthread_mutex_unlock(&(tpool->lock));
    die("thpool_add_task(): Task queue is full");
    return ERR;
  }

  task new_task;
  new_task.func = p_func_t;
  new_task.arg = arg;

  taskqueue_push(&(tpool->task_queue), new_task);

  /* Signal workers that are waiting that new task is to be completed */
  pthread_cond_signal(&(tpool->work_cond));

  pthread_mutex_unlock(&(tpool->lock));

  return OK;
}

void thpool_wait(Thpool *tpool) {
  pthread_mutex_lock(&(tpool->lock));
  while (!taskqueue_is_empty(&(tpool->task_queue)) || tpool->n_working) {
    pthread_cond_wait(&(tpool->wait_cond), &(tpool->lock));
  }

  pthread_mutex_unlock(&(tpool->lock));
}

void thpool_pause(Thpool *tpool) {
  pthread_mutex_lock(&(tpool->lock));
  tpool->pause = 1;
  pthread_mutex_unlock(&(tpool->lock));
}

void thpool_resume(Thpool *tpool) {
  pthread_mutex_lock(&(tpool->lock));
  tpool->pause = 0;
  /* Resume ALL threads with broadcast, signal only does for one */
  pthread_cond_broadcast(&(tpool->work_cond));
  pthread_mutex_unlock(&(tpool->lock));
}

void thpool_destroy(Thpool *tpool) {
  if (tpool == NULL)
    return;

  pthread_mutex_lock(&(tpool->lock));
  tpool->done = 1;
  pthread_cond_broadcast(&(tpool->work_cond));
  pthread_mutex_unlock(&(tpool->lock));

  for (u64 i = 0; i < tpool->n_threads_alive; i++) {
    pthread_join(tpool->workers[i], NULL);
  }

  taskqueue_destroy(&(tpool->task_queue));
  free(tpool->workers);
  pthread_mutex_destroy(&(tpool->lock));
  pthread_cond_destroy(&(tpool->wait_cond));
  pthread_cond_destroy(&(tpool->work_cond));
  free(tpool);
}

u64 thpool_working_threads(Thpool *tpool) {
  pthread_mutex_lock(&(tpool->lock));
  u64 out = tpool->n_working;
  pthread_mutex_unlock(&(tpool->lock));
  return out;
}

u64 thpool_free_threads(Thpool *tpool) {
  pthread_mutex_lock(&(tpool->lock));
  u64 out = tpool->n_threads_alive - tpool->n_working;
  pthread_mutex_unlock(&(tpool->lock));
  return out;
}
