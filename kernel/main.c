#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "ansi.h"
#include "spinlock.h"
volatile static int started = 0;
extern char end[];
volatile int booted_cpus = 0;
extern uint ticks;
extern uint64 sys_mycall(void);

void print_colored_welcome(void) {
    if(cpuid() != 0) return;
    
    printf("\n");
    printf_color(ANSI_BOLD ANSI_CYAN, 
        "=========================================\n");
    printf_color(ANSI_BOLD ANSI_CYAN, 
        "         " ANSI_BOLD ANSI_YELLOW "欢迎使用 xv6 操作系统" ANSI_BOLD ANSI_CYAN "         \n");
    printf_color(ANSI_BOLD ANSI_CYAN, 
        "=========================================\n");
    
    printf_green("系统状态: 正在启动...\n");
    printf_blue("学生ID: 202332111340\n");
    printf("\n");
}

void print_colored_system_info(void) {
    if(cpuid() != 0) return;
    
    uint64 total_memory = (uint64)PHYSTOP - (uint64)KERNBASE;
    uint64 kernel_size = (uint64)end - (uint64)KERNBASE;
    uint64 available_memory = total_memory - kernel_size;
    
    printf_color(ANSI_BOLD ANSI_MAGENTA, "内存信息:\n");
    printf("  物理内存总量: %d MB\n", total_memory / 1024 / 1024);
    printf("  内核占用空间: %d KB\n", kernel_size / 1024);
    printf("  可用内存: %d MB\n", available_memory / 1024 / 1024);
    
    printf_color(ANSI_BOLD ANSI_MAGENTA, "\nCPU信息:\n");
    printf("  系统支持最大CPU数: %d\n", NCPU);
    printf("  当前运行CPU核心: %d\n", cpuid());
    printf("  实际启动CPU核心: %d\n", booted_cpus);
}

void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    print_colored_welcome();
    kinit();
    kvminit();
    kvminithart();
    procinit();
    #if CURRENT_SCHEDULER == SCHED_MLQ
        mlq_init();      
// 初始化多级队列
#endif
    trapinit();      // 初始化陷阱处理
    trapinithart();  // 启用中断处理
    plicinit();      // 初始化中断控制器
    plicinithart(); 
    uint64 start_time = sys_mycall();
    printf("启动开始时间: %d ticks\n", start_time);  // 启用中断控制器
    binit();
    iinit();
    fileinit();
    virtio_disk_init();
    userinit();
    // 在初始化后添加
    printf("检查时钟中断...\n");
    uint64 initial_ticks = sys_mycall();
    uint64 current_ticks = initial_ticks;
    const int max_spin = 20000000; // 提前等待一段时间，确保看到至少一个时钟中断
    for (int i = 0; i < max_spin && current_ticks == initial_ticks; i++) {
        current_ticks = sys_mycall();
    }
    if (current_ticks == initial_ticks) {
        printf("警告: 时钟中断未在预期时间内到达，ticks 仍为 %d\n", (uint)initial_ticks);
    } else {
        printf("时钟中断正常，ticks 已更新: %d -> %d\n", (uint)initial_ticks, (uint)current_ticks);
    }

    uint64 end_time = sys_mycall();
    uint64 duration = end_time - start_time;
    printf("启动结束时间: %d ticks\n", end_time);
    printf("系统启动耗时: %d ticks\n", duration);
    print_colored_system_info();
    
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    __sync_fetch_and_add(&booted_cpus, 1);
    printf("hart %d starting\n", cpuid());
    kvminithart();
    trapinithart();
    plicinithart();
  }

  scheduler();        
}
