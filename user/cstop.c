#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"


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
    
    char path[strlen(argv[1])+2];
    strcpy(path, "/");
    strcpy(&path[1],argv[1]);
    strcpy(&path[strlen(argv[1])+1],"\0");
    unlinkcontainerfiles(path);
    unlink(argv[1]);
    exit(0);
}