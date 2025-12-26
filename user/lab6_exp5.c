#include <stdio.h>
#include <stdlib.h>

// 全局已初始化数据段
int g_data_init = 42;
// 全局未初始化（BSS）
int g_data_bss;
// 只读数据（放在只读段）
const char *g_ro_str = "hello-rodata";

struct Point {
    int x;
    int y;
};

int add(int a, int b) {
    int result = a + b; // 栈上的局部变量
    return result;
}

void increment_point(struct Point *p) {
    if (!p) return;
    p->x += 1;
    p->y += 1;
}

// 用递归打印不同栈帧的地址，直观展示栈向下生长
void show_stack_level(int level, int max_level) {
    int local_var = level;
    printf("  stack level %d local_var address = %p\n", level, (void *)&local_var);
    if (level < max_level) {
        show_stack_level(level + 1, max_level);
    }
}

int main(void) {
    int stack_main = 123; // 栈变量
    static int static_var = 7; // 静态局部（属于数据/或BSS）

    printf("=== Code / Data / BSS / Heap / Stack address demo ===\n");
    printf("Function addresses (code/text):\n");
    printf("  main:              %p\n", (void *)&main);
    printf("  add:               %p\n", (void *)&add);
    printf("  increment_point:   %p\n", (void *)&increment_point);

    printf("\nGlobal / static variables:\n");
    printf("  g_data_init (data): %p (value=%d)\n", (void *)&g_data_init, g_data_init);
    printf("  g_data_bss  (bss):  %p (value=%d)\n", (void *)&g_data_bss, g_data_bss);
    printf("  g_ro_str   (rodata):%p -> \"%s\"\n", (void *)g_ro_str, g_ro_str);
    printf("  static_var (data/BSS): %p (value=%d)\n", (void *)&static_var, static_var);

    printf("\nStack variables:\n");
    printf("  stack_main (in main): %p (value=%d)\n", (void *)&stack_main, stack_main);
    show_stack_level(1, 3);

    printf("\nHeap allocation:\n");
    struct Point *heap_point = (struct Point *)malloc(sizeof(struct Point));
    if (!heap_point) {
        perror("malloc failed");
        return 1;
    }
    heap_point->x = 100;
    heap_point->y = 200;
    printf("  heap_point addr: %p (x=%d, y=%d)\n", (void *)heap_point, heap_point->x, heap_point->y);
    increment_point(heap_point);
    printf("  after increment: x=%d, y=%d\n", heap_point->x, heap_point->y);

    printf("\nCall add(3,5): result=%d (stack result inside add, not shown directly)\n", add(3, 5));

    free(heap_point);
    heap_point = NULL;
    return 0;
}
