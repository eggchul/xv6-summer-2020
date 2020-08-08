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
		// printf("cont name: %s, t name: %s\n", c->name, name);
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

// set new max disk space for non root containers
int
setmaxdsk(char * contname, uint64 newmax)
{
	struct container *c = name2cont(contname);
	if(c == 0){
		return -1;
	}
	if(c->isroot){
		printf("Cannot set disk limit for root container\n");
		exit(0);
	}
	if(newmax > c->max_dsk){
		printf("assign max disk exceed system limit, will use default size\n");
		return -1;
	}else{
		c->max_dsk = newmax;
	}
	return 1;
}

// set new max mem for non root container 
int
setmaxmem(char *contname, uint64 newmax)
{
	struct container *c = name2cont(contname);
	if(c == 0){
		return -1;
	}
	if(c->isroot){
		printf("Cannot set mem limit for root container\n");
		exit(0);
	}
	if(newmax > c->max_sz){
		printf("assign max disk exceed system limit, will use default size\n");
		return -1;
	}else{
		c->max_sz = newmax;
	}
	return 1;
}

// set new max proc for non root container 
int
setmaxproc(char *contname, int numproc)
{
	struct container *c = name2cont(contname);
	if(c == 0){
		return -1;
	}
	if(c->isroot){
		printf("Cannot set proc limit for root container\n");
		exit(0);
	}
	if(numproc > c->max_proc){
		printf("assign max disk exceed system limit, will use default size\n");
		return -1;
	}else{
		c->max_proc = numproc;
	}
	return 1;
}


int
updatecontmem(uint64 memchanged, struct container *c)
{
	if(c->used_mem + memchanged > c->max_sz){
		printf("Container memory exceed\n");
		return -1;
	} else {
		c->used_mem = c->used_mem + memchanged;
	}
	return 1;
}

int 
updatecontdsk(uint64 dskchanged, struct container *c)
{
	if(c->used_dsk + dskchanged > c->max_dsk){
		printf("Container disk exceed\n");
		return -1;
	}else{
		c->used_dsk = c->used_dsk + dskchanged;
	}
	return 1;
}

int
updatecontproc(int procchange, struct container *c)
{
	//printf("checking:: cont:%s, used:%d, free:%d\n", c->name, c->used_proc, c->max_proc-c->used_proc);
	if(c->used_proc + procchange > c->max_proc){
		printf("Container proc exceed\n");
		return -1;
	}else{
		c->used_proc = c->used_proc + procchange;
		//printf("updated:: cont:%s, used:%d, free:%d\n", c->name, c->used_proc, c->max_proc-c->used_proc);
		if(c->used_proc < 0){
			c->used_proc = 0;
		}
	}
	return 1;
}

struct container*
getnextconttosched(void)
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

//continit initialize the root container
struct container*
initrootcont()
{
	struct container *c;
	struct inode *rootdir;

	// if ((c = alloccont()) == 0) 
	// 	panic("Can't alloc init container.");
	c = &cont[ROOTCONT];
	printf("done initing root container\n");
	if ((rootdir = namei("/")) == 0)
		panic("Can't set '/' as root container's rootdir");
	printf("done initing root dir\n");
	acquire(&c->lock);
	c->max_proc = NPROC;
	c->max_sz = MAX_CMEM;
	c->max_dsk = MAX_CDISK;	
	c->state = CRUNNABLE;	
	c->used_proc = 0;
	c->used_mem = 0;
	c->used_dsk = 0;
	c->rootdir = idup(rootdir);
    c->isroot = 1; // this is the root container
    c->nextlocalpid = 1;
	c->ticks = 0;
	safestrcpy(c->name, "rootcont", sizeof(c->name));	
	release(&c->lock);

	return c;
}


void
cinit(void)
{
	initlock(&cid_lock, "nextcid");
	int i;
	for(i = 0; i < NCONT; i ++){
		initlock(&cont[i].lock, "cont");
		cont[i].cid = i;
		cont[i].state = CUNUSED;
	}
	initrootcont();
	procinit();
}


// Set up first user process with container
void
userinit(void)
{
	userprocinit(&cont[ROOTCONT]);
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
	c->max_sz = MAX_CMEM/NCONT;
	c->max_dsk = MAX_CDISK/NCONT;
	c->used_dsk = 0;
	c->used_mem = 0;
	c->used_proc = 0;
	c->rootdir = rootdir;
	strncpy(c->name, name, 16);
	c->state = CINITED;	
	c->nextlocalpid = 1;
	c->isroot = 0;
	release(&c->lock);	

	return 1; 
}

