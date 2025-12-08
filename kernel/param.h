#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name

//调度模式定义
#define SCHED_PRIORITY    0  // 优先级调度模式
#define SCHED_MLQ         1  // 多级队列调度模式
// 当前调度策略（你已有 extern），默认选择其中之一
#ifndef DEFAULT_SCHED_POLICY
#define DEFAULT_SCHED_POLICY SCHED_PRIORITY
#endif
extern int sched_policy;     // 当前调度策略
// 多级队列参数（3 层：高/中/低）
#define MLQ_LEVELS        3
#define MLQ_HIGH          0
#define MLQ_MEDIUM        1
#define MLQ_LOW           2
// 抢占策略开关：唤醒高层队列是否尝试抢占当前运行进程
#define MLQ_PREEMPT_ON_WAKEUP 1

// 每个队列的时间片长度
#define HIGH_QUEUE_QUANTUM    5
#define MEDIUM_QUEUE_QUANTUM  10
#define LOW_QUEUE_QUANTUM     20
#define FIXED_QSLICE_PRIORITY 10  // 优先级调度的统一时间片长度

#define QSLICE_HIGH   5
#define QSLICE_MEDIUM 10
#define QSLICE_LOW    20