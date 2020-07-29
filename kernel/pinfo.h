#ifndef PINFO_H
#define PINFO_H

struct pdata {
    int     pid;
    int     local_pid;
    int     mem;
    char    name[16];
    char    contname[16];
};

struct pinfo{
    struct pdata    pdata_array[64];
    int             numproc;
};

#endif