//contstart take in container name and vc name
int kcstart(char* name, char* vcname, uint64 disksize) 
{
	struct container *c;
	if(ifvcavaliable(vcname) < 0){
		return -1;
	}
	c = name2cont(name);
	if(c->state != CINITED){
		printf("No avaliable container named %s is found\n", name);
		return -1;
	}
	if(c == 0){
		printf("Error: kcstart, no container %s found\n", name);
		return -1;
	}
	updatecontdsk(disksize, c);
	acquire(&c->lock);
	strncpy(c->vc_name, vcname, 16);
	c->state = CRUNNABLE;
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
	pauseprocforcontainer(c);
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
	printf("kcstop checking container %s state\n", c->name);
	if (c->state != CRUNNABLE && c->state != CPAUSED){
		printf("Error: kcstop, cannot kill a not running container\n");
		return -1;
	}

	acquire(&c->lock);
	c->state = CSTOPPED;
	containerkillall(c);
	release(&c->lock);
	freecontainer(c);
	resetticksforcontainers();
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
	c->nextlocalpid = 0;
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
	
	if (c->state != CPAUSED){
		printf("Error: kcresume, container is already in progress\n");
		return -1;
	}

	acquire(&c->lock);
	c->state = CRUNNABLE;
	resumeprocforcontainer(c);
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
	char* statestr[] = {"unused", "inited", "runnable", "paused","stopped"};
	struct container *c = myproc()->cont;
	if(c->isroot != 1){
		printf("Only root container can execute cinfo\n");
		return -1;
	}
	struct continfo cinfoobj;
	struct cinfo* cinfotable = cinfoobj.conts;
	int i;
	uint64 totalmem = 0;
	uint64 totaldisk = 0;
	int totalproc = 0;

	for(i = 0; i < NCONT; i ++) {
		strncpy(cinfotable[i].cname, cont[i].name, 16);
		strncpy(cinfotable[i].state, statestr[cont[i].state], 16);
		cinfotable[i].max_dsk = cont[i].max_dsk;
		cinfotable[i].max_sz = cont[i].max_sz;
		cinfotable[i].max_proc = cont[i].max_proc;
		cinfotable[i].used_dsk = cont[i].used_dsk;
		cinfotable[i].used_mem = cont[i].used_mem;
		cinfotable[i].used_proc = cont[i].used_proc;
		// count for root container
		totalmem = cont[i].used_mem + totalmem;
		totaldisk = cont[i].used_dsk + totaldisk;
		totalproc = cont[i].used_proc + totalproc;
	}

	//assign value for root
	cinfotable[ROOTCONT].max_dsk = cont[ROOTCONT].max_dsk;
	cinfotable[ROOTCONT].max_sz = cont[ROOTCONT].max_sz;
	cinfotable[ROOTCONT].max_proc = cont[ROOTCONT].max_proc;
	cinfotable[ROOTCONT].used_proc = totalproc;
	cinfotable[ROOTCONT].used_mem = totalmem;
	cinfotable[ROOTCONT].used_dsk = totaldisk;

	copyout(myproc()->pagetable, info_c, (void *) &cinfoobj, sizeof(struct continfo));
	return 1;
}

/*
(free) A free command that show the available and used memory
In a container, it should only show the available and used memory within a container
In the root container, it should show all available and used memory
(df) Disk space usage
In a container, it should show only the total disk space a disk space used by the container
In the root container it should show the total disk space available and used
*/

int
kcdf(void)
{
	struct container *c = myproc()->cont;
	
	if(c->isroot){
		int i;
  		uint64 totaldisk = 0;
		uint64 freedsk = 0;
		for(i = 1; i < NCONT; i ++) {
			printf("Container: %s\n", cont[i].name);
			freedsk = cont[i].max_dsk - cont[i].used_dsk;
			printf("Disk: used %d, avaliable %d\n", cont[i].used_dsk, freedsk);
			// count for root container
			totaldisk = cont[i].used_dsk + totaldisk;
		}
		// print root container info
		printf("Container: %s\n", cont[ROOTCONT].name);
		//need to redo calculation
		freedsk = cont[ROOTCONT].max_dsk - cont[ROOTCONT].used_dsk - totaldisk;
		printf("Disk: used %d, avaliable %d\n", cont[ROOTCONT].used_dsk, freedsk);
		return 1;
	}else{
		printf("Container: %s\n", c->name);
		uint64 freedsk = c->max_dsk - c->used_dsk;
		printf("Disk: used %d, avaliable %d\n", c->used_dsk, freedsk);
		return 0;
	}
}

int
kcfreemem(void)
{
	struct container *c = myproc()->cont;
	uint64 freemem = 0;
	if(c->isroot){
		int i;
  		uint64 totalmem = 0;
		for(i = 1; i < NCONT; i ++) {
			printf("Container: %s\n", cont[i].name);
			uint64 usedmem  = usedmemforcontainer(&cont[i]);
			freemem = c->max_sz - usedmem;
			printf("Mem: used %d, avaliable %d\n", usedmem, freemem);
			// count for root container
			totalmem = cont[i].used_dsk + totalmem;
		}
		// print root container info
		printf("Container: %s\n", cont[ROOTCONT].name);
		//need to redo calculation
		freemem = cont[ROOTCONT].max_dsk - cont[ROOTCONT].used_dsk - totalmem;
		printf("Mem: used %d, avaliable %d\n", cont[ROOTCONT].used_mem, freemem);
		return 1;
	}else {
		printf("Container: %s\n", c->name);
		uint64 usedmem  = usedmemforcontainer(c);
		freemem = c->max_sz - usedmem;
		printf("Mem: used %d, avaliable %d\n", usedmem, freemem);
		return 0;
	}
}

void
resetticksforcontainers(){
	for(i = 0; i < NCONT; i++){
		cont[i].ticks = 0;
	}
}
