#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    if(argc < 2){
        printf("Too few arguments. Usage: resume <container name>\n");
        exit(0);
    }
    int rv = cresume(argv[1]);
    if(rv <0){
        printf("resume container %s failed\n", argv[1]);
    }
    exit(0);
}