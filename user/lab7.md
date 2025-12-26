# 实验七：单层目录简易文件系统实现（讲解版）

面向初学者的逐步说明，带完整代码与关键操作系统概念。配套代码在 `user/lab7_task1.c`，直接用 GCC 编译即可：

```bash
cd /root/os-lab/xv6-labs-2021/user
gcc -std=c11 -Wall -Wextra -pedantic lab7_task1.c -o lab7_demo
./lab7_demo   # 运行内置演示
```

## 0. 核心数据结构与常量
```c
#define DISK_SIZE 4096      // 模拟磁盘总容量（字节）
#define BLOCK_SIZE 64       // 每个磁盘块大小（字节）
#define BLOCK_NUM (DISK_SIZE / BLOCK_SIZE)  // 块数量
#define MAX_FILES 32
#define MAX_FILENAME_LEN 20

typedef struct {
    char name[MAX_FILENAME_LEN]; // 文件名
    int  size;                   // 文件大小（字节）
    int  start_block;            // 起始块号（连续分配）
    int  is_valid;               // 是否占用
} fcb_t; // 文件控制块 FCB：描述一个文件的元数据

typedef struct {
    fcb_t fcb_table[MAX_FILES];      // FCB 表
    unsigned char disk[DISK_SIZE];   // 模拟磁盘空间
    int bit_map[BLOCK_NUM / 32];     // 位示图：0 空闲，1 已占用
} simple_fs_t;
```

- **磁盘块 / BLOCK**：文件系统分配的基本单位。连续分配模型下，一个文件占用若干连续的块。
- **位示图 / bitmap**：用二进制位表示每个块是否已被占用。位操作能高效检查与标记块。
- **FCB**：文件控制块，保存文件名、大小、起始块等元数据，相当于极简版的 inode。

位操作辅助函数（读/写位示图）：
```c
static inline int block_used(const simple_fs_t *fs, int block_id) {
    int idx = block_id / 32;          // 第几个 int
    int offset = block_id % 32;       // int 内第几位
    return (fs->bit_map[idx] >> offset) & 1;
}

static inline void set_block(simple_fs_t *fs, int block_id, int used) {
    int idx = block_id / 32;
    int offset = block_id % 32;
    unsigned int mask = 1u << offset;
    if (used) fs->bit_map[idx] |= mask; else fs->bit_map[idx] &= ~mask;
}
```

## 1. 任务1：查找连续空闲块 `find_free_blocks`
思想：从块 0 开始，枚举所有可能的起始块，检查之后 `block_count` 个块是否都空闲（位示图为 0），找到就返回起始块号，否则返回 -1。

代码：
```c
int find_free_blocks(simple_fs_t *fs, int block_count) {
    if (fs == NULL || block_count <= 0 || block_count > BLOCK_NUM) return -1;

    for (int start = 0; start <= BLOCK_NUM - block_count; start++) {
        int free_run = 1;
        for (int i = 0; i < block_count; i++) {
            if (block_used(fs, start + i)) { // 位示图检查是否已占用
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
```

操作系统知识点：位示图（bitmap）用于记录磁盘块分配状态；连续分配策略要求找到一段连续的空闲块，提高顺序读写性能，但可能产生外部碎片。

## 2. 任务2：创建文件 `my_create`
步骤：
1. 检查参数、文件名重复、寻找空闲 FCB。
2. 计算需要的块数：`(size + BLOCK_SIZE - 1) / BLOCK_SIZE`（向上取整）。
3. 调用 `find_free_blocks` 分配连续块。
4. 在位示图中将这些块标记为占用。
5. 填写 FCB 元数据，标记为有效。

代码：
```c
int my_create(simple_fs_t *fs, const char *filename, int size) {
    if (fs == NULL || filename == NULL || size <= 0) { printf("错误：参数无效\n"); return -1; }

    // 文件名唯一性检查
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->fcb_table[i].is_valid && strcmp(fs->fcb_table[i].name, filename) == 0) {
            printf("错误：文件名 %s 已存在！\n", filename);
            return -1;
        }
    }

    // 找空闲 FCB
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->fcb_table[i].is_valid == 0) { fd = i; break; }
    }
    if (fd == -1) { printf("错误：文件表已满！\n"); return -1; }

    int block_count = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int start_block = find_free_blocks(fs, block_count);
    if (start_block == -1) { printf("错误：磁盘空间不足！\n"); return -1; }

    for (int i = 0; i < block_count; i++) set_block(fs, start_block + i, 1); // 标记位示图

    strncpy(fs->fcb_table[fd].name, filename, MAX_FILENAME_LEN - 1);
    fs->fcb_table[fd].name[MAX_FILENAME_LEN - 1] = '\0';
    fs->fcb_table[fd].size = size;
    fs->fcb_table[fd].start_block = start_block;
    fs->fcb_table[fd].is_valid = 1;

    printf("文件创建成功：%s (FD=%d, 大小=%d字节)\n", filename, fd, size);
    return fd;
}
```

