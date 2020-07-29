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
int nextcid = 1;
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
    
	safestrcpy(c->name, "rootcont", sizeof(c->name));	
	release(&c->lock);

	return c;
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
	printf("acquired container lock\n");
	c->rootdir = idup(rootdir);
	printf("idup for cont done\n");
	strncpy(c->name, name, 16);
	c->state = CRUNNABLE;	
	c->isroot = 0;
	release(&c->lock);	
	printf("cont lock release\n");

	return 1; 
}

//contstart

//contpause

//contstop == kill

//contfree
void
freecontainer(struct container *c)
{ 

}

// continfo
int
continfo(uint64 info_c)
{
	return 1;
}

// Set up first user process with container
void
userinit(void)
{
	struct container *root;
	root = initrootcont();
	userprocinit(root);
}
