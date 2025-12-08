#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void cpu_job(const char* tag, int spins, int chunk, int say){
  volatile int x=0;
  for(int i=0;i<spins;i++){
    x += i ^ (x<<1);
    if (say && (i % chunk)==0)
      printf("%s: i=%d\n", tag, i);
  }
  printf("%s: done x=%d\n", tag, x);
}

void io_job(const char* tag, int rounds, int work, int slp){
  volatile int x=0;
  for(int r=0;r<rounds;r++){
    for(int i=0;i<work;i++) x+=i^(x<<1);
    printf("%s: r=%d sleep=%d\n", tag, r, slp);
    sleep(slp);
  }
  printf("%s: done x=%d\n", tag, x);
}

// 1) 验证唤醒提升与抢占
void test_mlfq_preemption(){
  int pid = fork();
  if(pid==0){
    cpu_job("cpu-A", 2000000, 200000, 1);
    exit(0);
  }
  int pid2 = fork();
  if(pid2==0){
    io_job("io-B", 30, 10000, 1);
    exit(0);
  }
  int st;
  wait(&st); wait(&st);
}

// 2) 验证时间片耗尽降级（后启动者插队）
void test_mlfq_demotion(){
  int pidA = fork();
  if(pidA==0){
    cpu_job("cpu-early", 3000000, 300000, 1);
    exit(0);
  }
  // 给 A 一点时间先跑，便于降级
  sleep(5);

  int pidB = fork();
  if(pidB==0){
    cpu_job("cpu-late",  1000000, 100000, 1);
    exit(0);
  }
  int st;
  wait(&st); wait(&st);
}

int main(){
  printf("test_sched: start\n");
  test_mlfq_preemption();
  test_mlfq_demotion();
  printf("test_sched: done\n");
  exit(0);
}
