#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "procinfo.h"
#include "systime.h"
extern uint ticks;
extern struct spinlock tickslock;
extern struct proc proc[NPROC];
uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64
sys_mycall(void)
{
   uint xticks;
acquire(&tickslock);
xticks = ticks;
release(&tickslock);
return xticks;
}

uint64 sys_getprocinfo(void)
{
uint64 addr;
argaddr(0, &addr);
struct procinfo info;
struct proc *p = myproc();
info.pid = p->pid;
info.ppid = p->parent ? p->parent->pid : 0;
info.state = p->state;
info.sz = p->sz;
safestrcpy(info.name, p->name, sizeof(info.name));
if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
return -1;
return 0;
}

uint64
sys_getsystime(void)
{
    uint64 addr;  
    struct systime st;
    
    // 获取用户空间传递的参数（指向systime结构体的指针）
    if(argaddr(0, &addr) < 0)
        return -1;
    
    // 获取当前tick数（需要加锁保护）
    acquire(&tickslock);
    st.ticks = ticks;
    release(&tickslock);
    
    // 计算运行时间（秒），假设 100 ticks = 1秒
    st.uptime = st.ticks / 100;
    
    // 将数据复制回用户空间
    if(copyout(myproc()->pagetable, addr, (char *)&st, sizeof(st)) < 0)
        return -1;
    
    return 0;
}

uint64
sys_setpriority(void)
{
    int pid, priority;
    struct proc *p;
    
    // 获取参数
    if(argint(0, &pid) < 0 || argint(1, &priority) < 0) {
        return -1;
    }
    
    // 边界检查
    if(priority < 0 || priority > 31) {  // 添加上限检查
        return -1;
    }
    
    // 查找目标进程
    int found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->pid == pid) {
            p->priority = priority;
            found = 1;
            release(&p->lock);
            break;
        }
        release(&p->lock);
    }
    
    return found ? 0 : -1;
}