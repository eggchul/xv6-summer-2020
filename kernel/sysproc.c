#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  int rv = argint(0, &n);
  if(myproc()->tracing){
    printf("[%d] sys_exit(%d)\n", myproc()->pid, n);
  }
  if( rv < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  if(myproc()->tracing) {
    printf("[%d] sys_getpid()\n", myproc()->pid);
  }
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  if(myproc()->tracing) {
    printf("[%d] sys_fork()\n", myproc()->pid);
  }
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  int rv = argaddr(0, &p);
  if(myproc()->tracing) {
    printf("[%d] sys_wait(%d)\n", myproc()->pid, p);
  }
  if(rv < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  int rv1 = argint(0, &n);
  if(myproc()->tracing) {
    printf("[%d] sys_sbrk(%d)\n", myproc()->pid, n);
  }
  if( rv1 < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;
  int rv = argint(0, &n);
  if(myproc()->tracing) {
    printf("[%d] sys_sleep(%d)\n", myproc()->pid, n);
  }
  if(rv < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;
  int rv = argint(0, &pid);
  if(myproc()->tracing){
    printf("[%d] sys_kill(%d)\n", myproc()->pid, pid);
  }  
  if(rv < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;
  if(myproc()->tracing) {
    printf("[%d] sys_uptime()\n", myproc()->pid);
  }
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_numprocs(void)
{
  return numprocs();
}


// turn on trace
uint64 
sys_traceon(void)
{
    if(myproc()->tracing) {
      printf("[%d] sys_traceon()\n", myproc()->pid);
    }
    return ktraceon();
}

uint64
sys_psinfo(void)
{
  uint64 info_p;
  argaddr(0, &info_p);
  if(myproc()->tracing) {
    printf("[%d] sys_pinfo()\n", myproc()->pid);
  }
  kpinfo(info_p);
  return 1;
}
