#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "semaphore.h"

// Configuration parameters
#define BUFFER_SIZE 10
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 20

// Item structure
typedef struct {
  int id;           // Item ID
  int producer_id;  // Producer ID
  double timestamp; // Production time
} Item;

// Bounded buffer with semaphores
typedef struct {
  Item buffer[BUFFER_SIZE];
  int in;    // Insert position
  int out;   // Remove position
  int count; // Current items in buffer

  semaphore_t empty; // Available empty slots
  semaphore_t full;  // Available full slots
  semaphore_t mutex; // Mutual exclusion for buffer
} BoundedBuffer;

// Statistics
typedef struct {
  long produced[NUM_PRODUCERS];
  long consumed[NUM_CONSUMERS];
  long total_produced;
  long total_consumed;

  // Instrumentation for task 3
  long full_hits;   // times buffer reached full
  long empty_hits;  // times buffer became empty
  double total_prod_block_time;  // seconds producers waited on empty
  double total_cons_block_time;  // seconds consumers waited on full
  long prod_block_events;
  long cons_block_events;

  base_lock_t lock; // Protects statistics
} Statistics;

static BoundedBuffer buffer;
static Statistics stats;
static int next_item_id = 0;
static base_lock_t item_id_lock;
static volatile int monitor_running = 1;

// Get current time in seconds
static double get_time(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Initialize buffer and semaphores
static void buffer_init(void) {
  buffer.in = 0;
  buffer.out = 0;
  buffer.count = 0;

  sem_init(&buffer.empty, BUFFER_SIZE, "empty");
  sem_init(&buffer.full, 0, "full");
  sem_init(&buffer.mutex, 1, "mutex");

  for (int i = 0; i < NUM_PRODUCERS; i++) {
    stats.produced[i] = 0;
  }
  for (int i = 0; i < NUM_CONSUMERS; i++) {
    stats.consumed[i] = 0;
  }
  stats.total_produced = 0;
  stats.total_consumed = 0;
  stats.full_hits = 0;
  stats.empty_hits = 0;
  stats.total_prod_block_time = 0.0;
  stats.total_cons_block_time = 0.0;
  stats.prod_block_events = 0;
  stats.cons_block_events = 0;
}

// Produce an item into the buffer
static void produce(Item item) {
  double t0 = get_time();
  sem_wait(&buffer.empty);  // Wait for empty slot
  double waited = get_time() - t0;
  if (waited > 0) {
    base_lock_acquire(&stats.lock);
    stats.total_prod_block_time += waited;
    stats.prod_block_events++;
    base_lock_release(&stats.lock);
  }
  sem_wait(&buffer.mutex);  // Enter critical section

  buffer.buffer[buffer.in] = item;
  buffer.in = (buffer.in + 1) % BUFFER_SIZE;
  buffer.count++;

  if (buffer.count == BUFFER_SIZE) {
    base_lock_acquire(&stats.lock);
    stats.full_hits++;
    base_lock_release(&stats.lock);
  }

  sem_signal(&buffer.mutex); // Leave critical section
  sem_signal(&buffer.full);  // Signal full slot available

  printf("[%.3f] [Producer-%d] Produced item #%d, buffer count: %d\n",
         get_time(), item.producer_id, item.id, buffer.count);
}

// Consume an item from the buffer
static Item consume(int consumer_id) {
  double t0 = get_time();
  sem_wait(&buffer.full);   // Wait for available item
  double waited = get_time() - t0;
  if (waited > 0) {
    base_lock_acquire(&stats.lock);
    stats.total_cons_block_time += waited;
    stats.cons_block_events++;
    base_lock_release(&stats.lock);
  }
  sem_wait(&buffer.mutex);  // Enter critical section

  Item item = buffer.buffer[buffer.out];
  buffer.out = (buffer.out + 1) % BUFFER_SIZE;
  buffer.count--;

  if (buffer.count == 0) {
    base_lock_acquire(&stats.lock);
    stats.empty_hits++;
    base_lock_release(&stats.lock);
  }

  sem_signal(&buffer.mutex); // Leave critical section
  sem_signal(&buffer.empty); // Signal empty slot available

  printf("[%.3f] [Consumer-%d] Consumed item #%d (from Producer-%d), buffer count: %d\n",
         get_time(), consumer_id, item.id, item.producer_id, buffer.count);
  return item;
}

// Producer thread
static void* producer(void *arg) {
  int id = *(int*)arg;
  for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
    Item item;
    base_lock_acquire(&item_id_lock);
    item.id = next_item_id++;
    base_lock_release(&item_id_lock);

    item.producer_id = id;
    item.timestamp = get_time();

    produce(item);

    base_lock_acquire(&stats.lock);
    stats.produced[id]++;
    stats.total_produced++;
    base_lock_release(&stats.lock);

    usleep(rand() % 50000 + 10000);  // 10-60ms
  }

  printf("[Producer-%d] Finished. Total produced: %ld\n",
         id, stats.produced[id]);
  return NULL;
}

