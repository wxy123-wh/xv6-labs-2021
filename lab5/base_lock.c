#include "base_lock.h"

#include <stdio.h>
#include <stdlib.h>

void base_lock_init(base_lock_t *lock, const char *name) {
  if (pthread_mutex_init(&lock->internal_mutex, NULL) != 0) {
    fprintf(stderr, "Failed to initialize lock: %s\n",
            name ? name : "unnamed");
    exit(1);
  }
  lock->name = name;
}

void base_lock_acquire(base_lock_t *lock) {
  pthread_mutex_lock(&lock->internal_mutex);
}

void base_lock_release(base_lock_t *lock) {
  pthread_mutex_unlock(&lock->internal_mutex);
}

void base_lock_destroy(base_lock_t *lock) {
  pthread_mutex_destroy(&lock->internal_mutex);
}
