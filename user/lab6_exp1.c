#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct free_area {
    int start_addr;           // 起始地址（单位：KB，便于测试）
    int size;                 // 分区大小
    struct free_area *next;   // 下一空闲分区
} free_area_t;

typedef struct {
    free_area_t *free_list;   // 按起始地址有序的空闲链表
    int total_size;           // 总内存大小
    int allocated_count;      // 分配次数
    int released_count;       // 释放次数
} memory_manager_t;

typedef int (*alloc_fn)(memory_manager_t *mm, int size);

typedef struct {
    const char *name;
    int success_count;
    int request_count;
    int free_block_count;
    double avg_fragment;
} metrics_t;

static free_area_t *new_node(int start, int size) {
    free_area_t *n = (free_area_t *)malloc(sizeof(free_area_t));
    if (!n) {
        perror("malloc");
        exit(1);
    }
    n->start_addr = start;
    n->size = size;
    n->next = NULL;
    return n;
}

void init_memory_manager(memory_manager_t *mm, int total_size) {
    mm->total_size = total_size;
    mm->allocated_count = 0;
    mm->released_count = 0;
    // 初始只有一个完整的空闲分区
    mm->free_list = new_node(0, total_size);
}

// 计算当前碎片信息
static void collect_fragments(memory_manager_t *mm, int *count, int *total_size) {
    *count = 0;
    *total_size = 0;
    for (free_area_t *p = mm->free_list; p; p = p->next) {
        (*count)++;
        (*total_size) += p->size;
    }
}

// 打印空闲链表，方便观察效果
static void print_free_list(memory_manager_t *mm) {
    printf("Free list: ");
    for (free_area_t *p = mm->free_list; p; p = p->next) {
        printf("[addr=%d, size=%d] ", p->start_addr, p->size);
    }
    printf("\n");
}

static int first_fit_allocate(memory_manager_t *mm, int size) {
    free_area_t *prev = NULL;
    for (free_area_t *p = mm->free_list; p; prev = p, p = p->next) {
        if (p->size < size) {
            continue;
        }
        int addr = p->start_addr;
        if (p->size == size) {
            // 完全匹配，移出链表
            if (prev) {
                prev->next = p->next;
            } else {
                mm->free_list = p->next;
            }
            free(p);
        } else {
            // 拆分分区
            p->start_addr += size;
            p->size -= size;
        }
        mm->allocated_count++;
        return addr;
    }
    return -1;
}

int best_fit_allocate(memory_manager_t *mm, int size) {
    free_area_t *best = NULL;
    free_area_t *best_prev = NULL;
    free_area_t *prev = NULL;
    for (free_area_t *p = mm->free_list; p; prev = p, p = p->next) {
        if (p->size >= size) {
            if (!best || p->size < best->size) {
                best = p;
                best_prev = prev;
            }
        }
    }
    if (!best) {
        return -1;
    }
    int addr = best->start_addr;
    if (best->size == size) {
        if (best_prev) {
            best_prev->next = best->next;
        } else {
            mm->free_list = best->next;
        }
        free(best);
    } else {
        best->start_addr += size;
        best->size -= size;
    }
    mm->allocated_count++;
    return addr;
}

int worst_fit_allocate(memory_manager_t *mm, int size) {
    free_area_t *worst = NULL;
    free_area_t *worst_prev = NULL;
    free_area_t *prev = NULL;
    for (free_area_t *p = mm->free_list; p; prev = p, p = p->next) {
        if (p->size >= size) {
            if (!worst || p->size > worst->size) {
                worst = p;
                worst_prev = prev;
            }
        }
    }
    if (!worst) {
        return -1;
    }
    int addr = worst->start_addr;
    if (worst->size == size) {
        if (worst_prev) {
            worst_prev->next = worst->next;
        } else {
            mm->free_list = worst->next;
        }
        free(worst);
    } else {
        worst->start_addr += size;
        worst->size -= size;
    }
    mm->allocated_count++;
    return addr;
}

