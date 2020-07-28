#ifndef CINFO_H
#define CINFO_H
#include "pinfo.h"
#include "types.h"
#include "param.h"

// Information of a single container
struct cinfo {
  int cid;					     // Container ID
  char name[16];          	     // Container name
  int max_proc;					 // Max amount of processes
  uint64 max_sz;				 // Max size of memory (bytes)
  uint64 max_dsk;				 // Max amount of disk space (bytes)
  int used_proc;				 // Used processes
  uint64 used_mem;       		 // Used pages of memory
  uint64 used_dsk;				 // Used disk space (blocks)
  struct pinfo proc_array[64];   // Array of processes owned by container
};

// Array of container infos
struct continfo {
	struct cinfo conts[NCONT];
	int isroot;					 // 1 if requested cinfo as root
};

#endif
