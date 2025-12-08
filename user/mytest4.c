// user/test_mlfq.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 模拟 CPU 密集型任务
void cpu_bound(const char *tag, int loops) {
  volatile int x = 0;
  for (int i = 0; i < loops; i++) {
    x += i ^ (x << 1);
    if ((i % (loops/10 + 1)) == 0) {
      // 适当打印，观察调度切换
      printf("%s: working i=%d\n", tag, i);
    }
  }
  printf("%s: done x=%d\n", tag, x);
}

// 模拟交互型任务：短计算 + 短睡眠
void io_like(const char *tag, int rounds, int work_loops, int ms_sleep) {
  volatile int x = 0;
  for (int r = 0; r < rounds; r++) {
    for (int i = 0; i < work_loops; i++) {
      x += i ^ (x << 1);
    }
    printf("%s: round=%d sleep=%dms\n", tag, r, ms_sleep);
    // xv6 的 sleep 单位为 tick；若无 ms 版本，这里直接 sleep(1/2/5/10) 这些刻度即可
    sleep(ms_sleep); // 若 sleep 是 tick，则传较小整数，比如 2/5
  }
  printf("%s: done x=%d\n", tag, x);
}

int
main(int argc, char *argv[])
{
  printf("test_mlfq: start\n");

  int pid1 = fork();
  if (pid1 == 0) {
    // 子进程1：CPU 密集，长计算
    cpu_bound("cpu-bound[1]", 50*100000);
    exit(0);
  }

  int pid2 = fork();
  if (pid2 == 0) {
    // 子进程2：交互型，短计算+短睡眠
    io_like("io-like[2]", 40, 10000, 2);
    exit(0);
  }

  int pid3 = fork();
  if (pid3 == 0) {
    // 子进程3：交互型，工作更短、睡眠更频繁
    io_like("io-like[3]", 60, 3000, 1);
    exit(0);
  }

  int pid4 = fork();
  if (pid4 == 0) {
    // 子进程4：CPU 密集，稍短
    cpu_bound("cpu-bound[4]", 20*100000);
    exit(0);
  }

  // 父进程：等待所有子进程
  int st;
  wait(&st);
  wait(&st);
  wait(&st);
  wait(&st);

  printf("test_mlfq: done\n");
  exit(0);
}
