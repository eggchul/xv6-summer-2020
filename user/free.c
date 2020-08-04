#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{

    if(cfree() <0){
        printf("execute cfree failed\n");
    }
    exit(0);
}