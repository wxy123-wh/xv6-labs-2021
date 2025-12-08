#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

int sched_policy = DEFAULT_SCHED_POLICY;

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
struct multilevel_queue mlq;
// 初始化 MLQ
static void mlq_init(void){
  for(int i=0;i<MLQ_LEVELS;i++){
    mlq.head[i] = 0;
    mlq.tail[i] = 0;
  }
}

// 入队到指定层级（尾插）
static void mlq_enqueue_level(struct proc *p, int lvl){
  struct proc **head = mlq_head_ref(lvl);
  struct proc **tail = mlq_tail_ref(lvl);

  p->rq_next = 0;
  p->rq_prev = *tail;

  if(*tail) (*tail)->rq_next = p;
  else      *head = p;

  *tail = p;
  p->qlevel = lvl;
}

static void mlq_remove_level(struct proc *p){
  int lvl = p->qlevel;
  if(lvl < MLQ_HIGH || lvl > MLQ_LOW) return;

  struct proc **head = mlq_head_ref(lvl);
  struct proc **tail = mlq_tail_ref(lvl);

  if(p->rq_prev) p->rq_prev->rq_next = p->rq_next;
  else           *head = p->rq_next;

  if(p->rq_next) p->rq_next->rq_prev = p->rq_prev;
  else           *tail = p->rq_prev;

  p->rq_next = 0;
  p->rq_prev = 0;
  p->qlevel  = -1;
}

static struct proc* mlq_pick_head(void){
  for (int lvl = MLQ_HIGH; lvl <= MLQ_LOW; lvl++){
    struct proc **head = mlq_head_ref(lvl);
    if (*head) return *head;   // 返回该层的队头节点（不修改队列）
  }
  return 0; // 所有层都为空
}

static struct proc* priority_pick_best(void){
  struct proc *best = 0;
  int bestp = 0x7fffffff;
  for(struct proc *p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state == RUNNABLE && p->priority < bestp){
      bestp = p->priority;
      best = p;
    }
    release(&p->lock);
  }
  return best;
}


// 根据当前策略入队
void rq_enqueue(struct proc *p){
  // 调用方已持 p->lock，且设置 p->state = RUNNABLE
  if(sched_policy == SCHED_MLQ){
    // 仅 MLFQ：决定进入哪一层
    int lvl = (p->qlevel >= 0 && p->qlevel < MLQ_LEVELS)
              ? p->qlevel
              : prio_to_mlq_level(p->priority); // 可用静态 prio 作为“初始层”
    mlq_enqueue_level(p, lvl);
  }else{ // SCHED_PRIORITY
    // 完全不入 MLQ，保持 RUNNABLE 即可
    if(p->qlevel >= 0) mlq_remove_level(p); // 防御式清理
    p->qlevel = -1;
    p->rq_next = p->rq_prev = 0;
    // 如需时间片：统一固定
    p->cur_qslice_left = FIXED_QSLICE_PRIORITY; // 例如 10
  }
}

void rq_remove(struct proc *p){
  if(sched_policy == SCHED_MLQ){
    if(p->qlevel >= 0) mlq_remove_level(p);
  }else{
    // PRIORITY 模式：不在 MLQ，无需移除
    p->qlevel = -1;
    p->rq_next = p->rq_prev = 0;
  }
}

struct proc* rq_pick_next(void){
  if(sched_policy == SCHED_MLQ){
    // 高→中→低取队头（队内顺序）
    return mlq_pick_head();
  }else{
    // 优先级调度：扫描所有 RUNNABLE，选最高优先级
    return priority_pick_best();
  }
}

// 时钟 tick 中的运行进程片扣减

