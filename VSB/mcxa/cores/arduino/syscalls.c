#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// This file's purpose is to disable the annoying warnings while compiling about missing implementations of certain syscalls. You can ignore this.

int _read(int file, char *ptr, int len) {
    return 0;
}

int _write(int file, char *ptr, int len) {
    return len;
}

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    while (1);
}