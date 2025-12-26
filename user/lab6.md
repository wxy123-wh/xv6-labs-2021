内存管理实验 - 学生指导手册
一、实验目的
本实验通过五个渐进式实验，帮助学生深入理解操作系统内存管理的核心概念。学生将亲手实现从基础内存分
配到完整虚拟存储系统的全过程，掌握操作系统管理内存资源的原理及机制。
1. 理解操作系统内存管理的核心原理与实现机制，掌握动态分区分配、分页系统、页面置换及虚拟存储的
核心概念。
2. 掌握三种动态分区分配算法（FF、BF、WF）、三种页面置换算法（FIFO、LRU、OPTIMAL）的设计与编
码实现。
3. 学会构建简单虚拟存储系统，理解逻辑地址到物理地址的转换流程及缺页处理机制。
4. 掌握栈式内存分配原理，理解函数调用栈机制及栈溢出防护方法。
5. 设计科学的测试用例与性能评估方案，具备分析不同算法优缺点及适用场景的能力。
二、实验准备
2.1 必备知识储备
1. 内存管理核心概念：动态分区、分页、虚拟内存、栈与堆的区别。
2. 数据结构基础：链表、队列、栈的操作（遍历、插入、删除、排序）。
3. C语言编程能力：结构体定义、指针操作、函数封装、循环与条件判断。
4. 操作系统基础：进程地址空间布局、地址转换、缺页中断、页面置换原理。
2.2 需阅读的材料
1. 实验指导手册全文（重点关注数据结构定义与任务要求）。
2. 《Operating System Concepts》第8章（内存管理）、第9章（虚拟内存）。
3. 参考资料：Linux内存管理内核文档、分页系统地址转换原理手册。
2.3 实验环境与工具
1. 操作系统：Linux/Windows（WSL2）/macOS。
2. 开发工具：GCC 7.0+ 或 Clang 编译器、Make 工具、GDB 调试工具、Valgrind 内存检测工具。
3. 辅助工具：Git（版本控制）、Visio/DrawIO（绘制流程图）、Excel（性能数据统计）。
4. 编程基础：C语言开发环境（确保支持标准库函数）。
三、实验内容与分步实施
3.1 实验一：动态分区分配算法实现（难度：★★☆☆☆）
核心目标
动态分区分配是操作系统内存管理的基础技术。内存被划分为多个可变大小的分区，每个进程根据需求获得适
当大小的分区。该任务将实现首次适应（FF）、最佳适应（BF）、最坏适应（WF）三种动态分区分配算法及内
存释放功能，并对比算法性能。
3.1.1 数据结构回顾
// 空闲分区结构 - 学生需要完整理解每个字段含义
typedef struct free_area {
 int start_addr; // 起始地址
 int size; // 分区大小
 struct free_area *next; // 下一个分区指针
} free_area_t;
// 内存管理器
typedef struct {
 free_area_t *free_list; // 空闲分区链表
 int total_size; // 总内存大小
 int allocated_count; // 分配次数统计
 int released_count; // 释放次数统计
} memory_manager_t;
3.1.2 学生任务与实施步骤
任务1：初始化内存管理器
1. 实现memory_manager_init函数，初始化空闲分区链表（初始为一个完整空闲分区，起始地址0，大小
为total_size）。
2. 初始化分配/释放次数统计为0。
void init_memory_manager(memory_manager_t *mm, int total_size) {
 // TODO: 学生需要完成以下步骤：
 // 1. 设置总内存大小
 // 2. 创建初始空闲分区（覆盖整个内存空间）
 // 3. 初始化统计计数器
 // 4. 验证初始化正确性
}
任务2：实现最佳适应算法（BF）
int best_fit_allocate(memory_manager_t *mm, int size) {
 // TODO: 学生需要完成以下步骤：
 // 步骤1：遍历空闲链表，查找所有大小≥size的分区
 // 步骤2：筛选出其中最小的分区（最佳匹配）
 // 步骤3：若找到分区，判断大小是否等于size：
 // - 相等：直接从链表中移除该分区
 // - 大于：拆分分区，保留剩余空间为新空闲分区
 // 步骤4：更新分配次数统计，返回分配分区的起始地址
 // 步骤5：未找到合适分区，返回-1（分配失败）
 return -1;
}
任务3：实现最坏适应算法（WF）
int worst_fit_allocate(memory_manager_t *mm, int size) {
 // TODO: 学生需要完成以下步骤：
 // 步骤1：遍历空闲链表，查找所有大小≥size的分区
 // 步骤2：筛选出其中最大的分区（最坏匹配）
 // 步骤3：拆分分区（同BF算法步骤3）
 // 步骤4：更新分配次数统计，返回起始地址；失败返回-1
 return -1;
}
任务4：实现内存释放与分区合并
bool release_memory(memory_manager_t *mm, int addr, int size) {
 // TODO: 学生需要完成以下步骤：
 // 步骤1：创建新的空闲分区结构体，设置起始地址addr和大小size
 // 步骤2：将新分区插入空闲链表的合适位置（按地址有序插入，便于合并）
 // 步骤3：检查相邻分区（前一个和后一个），若存在空闲分区则合并：
 // - 与前一个分区相邻：合并为新分区，删除原两个分区
 // - 与后一个分区相邻：合并为新分区，删除原两个分区
 // - 与前后都相邻：合并为一个分区，删除原三个分区
 // 步骤4：更新释放次数统计，返回true；地址非法返回false
 return true;
}
3.1.3 测试要求
1. 测试序列：按顺序执行分配请求 {50, 30, 20, 40, 60, 10, 25} KB，记录每次分配结果（成功/失败，分配地
址）。
2. 统计指标：分配成功率（成功次数/总请求次数）、剩余空闲分区数量、平均碎片大小（总碎片大小/空
闲分区数量）。
3. 对比FF（自行补充实现）、BF、WF三种算法的测试结果。
// 测试用例设计
int test_sizes[] = {50, 30, 20, 40, 60, 10, 25}; // 单位：KB
int test_count = 7;
// 性能指标记录
typedef struct {
 int algorithm_type; // 算法类型
 int success_count; // 成功分配次数
 int total_fragments; // 碎片总数
 double fragmentation_rate; // 碎片率
} performance_metrics_t;
3.2 实验二：分页系统地址转换（难度：★★★☆☆）
核心目标
实现分页系统的地址转换、缺页处理及FIFO页面置换算法，理解分页管理的基本流程。
3.2.1 数据结构回顾
// 页表项结构
typedef struct page_table_entry {
 int frame_num; // 物理帧号
 bool valid; // 有效位（1：页面在物理内存，0：缺页）
 bool modified; // 修改位（1：页面被修改，需写回磁盘）
 bool referenced; // 访问位（1：页面近期被访问）
 int protection_bits; // 保护位（读写权限）
} pte_t;
// 分页系统管理器
typedef struct {
 pte_t page_table[256]; // 页表（支持256个页面，每页4KB）
 char physical_memory[4][4096]; // 物理内存（4个帧，每帧4KB）
 int page_fault_count; // 缺页次数统计
 // 新增：FIFO页面置换队列（记录页面进入物理内存的顺序）
 int fifo_queue[4];
 int queue_front, queue_rear;
} paging_system_t;
3.2.2 学生任务与实施步骤
任务1：初始化分页系统
1. 实现paging_system_init函数，初始化页表所有项的valid位为0，frame_num为-1。
2. 初始化缺页次数为0，FIFO队列为空。
void paging_system_init(paging_system_t *ps) {
 // TODO: 学生需要完成以下步骤：
 // TODO: 初始化页表
 // 遍历所有页表项，将 valid 位设为 0，frame_num 设为 -1，其他位也置为初始值
 // ...
 // TODO: 初始化缺页次数计数器为 0
 // ...
 // TODO: 初始化 FIFO 队列（例如，用 -1 填充队列，表示空）
 // ...
}
任务 2：实现 find_free_frame 函数
函数功能：在物理内存中查找一个空闲的帧，并返回其帧号。如果所有帧都被占用，则返回 -1。
/**
 * @brief 查找物理内存中的第一个空闲帧。
 * 
 * @param ps 指向分页系统管理器的指针。
 * @return int 找到的空闲帧号；如果没有空闲帧，返回 -1。
 */
