#include "kernel/param.h"
#include "kernel/types.h" 
#include "kernel/stat.h"
#include "user/user.h"

// 简单的CPU工作负载
void cpu_work(int iterations) {
    for (int i = 0; i < iterations; i++) {
        // 空循环模拟CPU工作
    }
}

int main() {
    int pid1;
    
    printf("Starting priority scheduler test\n");
    
    // 测试1: 基本fork功能
    printf("Test 1: Basic fork test\n");
    pid1 = fork();
    if (pid1 == 0) {
        // 子进程1
        printf("Child process 1 running\n");
        cpu_work(100000);
        printf("Child process 1 finished\n");
        exit(0);
    }
    
    wait(0);
    printf("Test 1 passed: basic fork works\n\n");
    
    // 测试2: 优先级设置测试
    printf("Test 2: Priority setting test\n");
    
    pid1 = fork();
    if (pid1 == 0) {
        // 设置自己的优先级
        if (setpriority(getpid(), 10) == 0) {
            printf("Child: Successfully set my priority to 10\n");
        } else {
            printf("Child: Failed to set priority\n");
        }
        cpu_work(50000);
        exit(0);
    }
    
    // 父进程设置子进程的优先级
    if (setpriority(pid1, 5) == 0) {
        printf("Parent: Successfully set child priority to 5\n");
    } else {
        printf("Parent: Failed to set child priority\n");
    }
    
    wait(0);
    printf("Test 2 completed\n\n");
    
    // 测试3: 多个进程优先级测试
    printf("Test 3: Multiple processes with different priorities\n");
    
    int low_pid = fork();
    if (low_pid == 0) {
        setpriority(getpid(), 25);  // 低优先级
        printf("Low priority process started\n");
        for (int i = 0; i < 3; i++) {
            printf("Low priority working...\n");
            cpu_work(200000);
        }
        exit(0);
    }
    
    sleep(10);  // 给低优先级进程一些运行时间
    
    int high_pid = fork();
    if (high_pid == 0) {
        setpriority(getpid(), 5);   // 高优先级
        printf("High priority process started - should run faster!\n");
        for (int i = 0; i < 3; i++) {
            printf("High priority working...\n");
            cpu_work(200000);
        }
        exit(0);
    }
    
    // 设置子进程优先级
    setpriority(low_pid, 25);
    setpriority(high_pid, 5);
    
    // 等待子进程结束
    wait(0);
    wait(0);
    
    printf("All tests completed successfully!\n");
    exit(0);
}