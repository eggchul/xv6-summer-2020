#ifndef RESUMABLE_H
#define RESUMABLE_H

struct resumehdr {
    int mem_sz; // the size of the target process
    int code_sz;
    int stack_sz;
    int tracing;
    char name[16];
};

#endif
