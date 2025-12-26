#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "semaphore.h"

#define NUM_THREADS 5
#define INCREMENTS 10

static semaphore_t test_sem;
static int shared_counter = 0;

static void *test_thread(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < INCREMENTS; i++) {
    sem_wait(&test_sem);

    int temp = shared_counter;
    usleep(1000);  // Simulate work
    shared_counter = temp + 1;
    printf("[Thread %d] Counter = %d\n", id, shared_counter);

    sem_signal(&test_sem);
    usleep(500);
  }
  return NULL;
}

int main(void) {
  printf("=== Testing semaphore implementation ===\n");
  sem_init(&test_sem, 1, "test_mutex");

  pthread_t threads[NUM_THREADS];
  int ids[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, test_thread, &ids[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("\nFinal counter: %d (expected %d)\n",
         shared_counter, NUM_THREADS * INCREMENTS);
  sem_print_status(&test_sem);

  sem_destroy(&test_sem);
  return 0;
}
