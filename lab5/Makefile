CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0
LDFLAGS = -lpthread

TARGETS = test_semaphore producer_consumer

all: $(TARGETS)

test_semaphore: base_lock.o semaphore.o test_semaphore.o
	$(CC) $^ -o $@ $(LDFLAGS)

producer_consumer: base_lock.o semaphore.o producer_consumer.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
