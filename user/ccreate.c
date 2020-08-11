#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"


char buf[512];

int
cp(char* dest, char* src)
{
    int n;
    int pathsize = strlen(dest) + strlen(src) + 2; 
    char path[pathsize]; 
    char buf[1024];

    strcpy(path, dest);
    strcpy(&path[strlen(dest)], "/");
    strcpy(&path[strlen(dest)+1], src);
    strcpy(&path[strlen(dest)+1+strlen(src)], "\0");

    int srcfd = open(src, O_RDONLY);
    if(srcfd < 0){
        printf("cp:Failed to open src folder/file %s\n", src);
        return -1;
    }
    int destfd = open(path, O_WRONLY|O_CREATE);
    if(destfd < 0){
        printf("cp:Failed to open dest folder/file %s\n", dest);
        close(destfd);
        return -1;
    }
    while((n = read(srcfd, buf, sizeof(buf))) != 0) {
        write(destfd, buf, n);
    }
    close(srcfd);
    close(destfd);
    return 1;
}

// create a container anad populate a container's file system
int
main(int argc, char *argv[]){
    if(argc < 2){
        printf("too few arguments\n");
        printf("usage: ccreate <container name> [cmd] [cmd] ... \n");
    }
    if (mkdir(argv[1]) != 0) {
		printf( "Error creating directory %s\n", argv[2]);
		exit(0);
	}

    // copy selected function file to dest file/dir
    int i;
    for(i = 2; i < argc; i++){
        int rv = cp(argv[1],argv[i]);
        if(rv < 0){
            printf("cp %s to %c failed\n", argv[i], argv[1]);
        }
        //check if over disk size
    }
    
    if((ccreate(argv[1])) == 1){
        printf("container %s is created\n",argv[1]);
    }else {
        printf("create container failed\n");
        unlink(argv[1]);
    }
    exit(0);
}
