#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    if(df() <0){
        printf("df failed\n");
    }
    exit(0);
}