#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    if(argc < 2){
        printf("Too few arguments. Usage: cstop <container name>\n");
        exit(0);
    }
    int rv = cstop(argv[1]);
    if(rv <0){
        printf("stop container %s failed\n", argv[1]);
    }
    unlink(argv[1]);
    exit(0);
}