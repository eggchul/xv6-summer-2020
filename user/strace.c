#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(1,"invalid number of arguments\n");
        exit(0);
    }
    int id;
    id = fork();
    if (id == 0) {
        /* we are in the child */
        traceon();
        exec(argv[1], argv);
        exit(0);
    } else {
        /* we are in the parent */
        wait(0);
    }
    exit(0);
}