int find_free_frame(paging_system_t *ps) {
 // TODO: 学生需要完成以下步骤：
 // 遍历所有可能的物理帧（0到3）
 // 提示：一个帧是否空闲，可以通过检查所有页表项来判断。
 // 如果一个帧号没有出现在任何一个 valid 位为 true 的页表项的 frame_num 字段中，那么
它就是空闲的。
 
 // 简化思路（假设页表项索引与帧号有对应关系）：
 // 遍历所有页表项，找到第一个 valid 位为 false 的项，其索引即可作为空闲帧号。
 // 注意：这种简化思路只在特定情况下成立，但其核心思想（查找未被占用的帧）是一致的。
 
 // 如果遍历完所有帧都没有找到空闲的，返回 -1
 return -1; 
}
任务3：实现FIFO页面置换算法
实现 fifo_replace 和 enqueue_fifo 函数。
void enqueue_fifo(paging_system_t *ps, int page_num) {
 // TODO: 学生需要完成以下步骤：
 // 将 page_num 加入到 fifo_queue 中
 // 提示：需要维护 queue_rear 指针，并处理循环队列的情况。
 
}
int fifo_replace(paging_system_t *ps) {
 // TODO: 学生需要完成以下步骤：
 // 步骤1：从FIFO队列头部取出最早进入的页面（队首元素）
 // 步骤2：记录该页面对应的帧号，作为被置换的帧号
 // 步骤3：将新页面加入队列尾部，维护队列指针
 // 步骤4：更新被置换页面的页表项（valid设为0）
 // 步骤5：返回被置换的帧号
 return 0;
}
任务4：完善缺页处理handle_page_fault 函数。
void handle_page_fault(paging_system_t *ps, int page_num) {
 // TODO: 学生需要完成以下步骤：
 // 步骤1：调用find_free_frame函数查找空闲物理帧
 
 // 步骤2：若无空闲帧，调用fifo_replace获取被置换的帧号
 if (frame_num == -1) {
 //...
 // 步骤3：若被置换页面被修改（modified=1），模拟写回磁盘（打印提示即可）
 }
 // 步骤4：模拟从磁盘加载页面到物理帧（无需实际I/O，打印提示）
 // 步骤5：更新页表项（valid=1，frame_num设为目标帧号，modified=0）
 // 步骤6：缺页次数+1
}
任务5：实现地址转换 translate_address 函数。
/**
 * @brief 将逻辑地址转换为物理地址。
 * 
 * @param ps 指向分页系统管理器的指针。
 * @param logical_addr 待转换的逻辑地址。
 * @return int 转换成功则返回物理地址；如果转换失败（如缺页处理失败），返回 -1。
 */
