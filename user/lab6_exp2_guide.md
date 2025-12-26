# 实验二：分页地址转换 + FIFO 置换（小白友好版）

## 快速运行
- 编译：`cd /root/os-lab/xv6-labs-2021/user && gcc -std=c11 -Wall -Wextra -O2 lab6_exp2.c -o lab6_exp2`
- 执行：`./lab6_exp2`
- 默认场景：页大小 4KB，物理帧 4 个；访问序列 `{1000, 5000, 9000, 13000, 17000, 21000, 1000}`。因为只有 4 帧，7 次访问都会缺页，能直观看到 FIFO 替换。

## 核心数据结构
- `pte_t`（页表项）：`frame_num`（物理帧号）、`valid`、`modified`（脏页演示）、`referenced`、`protection_bits`。
- `paging_system_t`：页表、模拟的物理内存、缺页计数、FIFO 队列（数组存页号）及前/后指针和当前大小 `queue_size`。

## 代码流程怎么运作
1. **初始化**（`paging_system_init`）
   - 页表全部置为无效、帧号 -1；物理内存清零；FIFO 队列填 -1，计数清零。
2. **查空闲帧**（`find_free_frame`）
   - 遍历帧号 0~3，若未出现在任何有效页表项的 `frame_num` 中则返回；否则 -1 表示需要置换。
3. **FIFO 队列**  
   - `enqueue_fifo`：循环队列，队列未满则尾插；满时覆盖最旧元素（相当于先弹再压）。  
   - `fifo_replace`：弹出队头页面，失效其页表项，返回被淘汰页面的帧号，并通过出参拿到被淘汰的页号，方便判定是否写回。
4. **缺页处理**（`handle_page_fault`）
   - 先试图找空闲帧；找不到就 FIFO 置换，若淘汰页有 `modified` 标记则打印“写回磁盘”提示。
   - “装入”新页：更新页表项为有效、记录帧号、模拟 referenced，简单把奇数页标为脏页以便演示写回提示。
   - 缺页计数 +1，并把新页号入队。
5. **地址转换**（`translate_address`）
   - 拆分逻辑地址：`page_num = addr / 4096`，`offset = addr % 4096`；越界直接报错。
   - 若页无效则触发缺页处理；之后计算物理地址 `frame_num * 4096 + offset` 并打印详情（含是否脏页）。

## 如何阅读输出
- `[PF] ...` 行：出现缺页，可能伴随“victim page X was modified -> write back to disk”提示。
- `VA ... -> page/off/PA`：每次访问的逻辑地址、页号、页内偏移、物理地址以及是否脏页。
- `FIFO queue (front -> rear)`：展示当前物理内存中页的进入顺序。
- 最后输出缺页次数与缺页率。

## 自己尝试
- 修改 `addrs` 数组可测试其它访问模式；调整 `FRAME_COUNT` 可观察缺页率变化。
- 如果想更真实地标记脏页，可在访问路径增加“写”操作时设置 `modified=true`。
