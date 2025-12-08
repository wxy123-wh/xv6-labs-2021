struct procinfo {
int pid; // 进程 ID
int ppid; // 父进程 ID
int state; // 进程状态
uint sz; // 内存大小
char name[16]; // 进程名称
};
