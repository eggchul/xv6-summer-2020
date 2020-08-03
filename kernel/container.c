#include "cinfo.h"
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "resumable.h"
#include "proc.h"

struct container *root_container;
struct spinlock cid_lock;
void acquirecidlock(void) { acquire(&cid_lock); }

void releasecidlock(void) { release(&cid_lock); }

struct container cont[NCONT];
struct proc *proctable;

//alloccont: allocate container to an unused container
static struct container*
alloccont(void)
{
	struct container *c;
	for(c = cont; c < &cont[NCONT]; c++){
		acquire(&c->lock);
		if(c->state == CUNUSED){
			goto found;
		}
		release(&c->lock);
	}
	return 0;

found:	
	c->state = CRUNNABLE;
	release(&c->lock);
	return c;
}

// find container with cid
struct container*
cid2cont(int cid)
{
	struct container* c;
	int i;

	for (i = 0; i < NCONT; i++) {
		c = &cont[i];
		if (c->cid== cid && c->state != CUNUSED)
			return c;
	}
	return 0;
}

// find container with name
struct container*
name2cont(char* name)
{
	struct container* c;
	int i;

	for (i = 0; i < NCONT; i++) {
		c = &cont[i];
		printf("cont name: %s, t name: %s\n", c->name, name);
		if (strncmp(name, c->name, strlen(name)) == 0){
			return c;
		}
	}
	return 0;
}

int
name2cid(char* name)
{
	struct container* c;
	int i;

	for (i = 0; i < NCONT; i++) {
		c = &cont[i];
		if (strncmp(name, c->name, strlen(name)) == 0)
			return c->cid;
	}
	return -1;// if no container found
}

int
updatecontmem(uint64 memchanged, struct container *c)
{
	acquire(&c->lock);
	if(c->used_mem + memchanged > c->max_sz){
		printf("Container memory exceed\n");
		release(&c->lock);
		return -1;
	} else {
		c->used_mem = c->used_mem + memchanged;
		release(&c->lock);
	}
	return 1;
}

int 
updatecontdsk(uint64 dskchanged, struct container *c)
{
	acquire(&c->lock);
	if(c->used_dsk + dskchanged > c->max_dsk){
		printf("Container disk exceed\n");
		release(&c->lock);
		return -1;
	}else{
		c->used_dsk = c->used_dsk + dskchanged;
		release(&c->lock);
	}
	return 1;
}

int
updatecontproc(int procchange, struct container *c)
{
	acquire(&c->lock);
	if(c->used_proc + procchange > c->max_proc){
		printf("Container proc exceed\n");
		release(&c->lock);
		return -1;
	}else{
		c->used_proc = c->used_proc + procchange;
		release(&c->lock);
	}
	return 1;
}

struct container*
getnextconttosched()
{
	int i;
	struct container *c = &cont[0];
	int holder = cont[0].ticks;
	for(i = 1; i < NCONT; i++){
		if(cont[i].ticks < holder && cont[i].state == CRUNNABLE){
			c = &cont[i];
		}
	}
	return c;
}

// if the vc is used by other container
int 
ifvcavaliable(char* vcname){
	struct container* c;
	int i;
	for (i = 0; i < NCONT; i++) {
		c = &cont[i];
		if (strncmp(vcname, c->vc_name, strlen(vcname)) == 0 && c->state != CUNUSED){
			printf("vc is used\n");
			return -1;
		}
	}
	return 1;
}


void
cinit(void)
{
	initlock(&cid_lock, "nextcid");
	int i;
	for(i = 0; i < NCONT; i ++){
		initlock(&cont[i].lock, "cont");
		cont[i].cid = i;
	}
	procinit();
}

//continit initialize the root container
struct container*
initrootcont(void)
{
	struct container *c;
	struct inode *rootdir;

	if ((c = alloccont()) == 0) 
		panic("Can't alloc init container.");
	printf("done initing root container\n");
	if ((rootdir = namei("/")) == 0)
		panic("Can't set '/' as root container's rootdir");
	printf("done initing root dir\n");
	acquire(&c->lock);
	c->max_proc = NPROC;
	c->max_sz = MAX_CMEM;
	c->max_dsk = MAX_CDISK;	
	c->state = CRUNNABLE;	
	c->rootdir = idup(rootdir);
    c->isroot = 1; // this is the root container
    c->nextproc = 1;
	c->ticks = 0;
	safestrcpy(c->name, "rootcont", sizeof(c->name));	
	release(&c->lock);

	return c;
}

// Set up first user process with container
void
userinit(void)
{
	struct container *root;
	root = initrootcont();
	userprocinit(root);
}

