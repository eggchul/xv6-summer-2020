#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "resumable.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void wakeup1(struct proc *chan);

extern char trampoline[]; // trampoline.S

void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");

      // Allocate a page for the process's kernel stack.
      // Map it high in memory, followed by an invalid
      // guard page.
      char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) (p - proc));
      kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      p->kstack = va;
  }
  kvminithart();
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

// global pid
int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// local pid
int
alloclocalpid(struct container *c) {
  int local_pid;
  acquirecidlock();
  local_pid = c->nextlocalpid;
  c->nextlocalpid ++;
  releasecidlock();

  return local_pid;
}

// Look in the process table in target container for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, return 0.
struct proc*
allocproc(struct container *c)
{
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  if(c->isroot){
    p->local_pid = p->pid;
  }else{
    p->local_pid = alloclocalpid(c);
  }
  p->cont = c;
  // Allocate a trapframe page.
  if((p->tf = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    // kfree(p->tf);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof p->context);
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  return p;
}

struct proc*
getnextprocforcontainer(struct container *c){
  struct proc* p;
  p = &proc[c->nextproctorun];
  //iterate the proc[] and find runnable next proc
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->tf)
    kfree((void*)p->tf);
  p->tf = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  if(p!= 0){
    updatecontproc(-1,p->cont);
  }
  p->cont = 0;
}

// Create a page table for a given process,
// with no user pages, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  mappages(pagetable, TRAMPOLINE, PGSIZE,
           (uint64)trampoline, PTE_R | PTE_X);

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  mappages(pagetable, TRAPFRAME, PGSIZE,
           (uint64)(p->tf), PTE_R | PTE_W);

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, PGSIZE, 0);
  uvmunmap(pagetable, TRAPFRAME, PGSIZE, 0);
  if(sz > 0)
    uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x05, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x05, 0x02,
  0x9d, 0x48, 0x73, 0x00, 0x00, 0x00, 0x89, 0x48,
  0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0xbf, 0xff,
  0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x01,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};