// 返回 1 表示需要在调用点 yield；返回 0 表示不需要
int on_tick_running(struct proc *p){
  if (!p) return 0;
  if (p->state != RUNNING) return 0;

  // 递减时间片
  if (p->cur_qslice_left > 0) {
    p->cur_qslice_left--;
  }

  // 未耗尽则不抢占
  if (p->cur_qslice_left > 0) {
    return 0;
  }

  // 时间片耗尽：按策略处理，并请求让出
  if (sched_policy == SCHED_MLQ){
    // 计算当前层级（若未知，按最低层兜底）
    int lvl = (p->qlevel >= 0) ? p->qlevel : MLQ_LOW;

    // 降级目标
    int newlvl = (lvl < MLQ_LOW) ? (lvl + 1) : MLQ_LOW;

    // 修改状态与队列（注意锁）
    acquire(&p->lock);
    // 将运行态进程标记回 RUNNABLE，并放入新层队列
    p->state = RUNNABLE;

    // 从旧层移除（若层级有效）
    if (p->qlevel >= 0) {
      mlq_remove_level(p);
    }
    // 入队到新层
    mlq_enqueue_level(p, newlvl);

    // 更新层级与新层时间片
    p->qlevel = newlvl;
    p->cur_qslice_left = qslice_for_level(newlvl);
    release(&p->lock);

    // 时间片耗尽，应让出
    return 1;
  } else {
    // PRIORITY 策略：不涉及 MLFQ 层级
    acquire(&p->lock);
    p->state = RUNNABLE;
    // 保持 qlevel = -1（或不使用）
    p->cur_qslice_left = FIXED_QSLICE_PRIORITY; // 重置固定时间片
    release(&p->lock);

    return 1; // 让出以便调度下一个
  }
}


// 唤醒时（比如 sleep->wakeup）
// 返回 1 表示建议抢占当前进程（由调用点决定是否 yield）；返回 0 表示不必
int on_wakeup(struct proc *p){
  if (!p) return 0;

  // 将 SLEEPING → RUNNABLE，并按策略入队
  acquire(&p->lock);
  p->state = RUNNABLE;

  if (sched_policy == SCHED_MLQ){
    // 先从原层移除（若有）
    if (p->qlevel >= 0) {
      mlq_remove_level(p);
    }
    // 提升到高层
    int newlvl = MLQ_HIGH;
    mlq_enqueue_level(p, newlvl);
    p->qlevel = newlvl;
    p->cur_qslice_left = qslice_for_level(newlvl);
    release(&p->lock);

    // MLFQ 中，I/O 唤醒通常奖励高层，建议抢占
    return 1;
  } else {
    // PRIORITY 策略：不使用 MLFQ 层级（保持 qlevel = -1）
    p->qlevel = -1;
    p->rq_next = p->rq_prev = 0;
    // 放入通用就绪队列
    rq_enqueue(p);
    // 重置固定时间片可选（取决于你的实现）
    p->cur_qslice_left = FIXED_QSLICE_PRIORITY;
    // 根据优先级判断是否建议抢占由调用点完成，这里保守返回 0
    release(&p->lock);
    return 0;
  }
}



void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->priority = 15;
  p->wait_time = 0;
  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
// proc.c - 修改您提供的fork函数
// proc.c - 完整的 fork 函数实现
// proc.c - 完整的 fork 函数实现
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // 分配进程控制块
  if((np = allocproc()) ==0){
    return -1;
  }

  // 复制用户内存从父进程到子进程
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // 复制保存的用户寄存器
  *(np->trapframe) = *(p->trapframe);

  // 使fork在子进程中返回0
  np->trapframe->a0 = 0;

  // 增加打开文件描述符的引用计数
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  // 安全复制进程名
  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  // 设置父进程关系
  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  // 设置子进程状态为可运行
  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  // 优先级调度：如果子进程优先级更高，触发抢占
  if(np->priority < p->priority) {
    yield();  // 立即让出CPU
  }

  return pid;
}
// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.

