#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pthread.h>

#include "base_lock.h"

// Counting semaphore with basic statistics for debugging.
typedef struct {
  int value;                  // Current semaphore value
  base_lock_t lock;           // Protects all fields below
  pthread_cond_t cond;        // Condition variable for waiting threads
  const char *name;           // Optional name for debugging

  // Statistics (for debugging/verification)
  int wait_count;             // Threads currently waiting
  long total_wait_count;      // Total times threads had to wait
  long total_signal_count;    // Total times signal was called
} semaphore_t;

// Initialize a semaphore.
void sem_init(semaphore_t *sem, int value, const char *name);

// P operation (wait/down).
void sem_wait(semaphore_t *sem);

// V operation (signal/up).
void sem_signal(semaphore_t *sem);

// Get current value (for debugging only).
int sem_get_value(semaphore_t *sem);

// Print semaphore status (for debugging).
void sem_print_status(semaphore_t *sem);

// Destroy semaphore resources.
void sem_destroy(semaphore_t *sem);

#endif  // SEMAPHORE_H
