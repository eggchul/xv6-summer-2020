struct stat;
struct rtcdate;
#include "kernel/pinfo.h"
#include "kernel/cinfo.h"

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int numprocs(void);
int traceon(void);
int psinfo(struct pinfo *);
int suspend(int, int); // pid, fd
int resume(char *); // fd
int getticks(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int itoa(int, char *, int);

//container.c
int ccreate(char *);
int cfork(char*);
int cstart(char *, char*, int);
int cstop(char *);
int cpause(char *);
int cresume(char *);
int cinfo(struct continfo *);
int df(void);
int cfree(void);
int setmaxcmem(char*, int);
int setmaxcdsk(char*, int);
int setmaxcproc(char*, int);
