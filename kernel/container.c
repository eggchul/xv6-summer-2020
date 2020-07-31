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
		if (strncmp(name, c->name, strlen(name)) == 0 && c->state != CUNUSED)
			return c;
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
		if (strncmp(name, c->name, strlen(name)) == 0 && c->state != CUNUSED)
			return c->cid;
	}
	return -1;// if no container found
}


void
cinit(void)
{
	struct container *c;
	initlock(&cid_lock, "nextcid");
	for(c = cont; c < &cont[NCONT]; c++){
		initlock(&c->lock, "cont");
	}
	procinit();
}

//continit initialize the root container
struct container*
initrootcont(void)
{
	struct container *c;
	struct inode *rootdir;
	printf("initing root container ...\n");
	if ((c = alloccont()) == 0) 
		panic("Can't alloc init container.");

	printf("initing root dir...\n");
	if ((rootdir = namei("/")) == 0)
		panic("Can't set '/' as root container's rootdir");

	acquire(&c->lock);
	c->max_proc = NPROC;
	c->max_sz = MAX_CMEM;
	c->max_dsk = MAX_CDISK;	
	c->state = CRUNNABLE;	
	c->rootdir = idup(rootdir);
    c->isroot = 1; // this is the root container
    c->nextproc = 1;
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

// check target container current free disk space 
int
checkcontdisk(char* funcname, struct container *c, uint64 filesize)
{
	uint64 total = c->used_dsk + filesize;
	if(total > c->max_dsk){
		printf("Error %s: disk space is not enough in container %s\n", funcname, c->name);
		return -1;
	}
	return 0;
}

// check target container current free memory
int
checkcontmem(char* funcname, struct container *c, uint64 procsz)
{
	uint64 total = c->used_mem + procsz;
	if(total > c->max_sz){
		printf("Error %s: memory space is not enough in container %s\n", funcname, c->name);
		return -1;
	}
	return 0;
}

// check target container current process usage
int
checkprocusage(char* funcname, struct container *c)
{
	if(c->max_proc == c->used_proc){
		printf("Error %s: too many process in container %s\n", funcname, c->name);
		return -1;
	}
	return 0;
}

// check used disk space of a container
uint
checkuseddsk(struct container *c){

	return -1;
}


//fork process to target container by cid
int
cfork(int cid)
{
	struct container* c;
	if ((c = cid2cont(cid)) == 0){
		printf("Error: cfork, cannot find container\n");
		return -1;
	}
	checkprocusage("cfork", c);
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
	c->rootdir = idup(rootdir);
	strncpy(c->name, name, 16);
	c->state = CRUNNABLE;	
	c->nextproc = 1;
	c->isroot = 0;
	release(&c->lock);	

	return 1; 
}

//contstart
int kcstart(char* name, char* vcname, struct file *f)
{
	struct container *c;
	c = name2cont(name);
	if(c == 0){
		printf("Error: kcstart, no container %s found\n", name);
		return -1;
	}
	acquire(&c->lock);
	strncpy(c->vc_name, vcname, 16);
	linkvc2cont(c->rootdir, f);
	c->state = CRUNNING;
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
int
continfo(uint64 info_c)
{
	return 1;
}