void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  // 可选：初始化 MLQ
  if (sched_policy == SCHED_MLQ) {
    mlq_init();
    // 初始把现有 RUNNABLE 进程按 priority 映射到队列
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        p->qlevel = prio_to_mlq_level(p->priority);
        rq_enqueue(p);
      }
      release(&p->lock);
    }
  }

  for(;;){
    intr_on();

    struct proc *best_proc = 0;

    if (sched_policy == SCHED_MLQ) {
      // 从队列高到低拿第一个
      best_proc = rq_pick_next();
      if (best_proc) {
        // 出队
        acquire(&best_proc->lock);
        if (best_proc->state == RUNNABLE) {
          rq_remove(best_proc);
          best_proc->state = RUNNING;
          c->proc = best_proc;
          swtch(&c->context, &best_proc->context);
          c->proc = 0;
          // 运行返回后，保持 wait_time 管理
          best_proc->wait_time = 0;
        }
        release(&best_proc->lock);
      }
    } else {
      // 你的原始优先级调度扫描 + 老化
      int best_priority = 32;
      for(p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->state == RUNNABLE) {
          p->wait_time++;
          if(p->wait_time > 100 && p->priority > 0) {
            p->priority--;
            p->wait_time = 0;
          }
          if(!best_proc || p->priority < best_priority) {
            best_proc = p;
            best_priority = p->priority;
          }
        }
        release(&p->lock);
      }
      if(best_proc) {
        acquire(&best_proc->lock);
        if(best_proc->state == RUNNABLE) {
          best_proc->state = RUNNING;
          c->proc = best_proc;
          swtch(&c->context, &best_proc->context);
          c->proc = 0;
          best_proc->wait_time = 0;
        }
        release(&best_proc->lock);
      }
    }
  }
}



// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);

  // 调整 MLQ 层级（主动让出时，下调一层，避免长占用）
  if (sched_policy == SCHED_MLQ) {
    int old = p->qlevel >= 0 ? p->qlevel : prio_to_mlq_level(p->priority);
    int newlvl = old < MLQ_LOW ? (old + 1) : MLQ_LOW;
    p->qlevel = newlvl;
  }

  p->state = RUNNABLE;

  // 入队（根据策略映射/层级）
  rq_enqueue(p);

  sched();
  release(&p->lock);
}


// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  acquire(&p->lock);
  release(lk);

  // 若在队列里，移除之（避免悬挂）
  if (sched_policy == SCHED_MLQ) {
    // 仅当当前认为在队列中时移除；简单做法尝试按当前层移除
    if (p->state == RUNNABLE) {
      rq_remove(p);
    }
  }

  p->chan = chan;
  p->state = SLEEPING;

  sched();

  p->chan = 0;

  release(&p->lock);
  acquire(lk);
}


// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
// proc.c - 完整的 wakeup 函数实现
// proc.c - 完整的 wakeup 函数实现
// 需有声明：extern int on_wakeup(struct proc *p);

void
wakeup(void *chan)
{
  struct proc *p;
  struct proc *current = myproc();

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != current){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        // on_wakeup 内部会自行加锁处理，所以这里先释放锁
        release(&p->lock);

        // 执行唤醒逻辑（置 RUNNABLE、入队、层级设置/时间片重置等）
        int suggest_preempt = on_wakeup(p);

        // 根据策略与返回值决定是否抢占当前 RUNNING 进程
        if (suggest_preempt && current && current->state == RUNNING) {
          if (sched_policy == SCHED_MLQ) {
            // MLFQ：唤醒通常提升到高层，建议抢占
            yield();
            return; // 抢占后即可结束本次 wakeup
          } else if (sched_policy == SCHED_PRIORITY) {
            // 如果你希望在优先级策略下也根据优先级进行抢占，可在此加判断：
            // if (p->priority < current->priority) {
            //   yield();
            //   return;
            // }
            // 若不做优先级比较，保守地不抢占：
            // （保留 suggest_preempt==0 的默认行为）
          }
        }

        // 继续遍历其他进程，无需重新获取 p->lock（本分支已交由 on_wakeup 完成处理）
      } else {
        // 非目标或未睡眠：正常释放锁
        release(&p->lock);
      }
    }
  }
}


// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}