int translate_address(paging_system_t *ps, int logical_addr) {
 const int PAGE_SIZE = 4096; // 假设页面大小为 4KB
 // TODO: 学生需要完成以下步骤实现地址转换逻辑：
 // 步骤 1: 拆分逻辑地址
 // 提示:
 // - 页号 = 逻辑地址 / 页面大小 (使用整数除法)
 // - 页内偏移 = 逻辑地址 % 页面大小 (使用取余运算)
 // 请定义两个变量 page_num 和 offset 来存储结果。
 // 步骤 2: 检查页表项的有效性
 // 提示:
 // - 使用 page_num 作为索引，访问页表 ps->page_table。
 // - 检查该页表项的 'valid' 位。
 // - 如果 'valid' 位为 false，说明发生缺页中断。
 // 步骤 3: 处理缺页中断
 // 提示:
 // - 如果步骤 2 中发现缺页，调用 handle_page_fault(ps, page_num) 函数。
 // - 缺页处理完成后，页面应该已经被加载到物理内存中，页表项也已更新。
 // 步骤 4: 获取物理帧号
 // 提示:
 // - 再次访问页表项 (此时 'valid' 位应该为 true)。
 // - 获取该页表项中存储的物理帧号 'frame_num'。
 // 步骤 5: 计算物理地址
 // 提示:
 // - 物理地址 = 帧号 * 页面大小 + 页内偏移
 // - 请定义变量 physical_addr 来存储结果。
 // 步骤 6: 返回结果
 // - 返回计算出的物理地址。
 return -1; // 临时返回值，实现后请修改
}
3.2.3 测试要求
1. 测试序列：逻辑地址序列 {1000, 5000, 9000, 13000, 17000, 21000, 1000}（每页4KB，计算对应页号）。
2. 记录缺页次数，计算缺页率（缺页次数/总访问次数）。
3. 验证地址转换正确性：输出每次访问的逻辑地址、页号、偏移量、物理地址。
3.3 实验三：页面置换算法比较（难度：★★★★☆）
核心目标
实现LRU和OPTIMAL页面置换算法，对比三种算法在不同访问模式下的性能。
3.3.1 学生任务与实施步骤
任务1：实现LRU页面置换算法
typedef struct {
 int *frames; // 存储当前在内存中的页面号
 int frame_count; // 物理内存帧数
 int page_fault_count; // 缺页次数计数器
} replacement_manager_t;
int lru_algorithm(replacement_manager_t *rm, int *sequence, int length) {
 // 步骤1：维护页面访问时间戳数组，记录每个帧的最后访问时间
 // 时间戳可以用访问序列的索引（0, 1, 2, ..., length-1）来表示
 ...
 
 // 初始化帧和时间戳
 for (int i = 0; i < rm->frame_count; i++) {
 rm->frames[i] = -1; // -1 表示该帧为空
 timestamps[i] = -1; // -1 表示该帧从未被访问过
 }
 rm->page_fault_count = 0;
 // 步骤2：遍历访问序列，对每个页面：
 // - 若页面在物理内存：更新对应帧的时间戳为当前序号
 // - 若页面不在物理内存：缺页次数+1，查找时间戳最小的帧（最久未使用）进行置换
 // 步骤3：返回总缺页次数
 
 
}
任务2：实现OPTIMAL页面置换算法
int optimal_algorithm(replacement_manager_t *rm, int *sequence, int length) {
 // 步骤1：遍历访问序列，对每个缺页页面：
 // - 查看当前物理内存中所有页面，在后续访问序列中的出现位置
 // - 选择后续最晚出现（或永不出现）的页面进行置换
 // 步骤2：统计缺页次数，返回结果
 return 0;
}
3.3.2 测试要求
1. 测试序列：{1,2,3,4,1,2,5,1,2,3,4,5}，物理内存帧数分别为3、4、5。
2. 统计三种算法（FIFO、LRU、OPTIMAL）在不同帧数下的缺页次数和缺页率。
3. 分析Belady异常（FIFO算法在帧数增加时缺页率反而上升的现象）。
3.4 实验四：虚拟存储器综合模拟（难度：★★★★★）
核心目标
1. 综合运用前三个实验的技术，构建完整虚拟存储系统，实现地址转换、缺页处理、页面置换全流程。
2. 引入更复杂的页面置换算法（如LRU）。
3. 分析程序访问局部性原理对虚拟存储系统性能的影响。
请先阅读以下关键数据结构定义，理解每个字段在后续流程中的作用：
// 关键数据结构定义
typedef struct {
 int valid; // 有效位：1-在内存，0-不在内存
 int frame_num; // 物理帧号
 int dirty; // 脏位：1-被修改过，0-未修改
 int referenced; // 访问位：用于LRU等算法
 // 可自行设计更精确的LRU实现
} page_table_entry_t;
// 虚拟存储管理器
typedef struct {
 page_table_entry_t *page_table; // 页表
 int *physical_memory; // 物理内存帧
 int page_count; // 虚拟页面总数
 int frame_count; // 物理帧总数
 int page_faults; // 缺页次数统计
 int disk_io_count; // 磁盘I/O次数统计
 // 可自行设计更精确的LRU实现
} vm_manager_t;
3.4.1 学生任务与实施步骤
任务1 初始化虚拟存储管理器
实现初始化虚拟存储管理器（vm_init函数）：
void vm_init(vm_manager_t *vm, int page_count, int frame_count) {
 // TODO: 学生需要完成以下步骤：
 // 核心操作：
 //1. 为页表（page_table）动态分配内存（数量 = 虚拟页面总数），每个页表项初始化：
valid=0（默认不在内存）、frame_num=-1、dirty=0、referenced=0。
 //2. 为物理内存（physical_memory）动态分配内存（大小 = 物理帧总数 × 页面大小）。
 //3. 初始化统计变量：page_faults=0（缺页次数）、disk_io_count=0（磁盘 I/O 次数）。
 //4. 初始化 LRU 算法所需辅助结构（如访问时间戳数组等，用于跟踪页面访问顺序）。
}
任务2 核心任务
1. 实现vm_translate函数，完成逻辑地址到物理地址的完整转换（包含缺页处理）。
int vm_translate(vm_manager_t *vm, int logical_addr) {
 // TODO: 学生需要完成以下步骤：
 // 1. 从逻辑地址提取页号。
 // 2. 合法性检查：页号需在 [0, 虚拟页面总数 - 1] 范围内，否则返回 - 1（地址非法）。
 // 3. 检查页表有效性：访问vm->page_table[page_num]，若valid=1，执行步骤 5；若0，
执行步骤 4。
 // 4. 处理缺页：调用handle_page_fault(vm, page_num)，缺页处理完成后，重新确认页表
项valid=1。
 // 5. 更新访问标记：调用lru_update_access(vm, page_num)，维护 LRU 算法状态。
 // 6. 计算物理地址：物理地址 = 帧号 × 页面大小 + 页内偏移，返回该地址。
}
2. 集成LRU页面置换算法到缺页处理流程中。
3. 统计缺页次数和磁盘I/O次数（每次页面写回或加载记为一次I/O）。
3.4.2 测试要求
1. 生成1000次内存访问序列，包含两种模式：
局部性访问：80%的访问集中在20%的地址空间。
随机访问：20%的访问随机分布在整个地址空间。
2. 对比不同访问模式下的缺页率和磁盘I/O次数，分析局部性原理对虚拟存储性能的影响。
3.5 实验五：程序内存地址分配分析（难度：★★★☆☆）
3.5.1 实验原理
程序被调度到内存执行后，会分配具体内存空间，包含代码区，数据区和堆栈区。函数的执行过程中对临时变
量，参数和返回值的处理不同，同时函数调用也会发生堆栈区的变化。请自行回顾以下概念：
代码区 (Code Segment / Text Segment)
数据区 (Data Segment)
BSS区 (Block Started by Symbol)
堆 (Heap)
栈 (Stack)
3.5.2 示例程序
以下是一个 C 语言示例程序，我们将以此为基础，分析其在内存中的分配情况。为了观察内存地址，可在
main 函数中添加 printf 语句，打印出关键变量和函数的内存地址。
// func_demo.c
// 函数1：简单参数，简单返回值
int add(int a, int b)
{
 int result;
 result = a + b;
 return result;
}
// 结构体定义
struct Point {
 int x;
 int y;
};
// 函数2：结构体指针参数（指向堆内存），无返回值
// 功能：将一个点的 x 和 y 坐标都增加 1
void increment_point(struct Point *p)
{
 if (p == NULL) {
 printf("Error: Null pointer received!\n");
 return;
 }
 p->x += 1;
 p->y += 1;
}
int main()
{
 int sum = add(3, 5);
 printf("Sum: %d\n", sum);
 struct Point *heap_point = (struct Point *)malloc(sizeof(struct Point));
 if (heap_point == NULL) {
 perror("malloc failed"); // perror 会打印系统错误信息
 return 1; // 返回非0值表示程序异常退出
 }
 heap_point->x = 100;
 heap_point->y = 200;
 printf("Before increment (Heap): x = %d, y = %d\n", heap_point->x, heap_point-
>y);
 increment_point(heap_point);
 printf("After increment (Heap): x = %d, y = %d\n", heap_point->x, heap_point-
>y);
 free(heap_point);
 heap_point = NULL;
 return 0;
}
3.5.3 学生任务
观察内存分配:上述函数在Linux系统中执行后，请分析其在内存中代码区、数据区、堆栈区和堆的使用情况；反
汇编后观察函数是如何处理局部变量、参数、返回值的。 整理观察结果：绘制表格，列出你观察到的各个内存
区域的地址范围和存放内容。
四、性能评估与分析
4.1 评估指标定义
1. 分配成功率 = 成功分配次数 / 总分配请求次数 × 100%。
2. 缺页率 = 缺页次数 / 总内存访问次数 × 100%。
3. 平均等待时间 = 所有进程等待时间总和 / 进程数。
4. 内存利用率 = 已分配内存大小 / 总内存大小 × 100%。
5. 分配/释放耗时 = 单次分配/释放操作的平均时间（单位：微秒）。
4.2 分析要求
1. 对实验的测试结果制作对比表格（如不同算法的指标对比）或绘制性能图表（如缺页率随帧数变化曲
线、不同算法分配成功率对比柱状图等）。
2. 分析各算法的优缺点及适用场景（如BF算法适用于小内存请求较多的场景）。
五、实验报告要求
5.1 报告内容结构
1. 实验目的：简要复述实验核心目标。
2. 实验环境：详细记录操作系统、编译器、工具版本及开发环境配置。
3. 实验内容与实现：
分实验模块阐述，每个模块包含数据结构设计、核心函数实现（附代码片段）、关键流程图（如
地址转换流程、内存释放合并流程）。
说明遇到的问题及解决方案（如链表操作bug、置换算法逻辑错误的调试过程）。
4. 测试结果与分析：
按实验模块呈现测试用例、测试数据（表格形式）、性能图表。
对比分析不同算法的性能差异，结合原理说明原因。
5. 实验总结：
总结实验收获与体会（如对内存管理机制的理解、编程能力的提升）。
可提出算法改进建议（如如何优化BF算法的查找效率、栈保护机制的增强方案等）。
5.2 提交要求
1. 实验报告：PDF格式，命名规范“学号_姓名_内存管理实验报告.pdf”。
2. 源代码：压缩包格式，包含所有.c和.h文件，命名规范“学号_姓名_内存管理实验源码.zip”。
3. 测试材料：测试结果截图、性能分析表格（Excel格式），与报告一同提交。
六、参考资料
1. 《Operating System Concepts》（第10版）第8-9章。
2. xv6操作系统源码（kernel/memlayout.h、kernel/vm.c）。
3. Linux内核内存管理文档：https://www.kernel.org/doc/html/latest/admin-guide/mm/index.html。
4. 栈溢出防护技术白皮书：金丝雀值、ASLR机制原理介绍。