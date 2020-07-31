#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[]){
    if(argc < 4){
        printf("too few arguments\n");
        printf("usage: cstart <virtual console> <container name> <init program>\n");
    }

  int vcfd = open(argv[1], O_RDWR);
  if(vcfd < 0){
      printf("Could not find vc\n");
      exit(0);
  }
  int contfd = open(argv[2], O_RDWR);
  if(contfd < 0){
      printf("Could not find container %s\n", argv[2]);
      exit(0);
  }

  /* fork a child and exec argv[1] */
  int id = fork();

  if (id == 0){
    close(0);
    close(1);
    close(2);
    dup(vcfd);
    dup(vcfd);
    dup(vcfd);
    //cstart
    exec(argv[3], &argv[3]);
    exit(0);
  }

  printf("%s started on %s in container %s\n", argv[3], argv[1], argv[2]);
    exit(0);
}