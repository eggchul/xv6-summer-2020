#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE      5000 // size of file system in blocks
#define MAXPATH      128 // maximum file path name
#define NCONT         8  // maximum number of containers
#define ROOTCONT      0  // root container cid is 0
#define MAX_CDISK    NPROC*400*1024        // bytes
#define MAX_CMEM     NPROC*1000*4096        // bytes