#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"


char buf[512];

void
cp(char* dest, char* src)
{
    int n;
    int pathsize = strlen(dest) + strlen(src) + 2; // dst.len + '/' + src.len + \0
    char path[pathsize]; 

    memmove(path, dest, strlen(dest));
    memmove(path + strlen(dest), "/", 1);
    memmove(path + strlen(dest) + 1, src, strlen(src));
    memmove(path + strlen(src) + 1 + strlen(src), "\0", 1);
    
    int srcfd = open(src, O_RDONLY);
    if(srcfd < 0){
        printf("cp:Failed to open src folder/file %s\n", src);
    }
    int destfd = open(path, O_WRONLY|O_CREATE);
    if(destfd < 0){
        printf("cp:Failed to open dest folder/file %s\n", dest);
    }
    while((n = read(srcfd, buf, sizeof(buf))) > 0) {
        if (write(destfd, buf, n) != n) {
        printf("cp: write error\n");
        exit(1);
        }
    }
    if(n < 0){
        printf("cat: read error\n");
        exit(1);
    }
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
        cp(argv[1],argv[i]);
        //check if over disk size
    }
    if((ccreate(argv[1])) == 1){
        printf("container %s is created\n",argv[1]);
    }else {
        printf("create container failed\n");
    }
    exit(0);
}
