#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

// 简易时间戳：用 uptime 近似（ticks）
static int now_ticks() {
  // 如果你的 xv6 提供 uptime() syscall，直接用；没有则用 sleep 的累积做近似
  // 这里用 sleep(0) 不会返回 ticks，故改为在内核中已有的 uptime。
  return uptime();
}

static void busy_work(int loops) {
  volatile int x = 0;
  for (int i = 0; i < loops; i++) {
    x = x * 1103515245 + 12345; // 假负载
  }
}

static void cpu_task(const char* tag, int rounds, int yield_every) {
  for (int i = 0; i < rounds; i++) {
    busy_work(50000); // CPU 密集
    if (i % yield_every == 0) {
      printf("[%d] %s: yield at i=%d\n", now_ticks(), tag, i);
      sleep(1);// 主动让出，MLQ 下会下调层级
    } else {
      // 也打印一下，以观察频率
      if (i % (yield_every/2 + 1) == 0) {
        printf("[%d] %s: running i=%d\n", now_ticks(), tag, i);
      }
    }
  }
  printf("[%d] %s: done\n", now_ticks(), tag);
}

static void io_task(const char* tag, int rounds, int sleep_ticks) {
  for (int i = 0; i < rounds; i++) {
    printf("[%d] %s: before sleep i=%d\n", now_ticks(), tag, i);
    sleep(sleep_ticks); // 阻塞，唤醒后应提升到高层
    // 唤醒后立即打印，观察是否抢占优先
    printf("[%d] %s: after wake i=%d\n", now_ticks(), tag, i);
  }
  printf("[%d] %s: done\n", now_ticks(), tag);
}

static void medium_task(const char* tag, int rounds, int sleep_ticks, int yield_every) {
  for (int i = 0; i < rounds; i++) {
    busy_work(20000);
    if (i % yield_every == 0) {
      printf("[%d] %s: yield i=%d\n", now_ticks(), tag, i);
      sleep(1); // 主动让出
    }
    if (i % 3 == 0) {
      sleep(sleep_ticks); // 偶尔阻塞
      printf("[%d] %s: woke i=%d\n", now_ticks(), tag, i);
    }
  }
  printf("[%d] %s: done\n", now_ticks(), tag);
}

int
main(int argc, char *argv[])
{
  printf("[MLQTEST] starting, ticks=%d\n", now_ticks());
  printf("[MLQTEST] sched_policy expects SCHED_MLQ\n");

  int pid1 = fork();
  if (pid1 == 0) {
    // 进程 A：CPU 密集 + 频繁 yield，预期逐步下调到中/低层
    // rounds 要适中，确保能观察到层级反馈
    cpu_task("CPU-A(high->low)", 200, 3);
    exit(0);
  }

  int pid2 = fork();
  if (pid2 == 0) {
    // 进程 B：I/O 型，频繁 sleep/wakeup，预期一直被提升到高层抢占
    io_task("IO-B(boosted)", 60, 10); // sleep 10 ticks
    exit(0);
  }

  int pid3 = fork();
  if (pid3 == 0) {
    // 进程 C：中等负载，偶尔阻塞、偶尔 yield
    medium_task("MID-C(mixed)", 120, 5, 7);
    exit(0);
  }

  // 父进程等待三个子进程结束
  int status;
  wait(&status);
  wait(&status);
  wait(&status);

  printf("[MLQTEST] all children done, ticks=%d\n", now_ticks());
  exit(0);
}
