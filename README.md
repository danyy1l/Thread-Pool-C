# C Thread Pool

A robust, efficient, and fully generic thread pool implementation in pure C using POSIX threads (`pthreads`). Designed for high-performance parallel processing, focusing on zero-polling efficiency and strict thread safety.

## Features

* **100% Generic:** Accepts any task using generic function pointers `void (*func)(void *arg)`. The pool is agnostic to the underlying data structures.
* **Zero Polling:** Idle threads consume exactly 0% CPU. The system relies on POSIX condition variables (`pthread_cond_t`) for passive waking and sleeping.
* **Thread-Safe:** Internally protected against race conditions and memory corruption using precise mutex locking (`pthread_mutex_t`).
* **Strong Encapsulation:** Built using the Opaque Pointer pattern. Internal structures are hidden from the user, preventing accidental state modification.
* **Graceful Shutdown:** Ensures the task queue is emptied and all threads are safely joined and memory freed before exiting, guaranteeing zero memory leaks.
* **State Control:** Supports pausing (`thpool_pause`) and resuming (`thpool_resume`) the worker threads dynamically.

## Internal Architecture

The core is a bounded FIFO Circular Ring Buffer. The main thread (Producer) pushes tasks into the queue, while a predefined number of worker threads (Consumers) concurrently pop and execute them. All synchronization logic is handled internally.

## Build & Run

The project includes a strict `Makefile` configured with high-standard compiler flags (`-Wall -Wextra -pedantic -O3 -pthread`).

```bash
# Compile the entire project
make all

# Run the default benchmark
make run

# Clean compiled objects and binaries
make clean
```

## Quick Start
```c
#include <stdio.h>
#include "thread_pool.h"

void worker_func(void *arg) {
    int *val = (int *)arg;
    printf("Task processed: %d\n", *val);
}

int main(void) {
    // 8 threads, 1000 tasks capacity
    Thpool *pool = thpool_init(8, 1000);

    int data = 42;
    thpool_add_task(pool, worker_func, &data);

    thpool_wait(pool);
    thpool_destroy(pool);
    
    return 0;
}
```

## Public API Reference

| Function | Description |
|:---|:---|
| `thpool_init` | Initializes the pool with `n_threads` and a fixed `capacity` queue. |
| `thpool_add_task` | Adds a function and its argument to the job queue. Returns 0 on success. |
| `thpool_wait` | Blocks the caller until the queue is empty and all active tasks finish. |
| `thpool_pause` | Immediately prevents workers from picking up new tasks from the queue. |
| `thpool_resume` | Signals all paused threads to resume work. |
| `thpool_destroy` | Triggers shutdown, joins all threads, and deallocates all internal memory. |
| `thpool_working_threads` | Returns the current count of threads actively executing a task. |
| `thpool_free_threads` | Returns the count of idle threads currently waiting for work. |

### Function Signatures

```c
Thpool *thpool_init(u64 n_threads, u64 capacity);
i32 thpool_add_task(Thpool *tpool, void (*p_func_t)(void *arg), void *arg);
void thpool_wait(Thpool *tpool);
void thpool_pause(Thpool *tpool);
void thpool_resume(Thpool *tpool);
void thpool_destroy(Thpool *tpool);
u64 thpool_working_threads(Thpool *tpool);
u64 thpool_free_threads(Thpool *tpool);
