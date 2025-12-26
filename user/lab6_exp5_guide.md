# 实验五：程序内存分配观察（代码/数据/BSS/堆/栈）

## 快速运行
- 编译：`cd /root/os-lab/xv6-labs-2021/user && gcc -std=c11 -Wall -Wextra -O2 lab6_exp5.c -o lab6_exp5`
- 运行：`./lab6_exp5`
- 输出会打印函数地址、全局/静态变量地址、栈变量地址、堆分配地址，并用递归展示栈帧地址变化。

## 代码里各区域对应什么
- 代码段（text）：函数指针 `&main` / `&add` / `&increment_point`。
- 数据段：已初始化的全局/静态变量 `g_data_init`、`static_var`。
- BSS 段：未初始化全局 `g_data_bss`。
- 只读数据：字符串常量 `g_ro_str` 指向的文字区域。
- 栈：`stack_main` 以及 `show_stack_level` 中的递归局部变量，地址随深度递减，展示栈向下生长。
- 堆：`malloc` 得到的 `heap_point`，调用 `increment_point` 修改其内容。

## 输出怎么读
- “Function addresses” 对应代码段；“Global / static variables” 列出数据/BSS/rodata 地址；“Stack variables” 显示不同栈帧的局部变量地址；“Heap allocation” 给出堆块地址和修改前后数值。
- 地址数值因运行环境不同会变化，但相对分布（代码低、数据/rodata 紧邻、堆较低且向上增长、栈较高且向下增长）是可观察的。

## 可自行尝试
- 增加更多全局或静态变量，观察其地址是否连续；添加 `static` 未初始化变量以对比数据/BSS。
- 调整递归深度或调用链，观察栈地址变化；在 `malloc` 多次分配，观察堆地址增长方向。 