知识点：FCB 相当于 inode 简化版；连续分配与位示图配合，类似 FAT 系统早期的分配方式。

## 3. 任务3：写文件 `my_write`
步骤：
1. 校验文件系统指针、描述符范围、FCB 有效性、数据指针。
2. 取文件大小与输入数据长度的最小值，避免越界。
3. 计算磁盘写入起始地址：`start_block * BLOCK_SIZE`。
4. 用 `memcpy` 把数据拷贝到模拟磁盘数组。

代码：
```c
int my_write(simple_fs_t *fs, int fd, const char *data) {
    if (fs == NULL) { printf("错误：文件系统指针为NULL\n"); return -1; }
    if (fd < 0 || fd >= MAX_FILES) { printf("错误：无效的文件描述符 %d（有效范围：0-%d）\n", fd, MAX_FILES - 1); return -1; }
    if (fs->fcb_table[fd].is_valid == 0) { printf("错误：文件描述符 %d 对应的文件不存在或已被删除\n", fd); return -1; }
    if (data == NULL) { printf("错误：写入数据指针为NULL\n"); return -1; }

    fcb_t *fcb = &fs->fcb_table[fd];
    int data_len = (int)strlen(data);
    if (data_len > fcb->size) {
        data_len = fcb->size;
        printf("警告：数据将被截断为 %d 字节（文件大小限制）\n", data_len);
    }

    int start_address = fcb->start_block * BLOCK_SIZE;
    if (start_address + data_len > DISK_SIZE) { printf("错误：写入数据超过磁盘大小\n"); return -1; }

    memcpy(fs->disk + start_address, data, data_len);
    printf("写入成功：向文件 %s 写入 %d 字节\n", fcb->name, data_len);
    return data_len;
}
```

知识点：用户视角“文件偏移”映射到“磁盘地址”=起始块 * 块大小；`memcpy` 模拟底层块写入；必须防止越界写导致“磁盘”覆盖。

## 4. 任务4：读文件 `my_read`
步骤：
1. 校验指针、描述符、FCB、缓冲区与大小。
2. 计算读取长度：`min(buffer_size-1, fcb->size)`，为字符串预留结尾 `'\0'`。
3. 计算起始地址并 `memcpy` 到用户缓冲区。
4. 在末尾补 `'\0'`，返回读取字节数。

代码：
```c
int my_read(simple_fs_t *fs, int fd, char *buffer, int buffer_size) {
    if (fs == NULL) { printf("错误：文件系统指针为NULL\n"); return -1; }
    if (fd < 0 || fd >= MAX_FILES) { printf("错误：无效的文件描述符 %d（有效范围：0-%d）\n", fd, MAX_FILES - 1); return -1; }
    if (fs->fcb_table[fd].is_valid == 0) { printf("错误：文件描述符 %d 对应的文件不存在或已被删除\n", fd); return -1; }
    if (buffer == NULL) { printf("错误：输出缓冲区指针为NULL\n"); return -1; }
    if (buffer_size <= 0) { printf("错误：缓冲区大小必须为正整数\n"); return -1; }

    fcb_t *fcb = &fs->fcb_table[fd];
    int read_size = (buffer_size - 1 < fcb->size) ? buffer_size - 1 : fcb->size;
    int start_address = fcb->start_block * BLOCK_SIZE;
    if (start_address + read_size > DISK_SIZE) read_size = DISK_SIZE - start_address;

    memcpy(buffer, fs->disk + start_address, read_size);
    buffer[read_size] = '\0';
    printf("读取成功：从文件 %s 读取 %d 字节\n", fcb->name, read_size);
    return read_size;
}
```

知识点：读写接口与 POSIX 中的 `read`/`write` 类似；需要防止缓冲区溢出并保证 C 字符串以 `\0` 结尾。

## 5. 内置演示（可选）
文件 `lab7_task1.c` 中的 `main`（以 `TASK1_DEMO` 宏控制）演示：
1. 人为占用部分块，调用 `find_free_blocks` 观察输出。
2. 创建一个 200 字节的文件（需要 4 个 64B 块）。
3. 向文件写入字符串，再读出并打印。

编译运行演示：
```bash
gcc -std=c11 -Wall -Wextra -pedantic -DTASK1_DEMO lab7_task1.c -o lab7_demo
./lab7_demo
```
示例输出：
```
找到空闲块：起始块=2, 块数=2
找到空闲块：起始块=7, 块数=2
找到空闲块：起始块=7, 块数=4
文件创建成功：hello.txt (FD=0, 大小=200字节)
写入成功：向文件 hello.txt 写入 17 字节
读取成功：从文件 hello.txt 读取 63 字节
read content: hello, xv6 fs lab
```

## 6. 自测建议
- 修改 `main` 或写独立测试，覆盖：
  - 分配失败（磁盘满/块数超限）
  - 写入截断与读取长度校验
  - 多文件创建占用不同块
- 关注位示图标记是否正确，避免“重复分配”。

至此，四个任务（空闲块查找、创建、写、读）均已实现并解释完毕，可作为简易文件系统的参考实现。***