// Set up first user process. // this wil be in root container
void
userprocinit(struct container *c)
{
  struct proc *p;

  p = allocproc(c);
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->tf->epc = 0;      // user program counter
  p->tf->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = idup(c->rootdir);
  
  p->state = RUNNABLE;

  //update the root container, we do not check if exceed in root since we assume it is unlimited so far, but need to modify in future
  p->cont = c;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  // if(myproc() != 0){
  //   int rv = updatecontmem(n, myproc()->cont);
  // }
  sz = p->sz;
  if(n > 0){
    //if ( n + p->cont->usedmem > p->cont->maxmem)
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;

  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(struct container * c)
{
  int i, pid;
  struct proc *np, *parent;
  struct container *cont;
	struct inode *cwd;
  struct proc *p = myproc();
  if(c == 0){// from root
    cwd = p->cwd;
		cont = p->cont;
		parent = p;
  }else {
		cwd = c->rootdir;
		cont = c;
		parent = initproc;
  }
  // Allocate process.
  if((np = allocproc(cont)) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }


  np->sz = p->sz;

  np->parent = parent;

  // copy saved user registers.
  *(np->tf) = *(p->tf);

  // Cause fork to return 0 in the child.
  np->tf->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->state = RUNNABLE;
  release(&np->lock);

  if(pid > 0){
    if(updatecontproc(1, np->cont) < 0){
      printf("Too many process in container \n");
      kcstop(np->cont->name);
      exit(0);
    }
  }

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold p->lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    // this code uses pp->parent without holding pp->lock.
    // acquiring the lock first could cause a deadlock
    // if pp or a child of pp were also in exit()
    // and about to try to lock p.
    if(pp->parent == p){
      // pp->parent can't change between the check and the acquire()
      // because only the parent changes it, and we're the parent.
      acquire(&pp->lock);
      pp->parent = initproc;
      // we should wake up init here, but that would require
      // initproc->lock, which would be a deadlock, since we hold
      // the lock on one of init's children (pp). this is why
      // exit() always wakes init (before acquiring any locks).
      release(&pp->lock);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  // we might re-parent a child to init. we can't be precise about
  // waking up init, since we can't acquire its lock once we've
  // acquired any other proc lock. so wake up init whether that's
  // necessary or not. init may miss this wakeup, but that seems
  // harmless.
  acquire(&initproc->lock);
  wakeup1(initproc);
  release(&initproc->lock);

  // grab a copy of p->parent, to ensure that we unlock the same
  // parent we locked. in case our parent gives us away to init while
  // we're waiting for the parent lock. we may then race with an
  // exiting parent, but the result will be a harmless spurious wakeup
  // to a dead or wrong process; proc structs are never re-allocated
  // as anything else.
  acquire(&p->lock);
  struct proc *original_parent = p->parent;
  release(&p->lock);
  
  // we need the parent's lock in order to wake it up from wait().
  // the parent-then-child rule says we have to lock it first.
  acquire(&original_parent->lock);

  acquire(&p->lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup1(original_parent);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&original_parent->lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  // hold p->lock for the whole time to avoid lost
  // wakeups from a child's exit().
  acquire(&p->lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      // this code uses np->parent without holding np->lock.
      // acquiring the lock first would cause a deadlock,
      // since np might be an ancestor, and we already hold p->lock.
      if(np->parent == p){
        // np->parent can't change between the check and the acquire()
        // because only the parent changes it, and we're the parent.
        acquire(&np->lock);
        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&p->lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&p->lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&p->lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &p->lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  struct container *cont;
  int i;
  int start;
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    // &proc[cont->nextproctorun];
    i = 0;

    cont = getnextconttosched();

    for(p = proc; p < &proc[NPROC] && i < cont->used_proc; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE && p->cont->cid == cont->cid) {
        start = ticks;
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->scheduler, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        cont->ticks += (ticks - start);
        i++;
      }
      release(&p->lock);
    }
    if(i == cont->used_proc) {
       //reset this container's ticks?
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  if(lk != &p->lock){  //DOC: sleeplock0
    acquire(&p->lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
    }
    release(&p->lock);
  }
}

// Wake up p if it is sleeping in wait(); used by exit().
// Caller must hold p->lock.
static void
wakeup1(struct proc *p)
{
  if(!holding(&p->lock))
    panic("wakeup1");
  if(p->chan == p && p->state == SLEEPING) {
    p->state = RUNNABLE;
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;
  struct container *c = myproc()->cont;
  if(c->isroot != 1){
    //kill in current(non root) container
    return containerkillwithpid(c, pid);
  }
  // kill in root container
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      if(p->cont->isroot != 1){ // process is not in root container
        printf("Root container has no access to this pid\n");
        release(&p->lock);
        return -1;
      }
      p->killed = 1;
      if(p->state == SLEEPING || p->state == SUSPENDED){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      // updatecontproc(-1, c);
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [SUSPENDED] "suspended"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

int
numprocs(void)
{
  struct proc *p;
  int count = 0;

  for(p = proc; p < &proc[NPROC]; p++){
    if (p->state != UNUSED) {
      count += 1;
    }
  }

  return count;
}

// kernel turn tracing on, proc->tracing = 1
int 
ktraceon(void)
{
  myproc()->tracing = 1;
  return 1;
}

//write running process information into where the pinfo pointer points to
int 
kpinfo(uint64 info_p)
{
  struct pinfo psinfo;
  struct pdata *pd_array = psinfo.pdata_array;
  int count = 0;
  struct proc *p;
  int isroot = myproc()->cont->isroot;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(isroot){
      acquire(&p->lock);
      if(p->state != UNUSED) {
        pd_array[count].pid = p->pid;
        strncpy(pd_array[count].name, p->name, 16);
        pd_array[count].mem = p->sz;
        pd_array[count].local_pid = p->local_pid;
        strncpy(pd_array[count].contname, p->cont->name, 16);
        count +=1;
      }
      psinfo.numproc = count;
      release(&p->lock);
    }else{
      acquire(&p->lock);
      if((p->cont == myproc()->cont) && (p->state != UNUSED)) {
        strncpy(pd_array[count].name, p->name, 16);
        pd_array[count].mem = p->sz;
        pd_array[count].local_pid = p->local_pid;
        count +=1;
      }
      psinfo.numproc = count;
      release(&p->lock);
    }

  }
  copyout(myproc()->pagetable, info_p, (void *) &psinfo, sizeof(struct pinfo));
  if(isroot){
    return 6;
  }else{
    return 0;
  }
}

int 
ksuspend(int pid, int fd, struct file *f)
{
  struct proc *p;
  int found = 0;

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->pid != pid){
        continue;
    }
    found = 1;
    
    acquire(&p->lock);
    p->state = SUSPENDED;
    // p->chan = 0;
    // p->state = SLEEPING;
    release(&p->lock);

    struct resumehdr hdr;
    //assign value to hdr instances
    hdr.mem_sz = p->sz;
    hdr.code_sz = p->sz - 2 * PGSIZE;
    hdr.stack_sz = PGSIZE;
    hdr.tracing = p->tracing;
    strncpy(hdr.name, p->name, 16);
    // printf("mem: %d, code: %d, stack : %d, tracing: %d, name: %s\n", hdr.mem_sz, hdr.code_sz, hdr.stack_sz, hdr.tracing, hdr.name);
    pagetable_t tmp = myproc()->pagetable;  
    myproc()->pagetable = p->pagetable;
    filewritefromkernel(f, (uint64) &hdr, sizeof(struct resumehdr));      // write header 32
    filewritefromkernel(f, (uint64) p->tf, sizeof(struct trapframe));     // write trapframe 288 
    filewrite(f, (uint64) 0, hdr.code_sz);                                // write code+data
    filewrite(f, (uint64) (hdr.code_sz + PGSIZE), PGSIZE);                // write stack
    myproc()->pagetable = tmp;
  
    return 1;
  }
  
  if (found == 0) {
    printf("invalid process id\n");
    return 0;
  }

  return 0;
}

int 
kresume(char *path)
{
  struct resumehdr hdr;
  struct inode *ip;
  pagetable_t pagetable = 0, oldpagetable;
  struct proc *p = myproc();
  uint64 oldsz = p->sz;
  uint64 sz;

  begin_op(ROOTDEV);

  if((ip = namei(path)) == 0){
    end_op(ROOTDEV);
    return -1;
  }

  ilock(ip);

  // Check header
  if (readi(ip, 0, (uint64) &hdr, 0, sizeof(struct resumehdr)) != sizeof(struct resumehdr)) {
      goto bad;
  }
  int pg_begin = sizeof(struct resumehdr);

  //load trapframe
  if (readi(ip, 0, (uint64) p->tf, pg_begin, sizeof(struct trapframe)) < sizeof(struct trapframe)) {
      goto bad;
  }
  pg_begin += sizeof(struct trapframe);
  
  if ((pagetable =  proc_pagetable(p)) == 0) {
      goto bad;
  }

  // Load program into memory.
  sz = 0;
  
  // load code/data
  if ((sz = uvmalloc(pagetable, sz, hdr.code_sz)) == 0) {
      goto bad;
  }
  if (loadseg(pagetable, 0, ip, pg_begin, hdr.code_sz) < 0) {
      goto bad;
  }
  pg_begin += hdr.code_sz;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);

  if ((sz = uvmalloc(pagetable, sz, sz + 2 * PGSIZE)) == 0) {
      goto bad;
  }
  uvmclear(pagetable, sz-2*PGSIZE);

  // load stack
  if (loadseg(pagetable, hdr.code_sz + PGSIZE, ip, pg_begin, PGSIZE) < 0) {
      goto bad;
  }

  iunlockput(ip);
  ip = 0;

  strncpy(p->name, hdr.name, 16);
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->tracing = hdr.tracing;

  proc_freepagetable(oldpagetable, oldsz);
  return 0;

  bad:
  if(pagetable){
    proc_freepagetable(pagetable, sz);
  }
  if(ip){
    iunlockput(ip);
    end_op(ROOTDEV);
  }
  return -1;
}

int
containerkillwithpid(struct container *c, int local_pid)
{
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->cont == c){
      acquire(&p->lock);
      if(p->local_pid == local_pid){
        p->killed = 1;
        if(p->state == SLEEPING || p->state == SUSPENDED){
          // Wake process from sleep().
          p->state = RUNNABLE;
        }
        release(&p->lock);
        // updatecontproc(-1, c);
        return 0;
      }
      release(&p->lock);
    }
  }
  return -1;
}

//helper method to support container.c/kcstop
int
containerkillall(struct container *c)
{
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->cont == c){
      acquire(&p->lock);
      p->killed = 1;
      if(p->state == SLEEPING || p->state == SUSPENDED){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
  return -1;
}

int
totalusedproc(void)// count the number of process
{
  struct proc *p;
  int count = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state != UNUSED) {
      count ++;
    }
    release(&p->lock);
  }
  return count;
}

uint64
totalusedmem(void)// count the total of used memory
{
  struct proc *p;
  uint64 totalmem = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state != UNUSED) {
      totalmem += p->sz;
    }
    release(&p->lock);
  }
  return totalmem;
}

int
usedprocforcontainer(struct container *c)
{
  struct proc *p;
  int count = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->cont == c){
      acquire(&p->lock);
      if(p->state != UNUSED) {
        count ++;
      }
      release(&p->lock);
    }
  }
  return count;
}

uint64
usedmemforcontainer(struct container *c)// count the used memory
{
  struct proc *p;
  uint64 totalmem = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->cont == c){
      acquire(&p->lock);
      if(p->state != UNUSED) {
        totalmem += p->sz;
      }
      release(&p->lock);
    }
  }
  return totalmem;
}


void
pauseprocforcontainer(struct container *c)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->cont == c){
      if(p->state == SLEEPING || p->state == RUNNABLE){
        // Wake process from sleep().
        p->state = SUSPENDED;
      }
    }
    release(&p->lock);
  }
}

void
resumeprocforcontainer(struct container *c)
{
  printf("resuming process...\n");
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->cont == c){
      if(p->state == SUSPENDED){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
    }
    release(&p->lock);
  }
  printf("resume processes ok\n");
}

void 
stopprocforcontainer(struct container* c)
{
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->cont == c){
      p->killed = 1;
      if(p->state == SLEEPING || p->state == SUSPENDED){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
    }
    release(&p->lock);
  }
}
