#ifndef CINFO_H
#define CINFO_H
#include "types.h"
#include "param.h"


/*
It will show each container
The name of the container
The directory associated with the container
The max number of processes, the max amount of memory allocated, and the max disk space allocated
The amount of used/available processes, memory, and disk space
The processes running in the container
The execution statistics and percent of CPU consumed by each process and each container
*/

// Information of a single container
struct cinfo {
  char cname[16];        // Container name
  int max_proc;					 // Max amount of processes
  uint64 max_sz;				 // Max size of memory (bytes)
  uint64 max_dsk;				 // Max amount of disk space (bytes)
  int used_proc;				 // Used processes
  uint64 used_mem;       		 // Used pages of memory
  uint64 used_dsk;				 // Used disk space (blocks)
};

// Array of container infos
struct continfo {
	struct cinfo conts[NCONT];
};

#endif