//fork process to target container by cid
int
kcfork(char* name)
{
	struct container* c;
	if ((c = name2cont(name)) == 0){
		printf("Error: cfork, cannot find container %s\n", name);
		return -1;
	}
	if(updatecontproc(1, c)<0){
		return kcstop(c->name);
	}
	return fork(c);
}
//contceate
int
kccreate(char *name){
	struct container *c;
	struct inode *rootdir;
    
	if((name2cont(name)) != 0){
		printf("Container %s is created already\n", name);
		return -1;
	}

	if((c = alloccont()) == 0){
		printf("Can't alloc init container.");
		return -1;
	}

	if((rootdir = namei(name)) == 0){
		printf("Cannot find path\n");
		c->state = CUNUSED;
		return -1;
	}
	
	acquire(&c->lock);
	c->max_proc = NPROC/NCONT;
	c->max_sz = MAX_CMEM;
	c->max_dsk = MAX_CDISK;
	c->rootdir = rootdir;
	strncpy(c->name, name, 16);
	printf("cont name %s\n",c->name);
	c->state = !CUNUSED;	
	c->nextproc = 1;
	c->isroot = 0;
	release(&c->lock);	

	return 1; 
}

//contstart take in container name and vc name
int kcstart(char* name, char* vcname) 
{
	struct container *c;
	if(ifvcavaliable(vcname) < 0){
		return -1;
	}
	c = name2cont(name);
	if(c->state != CRUNNABLE){
		printf("Error: kcstart, container %s is started\n", name);
		return -1;
	}
	if(c == 0){
		printf("Error: kcstart, no container %s found\n", name);
		return -1;
	}
	printf("container found\n");
	acquire(&c->lock);
	strncpy(c->vc_name, vcname, 16);
	c->state = CRUNNING;
	release(&c->lock);
	return 1;
}

//contpause
int 
kcpause(char* name)
{
	struct container *c;
	c = name2cont(name);
	if(c == 0){
		printf("Error: kcspause, no container %s found\n", name);
		return -1;
	}
	if(c->isroot){
		printf("Error: kcpause,  cannot pause root container\n");
		return -1;
	}
	if (c->state != CRUNNABLE && c->state != CPAUSED){
		printf("Error: kcpause, cannot pause a not running container\n");
		return -1;
	}
	acquire(&c->lock);
	c->state = CPAUSED;
	release(&c->lock);
	return 1;
}

//contstop == kill
int 
kcstop(char* name)
{
	struct container *c;
	c = name2cont(name);
	if(c == 0){
		printf("Error: kcstop, no container %s found\n", name);
		return -1;
	}
	if(c->isroot){
		printf("Error: kcstop,  cannot stop root container\n");
		return -1;
	}
	
	if (c->state != CRUNNABLE && c->state != CPAUSED){
		printf("Error: kcstop, cannot kill a not running container\n");
		return -1;
	}

	acquire(&c->lock);
	containerkillall(c);
	c->state = CSTOPPED;
	release(&c->lock);
	freecontainer(c);
	return 1;
}

//contfree: free the container similar to exit
int
freecontainer(struct container *c)
{ 
	acquire(&c->lock);
	c->name[0] = 0;
	c->max_dsk = 0;
	c->max_sz = 0;
	c->max_proc = 0;
	c->rootdir = 0;
	c->nextproc = 0;
	c->used_dsk = 0;
	c->used_mem = 0;
	c->used_proc = 0;
	c->state = CUNUSED;	
	release(&c->lock);
	return 1;
}

// cresume
int
kcresume(char* name)
{
	struct container *c;
	c = name2cont(name);
	if(c == 0){
		printf("Error: kcresume, no container %s found\n", name);
		return -1;
	}
	
	if (c->state != CRUNNABLE && c->state != CPAUSED){
		printf("Error: kcresume, container is already in progress\n");
		return -1;
	}

	acquire(&c->lock);
	c->state = CRUNNABLE;
	release(&c->lock);
	return 1;
}


// continfo
/*
It will show each container
The name of the container
The directory associated with the container
The max number of processes, the max amount of memory allocated, and the max disk space allocated
The amount of used/available processes, memory, and disk space
The processes running in the container
The execution statistics and percent of CPU consumed by each process and each container
*/
int
continfo(uint64 info_c)
{
  struct container *c;
  int isroot = myproc()->cont->isroot;

  for(c = cont; c < &cont[NCONT]; c++) {
    if(isroot){
      acquire(&c->lock);

      release(&c->lock);
    }else{
      acquire(&c->lock);

      release(&c->lock);
    }

  }

  if(isroot){
    return 6;
  }else{
    return 0;
  }
}


