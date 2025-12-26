# 实验5·任务一：信号量实现说明

面向小白的简明说明，告诉你新增了什么、代码怎么运作、如何自己跑一遍。

## 本次新增/修改的文件
- `lab5/base_lock.h` / `lab5/base_lock.c`：对 `pthread_mutex_t` 的简单封装，提供 init/acquire/release/destroy 四个函数，给后面的信号量用来保护共享状态。
- `lab5/semaphore.h` / `lab5/semaphore.c`：完成计数信号量的定义和实现，含 `sem_init/sem_wait/sem_signal/sem_get_value/sem_print_status/sem_destroy`。
- `lab5/test_semaphore.c`：一个最小示例，把信号量当互斥锁，用 5 个线程抢占式累加计数器，验证不会出现数据竞争。
- `lab5/Makefile`：在 lab5 目录下单独编译上述代码的规则，不影响 xv6 原有工程。

> 说明：原始 xv6 源码未改动，只在新建的 `lab5/` 目录里补充任务一所需的文件。

## 信号量实现的核心逻辑
1. **初始化 (`sem_init`)**：设置初始 `value`，记录名字，初始化内部锁 `base_lock` 和条件变量 `pthread_cond_t`，统计字段清零。
2. **P 操作 (`sem_wait`)**：先加锁；如果 `value` 为 0，就把 `wait_count` +1、`total_wait_count` +1，然后 `pthread_cond_wait` 进入睡眠（自动释放锁，醒来后再拿锁）；直到有资源可用时才 `value--`，最后释放锁。
3. **V 操作 (`sem_signal`)**：加锁，将 `value` +1，`total_signal_count` +1；若有人在等（`wait_count>0`），就 `pthread_cond_signal` 唤醒一个等待线程，然后释放锁。
4. **调试辅助**：`sem_get_value` 读当前值（仅调试用），`sem_print_status` 打印当前值、等待队列数和统计次数，`sem_destroy` 清理锁和条件变量。
5. **为什么需要条件变量**：它对应 xv6 里的 sleep/wakeup，让等待的线程阻塞睡眠，不占 CPU；用 `while` 防止“虚假唤醒”。

## 如何编译和运行（只跑任务一的自测）
在仓库根目录执行：
```bash
cd lab5
make           # 生成 test_semaphore（Windows 下生成 .exe）
./test_semaphore
```
预期输出：能看到 5 个线程交替打印计数，最终 `Final counter: 50 (expected 50)`，并打印信号量状态（value 应为 1，wait_count 为 0）。

> 如果在 Windows PowerShell 直接运行挂住，可以在 WSL 里编译运行：  
> `wsl sh -c 'cd /root/os-lab/xv6-labs-2021/lab5 && gcc -Wall -Wextra -std=c11 -g -O0 base_lock.c semaphore.c test_semaphore.c -lpthread -o test_sem_linux && ./test_sem_linux'`

## 你需要知道的最少知识
- `value` 表示当前可用的“票”数；P 操作拿票（没票就睡），V 操作还票并唤醒一个等票的人。
- 所有对 `value` 和统计字段的读写都包在 `base_lock` 里，保证原子性。
- `wait_count/total_wait_count/total_signal_count` 只是用来调试/验证，不影响核心同步语义。

做到这里，任务一就完成了：信号量能正常阻塞/唤醒线程，示例程序验证了不会出现数据竞争。接下来做任务二、三时可以直接复用这套信号量。
