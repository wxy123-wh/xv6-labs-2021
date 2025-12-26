三、实验内容
3.1 实验架构
本实验采用分层设计：


┌─────────────────────────────────────┐
│  应用层：生产者-消费者问题                       │
├─────────────────────────────────────┤
同步层：信号量实现（学生完成）               │
─────────────────────────────────────┤



3.2 提供的代码框架
我们提供了基础锁的实现，但学生需要理解其原理：
文件：base_lock.h


#ifndef BASE_LOCK_H
#define BASE_LOCK_H
#include <pthread.h>
#include <stdbool.h>
/**
* 基础锁结构
* 注意：这是框架提供的基础设施，仅用于保护你实现的数据结构
* 你需要理解为什么需要这个锁，以及它在你的信号量实现中的作用*/
typedef struct {
pthread_mutex_t internal_mutex;
const char *name;  // 用于调试 } base_lock_t;
/**
* 初始化基础锁
* @param lock 锁指针
* @param name 锁的名称（用于调试输出） */
void base_lock_init(base_lock_t *lock, const char *name);
/**
* 获取锁
* @param lock 锁指针*/
void base_lock_acquire(base_lock_t *lock);
/**
* 释放锁
* @param lock 锁指针*/
void base_lock_release(base_lock_t *lock);
/**
* 销毁锁
* @param lock 锁指针*/
void base_lock_destroy(base_lock_t *lock);



文件：base_lock.c


#include "base_lock.h"
#include <stdio.h>
#include <stdlib.h>
void base_lock_init(base_lock_t *lock, const char *name) {
if (pthread_mutex_init(&lock->internal_mutex, NULL) != 0) {
fprintf(stderr, "Failed to initialize lock: %s\n", name); exit(1);
}
lock->name = name; }
void base_lock_acquire(base_lock_t *lock) {
pthread_mutex_lock(&lock->internal_mutex);
}
void base_lock_release(base_lock_t *lock) {
pthread_mutex_unlock(&lock->internal_mutex);
}
void base_lock_destroy(base_lock_t *lock) {
pthread_mutex_destroy(&lock->internal_mutex);
}

---
四、任务一：实现信号量机制
4.1 数据结构设计
在 semaphore.h 中定义：



* 3. 等待队列：需要自己设计如何让线程等待和唤醒*/

typedef struct { int value;
base_lock_t lock;
pthread_cond_t cond; const char *name;


// 信号量的值
// 保护信号量结构
// 条件变量（用于线程等待/唤醒） // 信号量名称（调试用）

// 统计信息（用于实验验证）
int wait_count;              // 当前等待的线程数
long total_wait_count;       // 累计等待次数
long total_signal_count;     // 累计signal次数
} semaphore_t;
/**
* 初始化信号量
* @param sem 信号量指针
* @param value 初始值
* @param name 信号量名称*/
void sem_init(semaphore_t *sem, int value, const char *name);
/**
* P操作（wait/down）
* 功能：将信号量的值减1
*       如果结果小于0，则线程阻塞
*
* @param sem 信号量指针*/
void sem_wait(semaphore_t *sem);
/**
* V操作（signal/up）
* 功能：将信号量的值加1
*       如果有等待的线程，则唤醒一个*
* @param sem 信号量指针*/
void sem_signal(semaphore_t *sem);
/**
* 获取信号量当前值（仅用于调试和显示）
* @param sem 信号量指针
* @return 当前信号量值*/
int sem_get_value(semaphore_t *sem);
/**
* 打印信号量状态（用于调试）
* @param sem 信号量指针*/
void sem_print_status(semaphore_t *sem);
/**



4.2 实现要求
创建 semaphore.c 并实现上述函数：
关键实现提示

1. sem_init() 实现框架


void sem_init(semaphore_t *sem, int value, const char *name) { // TODO: 初始化信号量的值
// TODO: 初始化基础锁
// TODO: 初始化条件变量  pthread_cond_init()
// TODO: 初始化统计信息 }
2. sem_wait() 实现框架


void sem_wait(semaphore_t *sem) { base_lock_acquire(&sem->lock);
// TODO: 将信号量值减1
// TODO: 如果值小于0，需要等待
// 提示：使用  pthread_cond_wait(&sem->cond, &sem->lock.internal_mutex) // 思考：为什么要在循环中检查条件？（防止虚假唤醒）
base_lock_release(&sem->lock); }
3. sem_signal() 实现框架


void sem_signal(semaphore_t *sem) {
base_lock_acquire(&sem->lock);
// TODO: 将信号量值加1
// TODO: 如果有线程在等待，唤醒一个
// 提示：使用  pthread_cond_signal(&sem->cond)



4.3 实现要点
重要说明：
1. 使用 pthread_cond_t 是允许的，因为它相当于xv6中的sleep/wakeup机制，是底层的睡眠和唤醒原语。
重点是理解如何基于这些原语构建信号量语义。
2. 防止虚假唤醒：使用while循环而不是if：


while (条件不满足) {
pthread_cond_wait(...);
}
1. 原子性保证：所有对信号量value的操作必须在锁的保护下进行。
2. 调试输出：建议在关键操作处添加日志：


#define DEBUG 1
#if DEBUG
printf("[SEM] %s: wait, value=%d, tid=%ld\n", sem->name, sem->value, pthread_self());
#endif
4.4 测试信号量实现
创建 test_semaphore.c 测试你的实现：



int temp = shared_counter;
usleep(1000);  // 模拟处理时间
shared_counter = temp + 1;
printf("[Thread %d] Counter = %d\n", id, shared_counter);
sem_signal(&test_sem);
usleep(500); }

return NULL;
}
int main() {
printf("=== 测试信号量实现  ===\n");
// 初始化信号量为1（互斥锁）
sem_init(&test_sem, 1, "test_mutex");
pthread_t threads[NUM_THREADS];
int ids[NUM_THREADS];
// 创建线程
for (int i = 0; i < NUM_THREADS; i++) {
ids[i] = i;
pthread_create(&threads[i], NULL, test_thread, &ids[i]); }
// 等待线程结束
for (int i = 0; i < NUM_THREADS; i++) {
pthread_join(threads[i], NULL); }
printf("\n最终计数器值 : %d (期望值 : %d)\n",
shared_counter, NUM_THREADS * 10);
sem_print_status(&test_sem); sem_destroy(&test_sem);

return 0; }	







编译运行：


gcc -o test_sem base_lock.c semaphore.c test_semaphore.c -lpthread ./test_sem
验证标准：
.     ✅ 最终计数器值正确 .     ✅ 没有数据竞争

五、任务二：解决生产者-消费者问题
5.1 问题描述
实现一个有界缓冲区的生产者-消费者系统：
.  缓冲区大小： 10个槽位
.  生产者数量 ：3个
.  消费者数量 ：2个
.  每个生产者生产 ：20个物品
.   要求：正确同步，无数据丢失，无重复消费
5.2 代码框架
创建 producer_consumer.c：

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "semaphore.h"
// 配置参数
#define BUFFER_SIZE 10
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 20
// 物品结构
typedef struct {
int id;              // 物品ID
int producer_id;     // 生产者ID
double timestamp;    // 生产时间戳 } Item;
// 有界缓冲区结构typedef struct {
Item buffer[BUFFER_SIZE];

int	in;	// 生产者插入位置
int	out;	// 消费者取出位置
int	count;	// 当前缓冲区中的物品数
// TODO: 添加需要的信号量
// semaphore_t empty;   // 空槽位数


// semaphore_t full;    // 满槽位数
// semaphore_t mutex;   // 互斥访问缓冲区 } BoundedBuffer;
// 统计信息
typedef struct {
long produced[NUM_PRODUCERS];    // 每个生产者生产的数量
long consumed[NUM_CONSUMERS];    // 每个消费者消费的数量
long total_produced;
long total_consumed;
base_lock_t lock;                // 保护统计信息
} Statistics;
BoundedBuffer buffer;
Statistics stats;
int next_item_id = 0;
base_lock_t item_id_lock;
// 获取当前时间（秒）
double get_time() {
struct timeval tv;
gettimeofday(&tv, NULL);
return tv.tv_sec + tv.tv_usec / 1000000.0; }
// TODO: 实现初始化函数
void buffer_init() {
// 初始化缓冲区
// 初始化信号量
// 思考：empty、full、mutex的初始值应该是多少？ }
// TODO: 实现生产函数
void produce(Item item) { // 1. 等待空槽位
// 2. 获取互斥锁
// 3. 将物品放入缓冲区// 4. 更新in指针
// 5. 释放互斥锁
// 6. 增加满槽位信号量
// 打印状态
printf("[%.3f] [Producer-%d] Produced item #%d, buffer count: %d\n", get_time(), item.producer_id, item.id, buffer.count);
}
// TODO: 实现消费函数
Item consume(int consumer_id) { Item item;
// 1. 等待满槽位
// 2. 获取互斥锁
// 3. 从缓冲区取出物品


// 4. 更新out指针
// 5. 释放互斥锁
// 6. 增加空槽位信号量
// 打印状态
printf("[%.3f] [Consumer-%d] Consumed item #%d (from Producer-%d), buffer count: %d\n",
get_time(), consumer_id, item.id, item.producer_id, buffer.count);
return item; }
// 生产者线程
void* producer(void *arg) {
int id = *(int*)arg;
for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
// 创建物品
Item item;
base_lock_acquire(&item_id_lock);
item.id = next_item_id++;
base_lock_release(&item_id_lock);
item.producer_id = id;
item.timestamp = get_time();
// 生产物品
produce(item);
// 更新统计
base_lock_acquire(&stats.lock);
stats.produced[id]++;
stats.total_produced++;
base_lock_release(&stats.lock);
// 模拟生产时间
usleep(rand() % 50000 + 10000);  // 10-60ms }
printf("[Producer-%d] Finished. Total produced: %ld\n",
id, stats.produced[id]);
return NULL;
}
// 消费者线程
void* consumer(void *arg) {
int id = *(int*)arg;
int expected_total = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
while (1) {
// 检查是否所有物品都已生产
base_lock_acquire(&stats.lock);
int total_consumed_now = stats.total_consumed;
base_lock_release(&stats.lock);



if (total_consumed_now >= expected_total) {
break;
}
// 消费物品
Item item = consume(id);
// 更新统计
base_lock_acquire(&stats.lock);
stats.consumed[id]++;
stats.total_consumed++;
base_lock_release(&stats.lock);
// 模拟消费时间
usleep(rand() % 80000 + 20000);  // 20-100ms }
printf("[Consumer-%d] Finished. Total consumed: %ld\n",
id, stats.consumed[id]); return NULL;
}
// 打印最终统计
void print_statistics() {
printf("\n");
printf("================== 统计信息  ==================\n");
printf("生产者统计:\n");
for (int i = 0; i < NUM_PRODUCERS; i++) {
printf("  Producer-%d: %ld items\n", i, stats.produced[i]); }
printf("  总计 : %ld items\n", stats.total_produced);
printf("\n消费者统计:\n");
for (int i = 0; i < NUM_CONSUMERS; i++) {
printf("  Consumer-%d: %ld items\n", i, stats.consumed[i]); }
printf("  总计 : %ld items\n", stats.total_consumed);
printf("\n验证结果:\n");
if (stats.total_produced == stats.total_consumed &&
stats.total_produced == NUM_PRODUCERS * ITEMS_PER_PRODUCER) {

printf("  ✓	生产和消费数量匹配\n");
printf("  ✓	没有物品丢失\n");
} else {
printf("  ✗	错误：生产=%ld, 消费=%ld, 期望=%d\n",
stats.total_produced, stats.total_consumed, NUM_PRODUCERS * ITEMS_PER_PRODUCER);
}
printf("\n信号量状态:\n");  // TODO: 打印信号量的统计信息
printf("==============================================\n");


}
int main() {
srand(time(NULL));
printf("=== 生产者-消费者问题模拟  ===\n");
printf("配置 : %d个生产者, %d个消费者, 缓冲区大小=%d\n",
NUM_PRODUCERS, NUM_CONSUMERS, BUFFER_SIZE);
printf("每个生产者生产 %d 个物品\n\n", ITEMS_PER_PRODUCER);
// 初始化
buffer_init();
base_lock_init(&stats.lock, "stats_lock");
base_lock_init(&item_id_lock, "item_id_lock");
// 创建线程
pthread_t producers[NUM_PRODUCERS];
pthread_t consumers[NUM_CONSUMERS];
int producer_ids[NUM_PRODUCERS];
int consumer_ids[NUM_CONSUMERS];
double start_time = get_time();
// 启动消费者
for (int i = 0; i < NUM_CONSUMERS; i++) {
consumer_ids[i] = i;
pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]); }
// 启动生产者
for (int i = 0; i < NUM_PRODUCERS; i++) {
producer_ids[i] = i;
pthread_create(&producers[i], NULL, producer, &producer_ids[i]); }
// 等待生产者结束
for (int i = 0; i < NUM_PRODUCERS; i++) {
pthread_join(producers[i], NULL); }
// 等待消费者结束
for (int i = 0; i < NUM_CONSUMERS; i++) {
pthread_join(consumers[i], NULL); }
double end_time = get_time();
printf("\n总运行时间 : %.2f 秒\n", end_time - start_time);
print_statistics();
// 清理
// TODO: 销毁信号量
base_lock_destroy(&stats.lock);



5.3 实现要点
1. 信号量的使用


// 三个信号量的作用：
semaphore_t empty;  // 初始值  = BUFFER_SIZE，表示可用空槽位semaphore_t full;   // 初始值  = 0，表示已有物品数
semaphore_t mutex;  // 初始值  = 1，保证互斥访问缓冲区
2. 生产者的PV操作顺序


void produce(Item item) {
sem_wait(&empty);
sem_wait(&mutex);	// P(empty) - 等待空槽位// P(mutex) - 进入临界区
// 放入物品
buffer.buffer[buffer.in] = item;
buffer.in = (buffer.in + 1) % BUFFER_SIZE; buffer.count++;
sem_signal(&mutex);  // V(mutex) - 离开临界区
sem_signal(&full);   // V(full) - 增加满槽位 }
3. 消费者的PV操作顺序


Item consume(int consumer_id) {
sem_wait(&full);     // P(full) - 等待满槽位
sem_wait(&mutex);    // P(mutex) - 进入临界区
// 取出物品
Item item = buffer.buffer[buffer.out];
buffer.out = (buffer.out + 1) % BUFFER_SIZE; buffer.count--;
sem_signal(&mutex);  // V(mutex) - 离开临界区sem_signal(&empty);  // V(empty) - 增加空槽位
return item; }



思考题：
·   为什么必须先P(empty/full)，再P(mutex)？
.   如果顺序反了会发生什么？
5.4 测试场景
场景1：均衡速度


producer_delay: 10-60ms
consumer_delay: 20-100ms
场景2：快速生产者


producer_delay: 5-20ms
consumer_delay: 50-150ms // 观察：缓冲区经常满
场景3：快速消费者


producer_delay: 50-150ms
consumer_delay: 5-20ms // 观察：缓冲区经常空

---
六、任务三：可视化与分析
6.1 实时状态显示（可选）
创建一个简单的状态监控函数：




6.2 日志分析
运行程序并将输出重定向到文件：


./producer_consumer > log.txt
分析日志，回答：
1. 缓冲区达到满状态多少次？
2. 缓冲区空状态多少次？
3. 平均每个生产者被阻塞的时间？
4. 是否出现过死锁或数据不一致？
七、参考资料
1. xv6-RISCV源码
   GitHub: https://github.com/mit-pdos/xv6-riscv
  重点阅读: kernel/spinlock.c, kernel/proc.c
2. 教材参考
  《操作系统概念》第10版，第6-7章
3. 在线资源
   POSIX线程教程：https://hpc-tutorials.llnl.gov/posix/?locale=zh_CN
4. 调试工具


# 内存检查
valgrind --tool=helgrind ./producer_consumer
# 线程检查
valgrind --tool=drd ./producer_consumer

---
附录A：完整的Makefile



CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0 LDFLAGS = -lpthread
# 目标文件
TARGETS = test_semaphore producer_consumer
# 所有目标
all: $(TARGETS)

# 测试信号量
test_semaphore: base_lock.o semaphore.o test_semaphore.o
$(CC) $^ -o $@ $(LDFLAGS)

# 生产者消费者
producer_consumer: base_lock.o semaphore.o producer_consumer.o
$(CC) $^ -o $@ $(LDFLAGS)

# 编译规则
%.o: %.c
$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
rm -f *.o $(TARGETS)
# 运行测试
test: test_semaphore
./test_semaphore
# 运行主程序
run: producer_consumer
./producer_consumer
.PHONY: all clean test run

---
附录B：调试宏
在 debug.h 中：



// 调试宏
#define DEBUG_PRINT(level, fmt, ...) \ do { \
if (DEBUG_LEVEL >= level) { \
printf("[DEBUG][TID:%ld] " fmt "\n", \  pthread_self(), ##__VA_ARGS__); \
} \
} while(0)
#define DEBUG_SEM(sem, op) \
DEBUG_PRINT(2, "SEM %s: %s, value=%d, wait_count=%d", \ sem->name, op, sem->value, sem->wait_count)
#define DEBUG_BUFFER() \
DEBUG_PRINT(2, "Buffer: count=%d, in=%d, out=%d", \ buffer.count, buffer.in, buffer.out)
#endif // DEBUG_H	



