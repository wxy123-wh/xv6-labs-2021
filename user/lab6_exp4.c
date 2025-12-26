#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PAGE_SIZE 4096

typedef struct {
    int valid;
    int frame_num;
    int dirty;
    int referenced;
    int last_access; // 用于 LRU，访问时间戳
} page_table_entry_t;

typedef struct {
    page_table_entry_t *page_table; // 长度 = page_count
    int *physical_memory;           // 模拟物理内存：frame_count * PAGE_SIZE
    int *frame_to_page;             // 记录每个帧当前放的是哪个页（-1 表示空）
    int page_count;
    int frame_count;
    int page_faults;
    int disk_io_count;              // 读写磁盘次数（装载/写回）
    int access_clock;               // 全局时间戳自增，用于 LRU
} vm_manager_t;

static void vm_init(vm_manager_t *vm, int page_count, int frame_count) {
    vm->page_count = page_count;
    vm->frame_count = frame_count;
    vm->page_faults = 0;
    vm->disk_io_count = 0;
    vm->access_clock = 0;

    vm->page_table = (page_table_entry_t *)malloc(sizeof(page_table_entry_t) * page_count);
    vm->physical_memory = (int *)malloc(sizeof(int) * frame_count * PAGE_SIZE);
    vm->frame_to_page = (int *)malloc(sizeof(int) * frame_count);
    if (!vm->page_table || !vm->physical_memory || !vm->frame_to_page) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < page_count; i++) {
        vm->page_table[i].valid = 0;
        vm->page_table[i].frame_num = -1;
        vm->page_table[i].dirty = 0;
        vm->page_table[i].referenced = 0;
        vm->page_table[i].last_access = -1;
    }
    for (int f = 0; f < frame_count; f++) {
        vm->frame_to_page[f] = -1;
    }
}

static int find_free_frame(vm_manager_t *vm) {
    for (int f = 0; f < vm->frame_count; f++) {
        if (vm->frame_to_page[f] == -1) {
            return f;
        }
    }
    return -1;
}

static int select_lru_victim(vm_manager_t *vm) {
    int victim_frame = 0;
    int oldest_time = vm->access_clock + 1;
    for (int f = 0; f < vm->frame_count; f++) {
        int page = vm->frame_to_page[f];
        if (page == -1) {
            return f; // 空闲帧直接用
        }
        int last = vm->page_table[page].last_access;
        if (last < oldest_time) {
            oldest_time = last;
            victim_frame = f;
        }
    }
    return victim_frame;
}

static void handle_page_fault(vm_manager_t *vm, int page_num) {
    vm->page_faults++;
    int frame = find_free_frame(vm);
    int victim_page = -1;
    if (frame == -1) {
        frame = select_lru_victim(vm);
        victim_page = vm->frame_to_page[frame];
        if (victim_page >= 0 && vm->page_table[victim_page].dirty) {
            vm->disk_io_count++; // 写回
        }
        if (victim_page >= 0) {
            vm->page_table[victim_page].valid = 0;
        }
    }
    // 装入新页：模拟一次磁盘读
    vm->disk_io_count++;
    vm->frame_to_page[frame] = page_num;
    page_table_entry_t *pte = &vm->page_table[page_num];
    pte->valid = 1;
    pte->frame_num = frame;
    pte->dirty = 0;
    pte->referenced = 1;
    pte->last_access = ++vm->access_clock;
}

static int vm_translate(vm_manager_t *vm, int logical_addr, int is_write) {
    if (logical_addr < 0) {
        return -1;
    }
    int page_num = logical_addr / PAGE_SIZE;
    int offset = logical_addr % PAGE_SIZE;
    if (page_num < 0 || page_num >= vm->page_count) {
        return -1; // 非法地址
    }
    if (!vm->page_table[page_num].valid) {
        handle_page_fault(vm, page_num);
    }
    page_table_entry_t *pte = &vm->page_table[page_num];
    if (!pte->valid) {
        return -1;
    }
    pte->referenced = 1;
    pte->last_access = ++vm->access_clock;
    if (is_write) {
        pte->dirty = 1;
    }
    int physical_addr = pte->frame_num * PAGE_SIZE + offset;
    return physical_addr;
}

// 生成一次访问：locality=true 时 80% 访问在前 20% 的页内，20% 随机全空间
static void run_workload(const char *name, int page_count, int frame_count, int access_times, int locality) {
    vm_manager_t vm;
    vm_init(&vm, page_count, frame_count);
    int hot_range = page_count / 5; // 20%
    if (hot_range < 1) {
        hot_range = 1;
    }

    for (int i = 0; i < access_times; i++) {
        int page;
        if (locality && (rand() % 100) < 80) {
            page = rand() % hot_range;
        } else {
            page = rand() % page_count;
        }
        int offset = rand() % PAGE_SIZE;
        int is_write = (rand() % 100) < 30; // 30% 写访问
        int logical_addr = page * PAGE_SIZE + offset;
        vm_translate(&vm, logical_addr, is_write);
    }

    double fault_rate = (double)vm.page_faults / access_times;
    printf("%s: frames=%d, pages=%d, accesses=%d\n", name, frame_count, page_count, access_times);
    printf("  page faults = %d (rate %.2f)\n", vm.page_faults, fault_rate);
    printf("  disk I/O    = %d (load+writeback)\n\n", vm.disk_io_count);

    free(vm.page_table);
    free(vm.physical_memory);
    free(vm.frame_to_page);
}

int main(void) {
    srand(1); // 固定种子便于复现
    const int page_count = 100;
    const int frame_count = 10;
    const int access_times = 1000;

    printf("=== Virtual Memory simulator with LRU replacement ===\n");
    run_workload("Locality-heavy (80/20)", page_count, frame_count, access_times, 1);
    run_workload("Uniform random", page_count, frame_count, access_times, 0);
    return 0;
}
