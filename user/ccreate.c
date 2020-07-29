#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


// create a container anad populate a container's file system
int
main(int argc, char *argv[]){
    if(argc < 2){
        printf("too few arguments\n");
        printf("usage: ccreate <container name>\n");
    }
    if (mkdir(argv[1]) != 0) {
		printf( "Error creating directory %s\n", argv[2]);
		exit(0);
	}

	// for (i = last_flag + 1, k = 0; i < argc; i++, k++) {
	// 	if (cp(argv[2], argv[i]) != 1) 
	// 		printf(1, "Failed to copy %s into folder %s. Continuing...\n", argv[i], argv[2]);
	// }  
    if((ccreate(argv[1])) == 1){
        printf("container %s is created\n",argv[1]);
    }else {
        printf("create container failed\n");
    }
    exit(0);
}
