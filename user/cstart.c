#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

int
containerinitdisk(char* path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  int size = 0;
  char curdir[strlen(path)+3];
  char updir[strlen(path)+4];
  strcpy(curdir,path);
  strcpy(&curdir[strlen(path)], "/.\0");
  strcpy(updir,path);
  strcpy(&updir[strlen(path)], "/..\0");

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return 0;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return 0;
  }

  switch(st.type){
  case T_FILE:
    size += st.size;
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path); 
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        continue;
      }
      if((strcmp(buf, curdir) == 0) || (strcmp(buf, updir) == 0)){
          continue;
      }
      size += st.size;
    }
    break;
  }
  close(fd);
  return size;
}

int
unlinkcontainerfiles(char* path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  char curdir[strlen(path)+3];
  char updir[strlen(path)+4];
  strcpy(curdir,path);
  strcpy(&curdir[strlen(path)], "/.\0");
  strcpy(updir,path);
  strcpy(&updir[strlen(path)], "/..\0");

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return 0;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return 0;
  }

  switch(st.type){
  case T_FILE:
    unlink(buf);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path); 
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        continue;
      }
      unlink(buf);
    }
    break;
  }
  close(fd);
  return 0;
}

int
main(int argc, char *argv[]){
  if(argc < 4){
      printf("too few arguments\n");
      printf("usage: cstart <virtual console> <container name> <init program> [max mem (Kb)] [max disk (Kb)] [max proc]\n");
  }

  int vcfd = open(argv[1], O_RDWR);
  if(vcfd < 0){
      printf("cstart: Could not find vc\n");
      exit(0);
  }
  //current container used disk space before start
  char path[strlen(argv[2])+2];
  strcpy(path, "/");
  strcpy(&path[1],argv[2]);
  strcpy(&path[strlen(argv[2])+1],"\0");

  int disksize = containerinitdisk(path);
  // printf("cont dir size: %d\n",disksize);

  if(argc > 4 && argv[4]){
    int newmem = atoi(argv[4]);
    printf(" checking new max mem...\n");
    if(newmem != 0){
        setmaxcmem(argv[2], newmem);
    }
  }

  if(argc > 5 && argv[5]){
    int newdisk = atoi(argv[5]);
    printf(" checking new max disk...\n");
    if(newdisk < disksize){
      printf("input disk size is too small, using default size\n");
    }else {
      setmaxcdsk(argv[2], newdisk);
    }
  }

  if(argc > 6 && argv[6]){
    int numproc = atoi(argv[6]);
    printf(" checking new max proc...\n");
    if(numproc > 0){
      setmaxcproc(argv[2], numproc);
    }
  }

  /* fork a child and exec argv[2] */
  int id = cfork(argv[2]);
  int cstart_rv = 0;
  if (id == 0){
      close(0);
      close(1);
      close(2);
      dup(vcfd);
      dup(vcfd);
      dup(vcfd);
      cstart_rv = cstart(argv[2], argv[1], disksize);
      if(cstart_rv < 0){
          printf("start container failed\n");
          exit(0);
      }else{
          printf("container started\n");
      }

      exec(argv[3], &argv[3]);

      char path[strlen(argv[2])+2];
      strcpy(path, "/");
      strcpy(&path[1],argv[2]);
      strcpy(&path[strlen(argv[2])+1],"\0");
      unlinkcontainerfiles(path);
      unlink(argv[2]);
      exit(0);
  }
  exit(0);
}