bool release_memory(memory_manager_t *mm, int addr, int size) {
    if (size <= 0 || addr < 0 || addr + size > mm->total_size) {
        return false;
    }
    free_area_t *prev = NULL;
    free_area_t *p = mm->free_list;
    // 找到插入位置（保持按地址有序）
    while (p && p->start_addr < addr) {
        prev = p;
        p = p->next;
    }
    // 与前后分区检查是否重叠
    if (prev && addr < prev->start_addr + prev->size) {
        return false;
    }
    if (p && addr + size > p->start_addr) {
        return false;
    }

    free_area_t *node = new_node(addr, size);
    if (prev) {
        prev->next = node;
    } else {
        mm->free_list = node;
    }
    node->next = p;

    // 尝试与前后相邻分区合并
    if (prev && prev->start_addr + prev->size == node->start_addr) {
        prev->size += node->size;
        prev->next = node->next;
        free(node);
        node = prev;
    }
    if (node->next && node->start_addr + node->size == node->next->start_addr) {
        free_area_t *next = node->next;
        node->size += next->size;
        node->next = next->next;
        free(next);
    }

    mm->released_count++;
    return true;
}

static metrics_t run_sequence(const char *name, alloc_fn fn, const int *reqs, int count, int total_size) {
    memory_manager_t mm;
    init_memory_manager(&mm, total_size);

    int success = 0;
    printf("=== %s ===\n", name);
    for (int i = 0; i < count; i++) {
        int addr = fn(&mm, reqs[i]);
        if (addr >= 0) {
            success++;
            printf("Request %d KB -> allocated at %d\n", reqs[i], addr);
        } else {
            printf("Request %d KB -> failed (no space)\n", reqs[i]);
        }
    }
    print_free_list(&mm);

    int frag_count = 0;
    int frag_total = 0;
    collect_fragments(&mm, &frag_count, &frag_total);

    metrics_t m = {
        .name = name,
        .success_count = success,
        .request_count = count,
        .free_block_count = frag_count,
        .avg_fragment = frag_count ? (double)frag_total / frag_count : 0.0};
    return m;
}

static void show_release_demo(void) {
    printf("\n=== Release & merge demo (best-fit) ===\n");
    memory_manager_t mm;
    init_memory_manager(&mm, 200);

    int a1 = best_fit_allocate(&mm, 50);
    int a2 = best_fit_allocate(&mm, 30);
    int a3 = best_fit_allocate(&mm, 20);
    print_free_list(&mm);

    release_memory(&mm, a2, 30);
    printf("After releasing block at %d (30 KB):\n", a2);
    print_free_list(&mm);

    release_memory(&mm, a1, 50);
    printf("After releasing block at %d (50 KB) -> should merge with previous:\n", a1);
    print_free_list(&mm);

    release_memory(&mm, a3, 20);
    printf("After releasing block at %d (20 KB) -> full merge expected:\n", a3);
    print_free_list(&mm);
}

int main(void) {
    const int total_size = 200; // 200 KB 总空间
    const int test_sizes[] = {50, 30, 20, 40, 60, 10, 25};
    const int test_count = sizeof(test_sizes) / sizeof(test_sizes[0]);

    metrics_t m1 = run_sequence("First Fit", first_fit_allocate, test_sizes, test_count, total_size);
    metrics_t m2 = run_sequence("Best Fit", best_fit_allocate, test_sizes, test_count, total_size);
    metrics_t m3 = run_sequence("Worst Fit", worst_fit_allocate, test_sizes, test_count, total_size);

    printf("\n=== Summary ===\n");
    metrics_t all[] = {m1, m2, m3};
    for (int i = 0; i < 3; i++) {
        printf("%s: success %d/%d, free blocks %d, avg fragment size %.2f KB\n",
               all[i].name,
               all[i].success_count,
               all[i].request_count,
               all[i].free_block_count,
               all[i].avg_fragment);
    }

    show_release_demo();
    return 0;
}
