#include <stdio.h>
#include <string.h>

#define DISK_SIZE 4096
#define BLOCK_SIZE 64
#define BLOCK_NUM (DISK_SIZE / BLOCK_SIZE)
#define MAX_FILES 32
#define MAX_FILENAME_LEN 20

// File control block.
typedef struct {
    char name[MAX_FILENAME_LEN];
    int size;
    int start_block;
    int is_valid;
} fcb_t;

// Simple file-system container.
typedef struct {
    fcb_t fcb_table[MAX_FILES];
    unsigned char disk[DISK_SIZE];
    int bit_map[BLOCK_NUM / 32];
} simple_fs_t;

static inline int block_used(const simple_fs_t *fs, int block_id) {
    int idx = block_id / 32;
    int offset = block_id % 32;
    return (fs->bit_map[idx] >> offset) & 1;
}

static inline void set_block(simple_fs_t *fs, int block_id, int used) {
    int idx = block_id / 32;
    int offset = block_id % 32;
    unsigned int mask = 1u << offset;
    if (used) {
        fs->bit_map[idx] |= mask;
    } else {
        fs->bit_map[idx] &= ~mask;
    }
}

void init_simple_fs(simple_fs_t *fs) {
    memset(fs, 0, sizeof(*fs));
}

// Task 1: find a continuous run of free blocks.
int find_free_blocks(simple_fs_t *fs, int block_count) {
    if (fs == NULL || block_count <= 0 || block_count > BLOCK_NUM) {
        return -1;
    }

    for (int start = 0; start <= BLOCK_NUM - block_count; start++) {
        int free_run = 1;
        for (int i = 0; i < block_count; i++) {
            if (block_used(fs, start + i)) {
                free_run = 0;
                break;
            }
        }
        if (free_run) {
            printf("找到空闲块：起始块=%d, 块数=%d\n", start, block_count);
            return start;
        }
    }

    printf("错误：找不到连续的%d个空闲块\n", block_count);
    return -1;
}

// Task 2: create a file and allocate contiguous blocks.
int my_create(simple_fs_t *fs, const char *filename, int size) {
    // 参数校验
    if (fs == NULL || filename == NULL || size <= 0) {
        printf("错误：参数无效\n");
        return -1;
    }

    // 检查文件名是否重复
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->fcb_table[i].is_valid && strcmp(fs->fcb_table[i].name, filename) == 0) {
            printf("错误：文件名 %s 已存在！\n", filename);
            return -1;
        }
    }

    // 查找空闲 FCB
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->fcb_table[i].is_valid == 0) {
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        printf("错误：文件表已满！\n");
        return -1;
    }

    // 1) 计算需要的块数（向上取整）
    int block_count = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 2) 查找连续空闲块
    int start_block = find_free_blocks(fs, block_count);
    if (start_block == -1) {
        printf("错误：磁盘空间不足！\n");
        return -1;
    }

    // 3) 在位示图中标记为已使用
    for (int i = 0; i < block_count; i++) {
        set_block(fs, start_block + i, 1);
    }

    // 4) 初始化 FCB
    strncpy(fs->fcb_table[fd].name, filename, MAX_FILENAME_LEN - 1);
    fs->fcb_table[fd].name[MAX_FILENAME_LEN - 1] = '\0';
    fs->fcb_table[fd].size = size;
    fs->fcb_table[fd].start_block = start_block;
    fs->fcb_table[fd].is_valid = 1;

    printf("文件创建成功：%s (FD=%d, 大小=%d字节)\n", filename, fd, size);
    return fd;
}

// Task 3: write data into a file.
int my_write(simple_fs_t *fs, int fd, const char *data) {
    if (fs == NULL) {
        printf("错误：文件系统指针为NULL\n");
        return -1;
    }
    if (fd < 0 || fd >= MAX_FILES) {
        printf("错误：无效的文件描述符 %d（有效范围：0-%d）\n", fd, MAX_FILES - 1);
        return -1;
    }
    if (fs->fcb_table[fd].is_valid == 0) {
        printf("错误：文件描述符 %d 对应的文件不存在或已被删除\n", fd);
        return -1;
    }
    if (data == NULL) {
        printf("错误：写入数据指针为NULL\n");
        return -1;
    }

    fcb_t *fcb = &fs->fcb_table[fd];
    int data_len = (int)strlen(data);
    if (data_len > fcb->size) {
        data_len = fcb->size;
        printf("警告：数据将被截断为 %d 字节（文件大小限制）\n", data_len);
    }

    int start_address = fcb->start_block * BLOCK_SIZE;
    if (start_address + data_len > DISK_SIZE) {
        printf("错误：写入数据超过磁盘大小\n");
        return -1;
    }

    memcpy(fs->disk + start_address, data, data_len);
    printf("写入成功：向文件 %s 写入 %d 字节\n", fcb->name, data_len);
    return data_len;
}

// Task 4: read data from a file.
int my_read(simple_fs_t *fs, int fd, char *buffer, int buffer_size) {
    if (fs == NULL) {
        printf("错误：文件系统指针为NULL\n");
        return -1;
    }
    if (fd < 0 || fd >= MAX_FILES) {
        printf("错误：无效的文件描述符 %d（有效范围：0-%d）\n", fd, MAX_FILES - 1);
        return -1;
    }
    if (fs->fcb_table[fd].is_valid == 0) {
        printf("错误：文件描述符 %d 对应的文件不存在或已被删除\n", fd);
        return -1;
    }
    if (buffer == NULL) {
        printf("错误：输出缓冲区指针为NULL\n");
        return -1;
    }
    if (buffer_size <= 0) {
        printf("错误：缓冲区大小必须为正整数\n");
        return -1;
    }

    fcb_t *fcb = &fs->fcb_table[fd];
    int read_size = (buffer_size - 1 < fcb->size) ? buffer_size - 1 : fcb->size;
    int start_address = fcb->start_block * BLOCK_SIZE;
    if (start_address + read_size > DISK_SIZE) {
        read_size = DISK_SIZE - start_address;
    }

    memcpy(buffer, fs->disk + start_address, read_size);
    buffer[read_size] = '\0';
    printf("读取成功：从文件 %s 读取 %d 字节\n", fcb->name, read_size);
    return read_size;
}

#ifdef TASK1_DEMO
// Minimal demonstration for task 1.
int main(void) {
    simple_fs_t fs;
    init_simple_fs(&fs);

    // Mark a few blocks as used to form gaps.
    set_block(&fs, 0, 1);
    set_block(&fs, 1, 1);
    set_block(&fs, 5, 1);
    set_block(&fs, 6, 1);

    find_free_blocks(&fs, 2);  // should pick start=2
    set_block(&fs, 2, 1);
    set_block(&fs, 3, 1);
    find_free_blocks(&fs, 2);  // should pick start=4
    find_free_blocks(&fs, 4);  // should fall back to a later slot

    // Demo Task 2: create a file of 200 bytes (needs 4 blocks with size 64).
    my_create(&fs, "hello.txt", 200);

    // Demo Task 3 & 4: write and read.
    my_write(&fs, 0, "hello, xv6 fs lab");
    char buf[64];
    my_read(&fs, 0, buf, sizeof(buf));
    printf("read content: %s\n", buf);

    return 0;
}
#endif
