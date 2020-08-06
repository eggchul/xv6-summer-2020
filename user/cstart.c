#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

int
main(int argc, char *argv[]){
    if(argc < 4){
        printf("too few arguments\n");
        printf("usage: cstart <virtual console> <container name> <init program> [max mem] [max disk] [max proc]\n");
    }

    int vcfd = open(argv[1], O_RDWR);
    if(vcfd < 0){
        printf("cstart: Could not find vc\n");
        exit(0);
    }

    // int newmem = atoi(argv[4]);
    // if(newmem != 0){
    //     setmaxcmem(argv[2], newmem);
    // }
    // int newdisk = atoi(argv[5]);
    // if(newdisk != 0){
    //     setmaxcdsk(argv[2], newdisk);
    // }
    // int numproc = atoi(argv[6]);
    // if(numproc != 0){
    //     setmaxcproc(argv[2], numproc);
    // }


    /* fork a child and exec argv[2] */
    int id = cfork(argv[2]);

    if (id == 0){
        close(0);
        close(1);
        close(2);
        dup(vcfd);
        dup(vcfd);
        dup(vcfd);
        int cstart_rv = cstart(argv[2], argv[1]);
        if(cstart_rv < 0){
            printf("start container failed\n");
            exit(0);
        }
        exec(argv[3], &argv[3]);
        exit(0);
    }

    printf("%s started on %s in container %s\n", argv[3], argv[1], argv[2]);
    exit(0);
}