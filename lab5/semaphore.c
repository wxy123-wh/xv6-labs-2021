#include "semaphore.h"

#include <stdio.h>
#include <stdlib.h>

void sem_init(semaphore_t *sem, int value, const char *name) {
  sem->value = value;
  sem->name = name;
  base_lock_init(&sem->lock, name);
  if (pthread_cond_init(&sem->cond, NULL) != 0) {
    fprintf(stderr, "Failed to initialize condition variable for %s\n",
            name ? name : "semaphore");
    exit(1);
  }
  sem->wait_count = 0;
  sem->total_wait_count = 0;
  sem->total_signal_count = 0;
}

void sem_wait(semaphore_t *sem) {
  base_lock_acquire(&sem->lock);
  while (sem->value == 0) {
    sem->wait_count++;
    sem->total_wait_count++;
    pthread_cond_wait(&sem->cond, &sem->lock.internal_mutex);
    sem->wait_count--;
  }
  sem->value--;
  base_lock_release(&sem->lock);
}

void sem_signal(semaphore_t *sem) {
  base_lock_acquire(&sem->lock);
  sem->value++;
  sem->total_signal_count++;
  if (sem->wait_count > 0) {
    pthread_cond_signal(&sem->cond);
  }
  base_lock_release(&sem->lock);
}

int sem_get_value(semaphore_t *sem) {
  base_lock_acquire(&sem->lock);
  int val = sem->value;
  base_lock_release(&sem->lock);
  return val;
}

void sem_print_status(semaphore_t *sem) {
  base_lock_acquire(&sem->lock);
  printf("[SEM %s] value=%d, wait_count=%d, total_wait=%ld, total_signal=%ld\n",
         sem->name ? sem->name : "unnamed",
         sem->value,
         sem->wait_count,
         sem->total_wait_count,
         sem->total_signal_count);
  base_lock_release(&sem->lock);
}

void sem_destroy(semaphore_t *sem) {
  pthread_cond_destroy(&sem->cond);
  base_lock_destroy(&sem->lock);
}