// Consumer thread
static void* consumer(void *arg) {
  int id = *(int*)arg;
  int expected_total = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
  while (1) {
    base_lock_acquire(&stats.lock);
    int total_consumed_now = stats.total_consumed;
    base_lock_release(&stats.lock);

    if (total_consumed_now >= expected_total) {
      break;
    }

    Item item = consume(id);
    (void)item;  // item already reported inside consume

    base_lock_acquire(&stats.lock);
    stats.consumed[id]++;
    stats.total_consumed++;
    base_lock_release(&stats.lock);

    usleep(rand() % 80000 + 20000);  // 20-100ms
  }

  printf("[Consumer-%d] Finished. Total consumed: %ld\n",
         id, stats.consumed[id]);
  return NULL;
}

// Print final statistics
static void print_statistics(void) {
  printf("\n");
  printf("================== Statistics ==================\n");
  printf("Producers:\n");
  for (int i = 0; i < NUM_PRODUCERS; i++) {
    printf("  Producer-%d: %ld items\n", i, stats.produced[i]);
  }
  printf("  Total: %ld items\n", stats.total_produced);

  printf("\nConsumers:\n");
  for (int i = 0; i < NUM_CONSUMERS; i++) {
    printf("  Consumer-%d: %ld items\n", i, stats.consumed[i]);
  }
  printf("  Total: %ld items\n", stats.total_consumed);

  printf("\nVerification:\n");
  if (stats.total_produced == stats.total_consumed &&
      stats.total_produced == NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
    printf("  ✓ Produced and consumed counts match\n");
    printf("  ✓ No items lost\n");
  } else {
    printf("  ✗ Mismatch: produced=%ld, consumed=%ld, expected=%d\n",
           stats.total_produced, stats.total_consumed,
           NUM_PRODUCERS * ITEMS_PER_PRODUCER);
  }

  printf("\nSemaphore status:\n");
  sem_print_status(&buffer.empty);
  sem_print_status(&buffer.full);
  sem_print_status(&buffer.mutex);

  printf("\nBuffer state events:\n");
  printf("  Full hits : %ld\n", stats.full_hits);
  printf("  Empty hits: %ld\n", stats.empty_hits);

  printf("\nBlocking stats:\n");
  printf("  Producer waits: events=%ld, total=%.3fs, avg=%.3fms\n",
         stats.prod_block_events,
         stats.total_prod_block_time,
         stats.prod_block_events ? (stats.total_prod_block_time / stats.prod_block_events) * 1000 : 0.0);
  printf("  Consumer waits: events=%ld, total=%.3fs, avg=%.3fms\n",
         stats.cons_block_events,
         stats.total_cons_block_time,
         stats.cons_block_events ? (stats.total_cons_block_time / stats.cons_block_events) * 1000 : 0.0);
  printf("==============================================\n");
}

// Monitor thread: periodically print live status (task 3 optional)
static void* monitor_thread(void *arg) {
  (void)arg;
  while (monitor_running) {
    usleep(500000);  // 0.5s
    int full_slots = sem_get_value(&buffer.full);
    int empty_slots = sem_get_value(&buffer.empty);

    base_lock_acquire(&stats.lock);
    long produced = stats.total_produced;
    long consumed = stats.total_consumed;
    base_lock_release(&stats.lock);

    printf("[MONITOR] buffer items=%d, empty slots=%d, produced=%ld, consumed=%ld\n",
           full_slots, empty_slots, produced, consumed);
  }
  return NULL;
}

int main(void) {
  srand((unsigned int)time(NULL));
  printf("=== Producer-Consumer Simulation ===\n");
  printf("Config: %d producers, %d consumers, buffer size %d\n",
         NUM_PRODUCERS, NUM_CONSUMERS, BUFFER_SIZE);
  printf("Each producer creates %d items\n\n", ITEMS_PER_PRODUCER);

  buffer_init();
  base_lock_init(&stats.lock, "stats_lock");
  base_lock_init(&item_id_lock, "item_id_lock");

  pthread_t producers[NUM_PRODUCERS];
  pthread_t consumers[NUM_CONSUMERS];
  pthread_t monitor;
  int producer_ids[NUM_PRODUCERS];
  int consumer_ids[NUM_CONSUMERS];

  double start_time = get_time();

  // Start monitor
  pthread_create(&monitor, NULL, monitor_thread, NULL);

  // Start consumers
  for (int i = 0; i < NUM_CONSUMERS; i++) {
    consumer_ids[i] = i;
    pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
  }

  // Start producers
  for (int i = 0; i < NUM_PRODUCERS; i++) {
    producer_ids[i] = i;
    pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
  }

  // Wait producers
  for (int i = 0; i < NUM_PRODUCERS; i++) {
    pthread_join(producers[i], NULL);
  }

  // Wait consumers
  for (int i = 0; i < NUM_CONSUMERS; i++) {
    pthread_join(consumers[i], NULL);
  }

  monitor_running = 0;
  pthread_join(monitor, NULL);

  double end_time = get_time();
  printf("\nTotal runtime: %.2f seconds\n", end_time - start_time);
  print_statistics();

  // Cleanup
  sem_destroy(&buffer.empty);
  sem_destroy(&buffer.full);
  sem_destroy(&buffer.mutex);
  base_lock_destroy(&stats.lock);
  base_lock_destroy(&item_id_lock);
  return 0;
}
