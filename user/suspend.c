#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/resumable.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("invalid number of arguments\n");
        exit(0);
    }
    int pid = atoi(argv[1]);
    int fd = open(argv[2], O_CREATE | O_WRONLY);
    suspend(pid, fd);
    close(fd);
    kill(pid);
    exit(0);
}