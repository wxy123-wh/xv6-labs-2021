#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_TABLE_SIZE 256
#define FRAME_COUNT 4
#define PAGE_SIZE 4096

typedef struct page_table_entry {
    int frame_num;       // 物理帧号
    bool valid;          // 是否在物理内存
    bool modified;       // 是否被修改（用于写回演示）
    bool referenced;     // 最近是否被访问
    int protection_bits; // 读写权限（本示例未用）
} pte_t;

typedef struct {
    pte_t page_table[PAGE_TABLE_SIZE];
    char physical_memory[FRAME_COUNT][PAGE_SIZE];
    int page_fault_count;
    int fifo_queue[FRAME_COUNT];
    int queue_front; // 指向队头
    int queue_rear;  // 指向队尾
    int queue_size;  // 当前队列大小
} paging_system_t;

static void paging_system_init(paging_system_t *ps) {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        ps->page_table[i].frame_num = -1;
        ps->page_table[i].valid = false;
        ps->page_table[i].modified = false;
        ps->page_table[i].referenced = false;
        ps->page_table[i].protection_bits = 0;
    }
    for (int f = 0; f < FRAME_COUNT; f++) {
        for (int b = 0; b < PAGE_SIZE; b++) {
            ps->physical_memory[f][b] = 0;
        }
        ps->fifo_queue[f] = -1;
    }
    ps->page_fault_count = 0;
    ps->queue_front = 0;
    ps->queue_rear = 0;
    ps->queue_size = 0;
}

// 返回首个空闲帧号，若无则返回 -1
static int find_free_frame(paging_system_t *ps) {
    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        bool used = false;
        for (int p = 0; p < PAGE_TABLE_SIZE; p++) {
            if (ps->page_table[p].valid && ps->page_table[p].frame_num == frame) {
                used = true;
                break;
            }
        }
        if (!used) {
            return frame;
        }
    }
    return -1;
}

// 将页面加入 FIFO 队列（满时覆盖最旧元素）
static void enqueue_fifo(paging_system_t *ps, int page_num) {
    if (ps->queue_size == 0) {
        ps->fifo_queue[0] = page_num;
        ps->queue_front = 0;
        ps->queue_rear = 0;
        ps->queue_size = 1;
        return;
    }
    if (ps->queue_size < FRAME_COUNT) {
        ps->queue_rear = (ps->queue_rear + 1) % FRAME_COUNT;
        ps->fifo_queue[ps->queue_rear] = page_num;
        ps->queue_size++;
        return;
    }
    // 队列已满，覆盖队头（相当于先弹出再压入）
    ps->queue_front = (ps->queue_front + 1) % FRAME_COUNT;
    ps->queue_rear = (ps->queue_rear + 1) % FRAME_COUNT;
    ps->fifo_queue[ps->queue_rear] = page_num;
}

// 弹出队头页面，返回其帧号；若队列为空返回 -1
static int fifo_replace(paging_system_t *ps, int *victim_page_out) {
    if (ps->queue_size == 0) {
        return -1;
    }
    int victim_page = ps->fifo_queue[ps->queue_front];
    int victim_frame = -1;
    if (victim_page >= 0 && ps->page_table[victim_page].valid) {
        victim_frame = ps->page_table[victim_page].frame_num;
    }
    // 弹出队头
    ps->queue_front = (ps->queue_front + 1) % FRAME_COUNT;
    ps->queue_size--;

    // 失效页表项
    if (victim_page >= 0) {
        ps->page_table[victim_page].valid = false;
    }

    if (victim_page_out) {
        *victim_page_out = victim_page;
    }
    return victim_frame;
}

// FIFO 缺页处理：找到空闲帧或置换最旧页面
static void handle_page_fault(paging_system_t *ps, int page_num) {
    printf("[PF] page %d fault\n", page_num);
    int victim_page = -1;
    int frame_num = find_free_frame(ps);
    if (frame_num == -1) {
        frame_num = fifo_replace(ps, &victim_page);
        if (victim_page >= 0 && ps->page_table[victim_page].modified) {
            printf("     victim page %d was modified -> write back to disk\n", victim_page);
        }
    }
    // 模拟装入新页面
    printf("     load page %d into frame %d\n", page_num, frame_num);
    pte_t *pte = &ps->page_table[page_num];
    pte->valid = true;
    pte->frame_num = frame_num;
    pte->modified = (page_num % 2 == 1); // 简单模拟：奇数页标记为已修改
    pte->referenced = true;
    pte->protection_bits = 0;
    ps->page_fault_count++;
    enqueue_fifo(ps, page_num);
}

// 逻辑地址 -> 物理地址，必要时触发缺页处理
static int translate_address(paging_system_t *ps, int logical_addr) {
    if (logical_addr < 0) {
        return -1;
    }
    int page_num = logical_addr / PAGE_SIZE;
    int offset = logical_addr % PAGE_SIZE;
    if (page_num >= PAGE_TABLE_SIZE) {
        printf("Logical address %d is out of range\n", logical_addr);
        return -1;
    }
    if (!ps->page_table[page_num].valid) {
        handle_page_fault(ps, page_num);
    }
    pte_t *pte = &ps->page_table[page_num];
    if (!pte->valid) {
        return -1;
    }
    pte->referenced = true;
    int physical_addr = pte->frame_num * PAGE_SIZE + offset;
    printf("VA %5d -> page %2d, offset %4d, PA %5d (frame %d)%s\n",
           logical_addr,
           page_num,
           offset,
           physical_addr,
           pte->frame_num,
           pte->modified ? " [dirty]" : "");
    return physical_addr;
}

static void print_fifo_queue(paging_system_t *ps) {
    printf("FIFO queue (front -> rear): ");
    for (int i = 0; i < ps->queue_size; i++) {
        int idx = (ps->queue_front + i) % FRAME_COUNT;
        printf("%d ", ps->fifo_queue[idx]);
    }
    printf("\n");
}

int main(void) {
    paging_system_t ps;
    paging_system_init(&ps);

    const int addrs[] = {1000, 5000, 9000, 13000, 17000, 21000, 1000};
    const int count = sizeof(addrs) / sizeof(addrs[0]);

    printf("=== Paging test (FIFO replacement, 4 frames, page size 4KB) ===\n");
    for (int i = 0; i < count; i++) {
        translate_address(&ps, addrs[i]);
        print_fifo_queue(&ps);
    }

    double fault_rate = (double)ps.page_fault_count / count;
    printf("\nPage faults: %d / %d (rate %.2f)\n", ps.page_fault_count, count, fault_rate);
    return 0;
}
