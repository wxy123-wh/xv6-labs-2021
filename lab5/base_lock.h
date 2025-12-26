#ifndef BASE_LOCK_H
#define BASE_LOCK_H

#include <pthread.h>
#include <stdbool.h>

// Wrapper around pthread mutex used to protect shared structures.
typedef struct {
  pthread_mutex_t internal_mutex;
  const char *name;  // Optional lock name for debugging
} base_lock_t;

// Initialize a base lock.
void base_lock_init(base_lock_t *lock, const char *name);

// Acquire the lock.
void base_lock_acquire(base_lock_t *lock);

// Release the lock.
void base_lock_release(base_lock_t *lock);

// Destroy the lock.
void base_lock_destroy(base_lock_t *lock);

#endif  // BASE_LOCK_H
