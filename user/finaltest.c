#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
struct procinfo {
    int pid;      // 进程ID
    int ppid;     // 父进程ID  
    int state;    // 进程状态
    uint64 sz;    // 进程内存大小
    char name[16]; // 进程名称
};
struct systime {
    uint ticks;
    uint uptime;
};
void test_getprocinfo() {
struct procinfo info;
printf("Testing getprocinfo()...\n");
if(getprocinfo(&info) == 0) {
printf("PID: %d, PPID: %d, State: %d\n",
info.pid, info.ppid, info.state);
printf("Size: %d, Name: %s\n", info.sz, info.name);
printf("getprocinfo() test PASSED\n");
} else {
printf("getprocinfo() test FAILED\n");
}
}
void test_getsystime() {
struct systime time;
printf("Testing getsystime()...\n");
if(getsystime(&time) == 0) {
printf("Ticks: %d, Uptime: %d seconds\n",
time.ticks, time.uptime);
printf("getsystime() test PASSED\n");
} else {
printf("getsystime() test FAILED\n");
}
}
void test_setpriority() {
printf("Testing setpriority()...\n");
// 测试正常情况
if(setpriority(getpid(), 5) == 0) {
printf("Set priority to 5: PASSED\n");
} else {
printf("Set priority to 5: FAILED\n");
}
// 测试边界条件
if(setpriority(getpid(), -1) == -1) {
printf("Invalid priority test: PASSED\n");
} else {
printf("Invalid priority test: FAILED\n");
}
// 测试无效 PID
if(setpriority(99999, 5) == -1) {
printf("Invalid PID test: PASSED\n");
} else {
printf("Invalid PID test: FAILED\n");
}
}
int main() {
printf("Starting system call tests...\n");
test_getprocinfo();
test_getsystime();
test_setpriority();
printf("All tests completed!\n");
exit(0);
}