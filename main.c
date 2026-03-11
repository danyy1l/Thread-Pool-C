#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 17
#define NUM_TASKS 1000000

typedef struct {
  int id;
  long result;
} TaskData;

void compute_task(void *arg) {
  TaskData *data = (TaskData *)arg;
  long sum = 0;

  for (int i = 0; i < 10000; i++) {
    sum += (data->id + i);
  }

  data->result = sum;
}

int main(void) {
  printf("Initializing thread pool with %d threads...\n", NUM_THREADS);

  Thpool *pool = thpool_init(NUM_THREADS, NUM_TASKS);
  if (pool == NULL) {
    fprintf(stderr, "Failed to initialize thread pool.\n");
    return EXIT_FAILURE;
  }

  TaskData *tasks = malloc(sizeof(TaskData) * NUM_TASKS);
  if (tasks == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    thpool_destroy(pool);
    return EXIT_FAILURE;
  }

  printf("Submitting %d tasks to the pool...\n", NUM_TASKS);

  for (int i = 0; i < NUM_TASKS; i++) {
    tasks[i].id = i;
    tasks[i].result = 0;
    thpool_add_task(pool, compute_task, &tasks[i]);
  }

  printf("Waiting for tasks to complete...\n");
  thpool_wait(pool);

  int valid_results = 0;
  for (int i = 0; i < NUM_TASKS; i++) {
    if (tasks[i].result > 0 || tasks[i].id == 0) {
      valid_results++;
    }
  }

  printf("Successfully processed %d/%d tasks.\n", valid_results, NUM_TASKS);

  thpool_destroy(pool);
  free(tasks);

  printf("Resources freed. Exiting.\n");
  return EXIT_SUCCESS;
}
