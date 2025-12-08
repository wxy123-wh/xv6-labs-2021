#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    // 调用自定义系统调用 mycall()
    int result = mycall();
    
    // 输出结果
    printf("当前系统运行时间: %d ticks\n", result);
    printf("约合: %d 毫秒\n", result * 10);  // 假设10ms/tick
    
    exit(0);
}