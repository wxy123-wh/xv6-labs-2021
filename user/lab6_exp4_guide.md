# 实验四：虚拟存储器综合模拟（LRU 置换、缺页与磁盘 I/O 统计）

## 快速运行
- 编译：`cd /root/os-lab/xv6-labs-2021/user && gcc -std=c11 -Wall -Wextra -O2 lab6_exp4.c -o lab6_exp4`
- 运行：`./lab6_exp4`
- 默认：100 个虚拟页、10 个物理帧、页大小 4KB，访问 1000 次；比较“局部性 80/20”与“完全随机”两种模式的缺页与 I/O。

## 核心数据结构
- `page_table_entry_t`：`valid`、`frame_num`、`dirty`、`referenced`、`last_access`（LRU 时间戳）。
- `vm_manager_t`：页表数组、物理内存（仅占位）、`frame_to_page` 映射、`page_faults`、`disk_io_count`、`access_clock`（时间戳自增）。

## 主要流程
1. **初始化**（`vm_init`）  
   分配页表/物理内存/帧映射，全部置为无效，时间戳清零。
2. **地址转换**（`vm_translate`）  
   - 解析页号/偏移，越界即失败。  
   - 若页无效，调用 `handle_page_fault`。  
   - 标记引用，更新时间戳；写访问则置 `dirty=1`。  
   - 物理地址 = `frame_num * PAGE_SIZE + offset`。
3. **缺页处理 + LRU 置换**（`handle_page_fault`）  
   - 尝试找空闲帧；没有则用 `select_lru_victim` 选最久未用的帧。  
   - 淘汰脏页则计一次写回 I/O，失效其页表项。  
   - “装入”新页计一次读 I/O，更新页表/映射/时间戳。
4. **工作负载生成**（`run_workload`）  
   - 局部性模式：80% 访问集中在前 20% 页，20% 随机；随机模式：全随机。  
   - 30% 概率视为写访问，用于产生脏页与写回。

## 示例输出解读
- 显示帧数、页数、访问次数，随后是缺页次数/缺页率和磁盘 I/O（装载 + 写回）。  
- 在默认参数下，局部性模式缺页率明显低于完全随机，体现工作集局部性提升性能。

## 自己尝试
- 调整 `page_count`/`frame_count`/`access_times` 观察缺页率变化。  
- 修改局部性比例（如 90/10）或写访问比例以查看对脏页写回的影响。  
- 若想看更详细的替换过程，可在 `handle_page_fault` 中打印 victim 与新页。
