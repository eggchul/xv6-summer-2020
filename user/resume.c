#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{   
    if (argc != 2) {
        printf("invalid number of arguments\n");
        exit(0);
    }
    int pid = fork();
    if(pid == 0) {
        resume(argv[1]);
    }
    if(pid < 0) {
        printf("fork failed for resume\n");
    }
    exit(0);
}