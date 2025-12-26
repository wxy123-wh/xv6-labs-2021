# 实验5·任务二：生产者-消费者实现说明

面向小白的简明说明，告诉你新增了什么、代码怎么运作、如何自己跑一遍。

## 新增/修改的文件
- `lab5/producer_consumer.c`：实现有界缓冲区的生产者-消费者，使用任务一完成的信号量。
- `lab5/Makefile`：加入 `producer_consumer` 目标，方便单独编译运行。

> xv6 原始源码未改动，一切都放在 `lab5/` 下独立运行。

## 核心思路（PV 操作顺序）
- 三个信号量：
  - `empty` 初始值 = `BUFFER_SIZE`，表示可用空槽。
  - `full` 初始值 = 0，表示已有的商品数。
  - `mutex` 初始值 = 1，用来保护缓冲区（类似互斥锁）。
- 生产者 `produce()`：
  1) `P(empty)` 等空槽；2) `P(mutex)` 进入临界区；3) 放入商品、移动 `in` 指针、`count++`；4) `V(mutex)`；5) `V(full)` 告诉消费者有新商品。
- 消费者 `consume()`：
  1) `P(full)` 等商品；2) `P(mutex)` 进入临界区；3) 取出商品、移动 `out` 指针、`count--`；4) `V(mutex)`；5) `V(empty)` 归还空槽。
- 商品 ID 用 `item_id_lock` 保护，统计信息用 `stats.lock` 保护，避免数据竞争。

## 编译与运行
```bash
cd lab5
make            # 生成 test_semaphore 和 producer_consumer
./producer_consumer
```
如果在 PowerShell 下运行有问题，可用 WSL 方式：
```bash
wsl sh -c 'cd /root/os-lab/xv6-labs-2021/lab5 && ./producer_consumer'
```
运行时会看到生产/消费日志，最后输出统计信息（生产消费总数匹配即成功）。

## 你需要知道的最少知识
- `empty/full/mutex` 的初值分别对应“空位数 / 商品数 / 互斥锁”。
- 生产者必须先拿空位再拿互斥锁；消费者必须先拿商品再拿互斥锁，否则会死锁。
- 统计打印会告诉你每个生产者/消费者各处理了多少商品，最终应全部匹配 `NUM_PRODUCERS * ITEMS_PER_PRODUCER`。***
