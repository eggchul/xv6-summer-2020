#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/cinfo.h"
#include "kernel/pinfo.h"

#define B2KB 1024
#define B2MB 1048576
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
main(int argc, char* argv[])
{
    struct continfo cf;
    int rv = cinfo(&cf);
    if(rv <0){
        printf("Current container has no permission\n");
        exit(0);
    }else{
        struct pinfo pf;
        int rv2 = psinfo(&pf);
        if(rv2 < 0){
            printf("failed to get proc info\n");
            exit(0);
        }
        struct cinfo *cd = cf.conts;
        struct pdata *pd = pf.pdata_array;
        int a;
        for(a = 0; a < NCONT; a ++){
            if(strcmp(cd[a].cname, "rootcont")== 0){
                printf("==== Container: %s\tDirectory: . \tState: %s \tTicks %d ====\n", cd[a].cname, cd[a].state, cd[a].ticks);
            }else{
                printf("==== Container: %s\tDirectory: %s \tState: %s \tTicks %d ====\n", cd[a].cname, cd[a].cname, cd[a].state, cd[a].ticks);
            }
            printf("\t\t USED \t\t FREE\n");
            printf("DISK:\t\t%d Kb \t\t %d Kb\n", cd[a].used_dsk/B2KB, (cd[a].max_dsk - cd[a].used_dsk)/B2KB);
            printf("MEM :\t\t%d Kb\t\t %d Kb\n", cd[a].used_mem/B2KB, (cd[a].max_sz - cd[a].used_mem)/B2KB);
            printf("PROC:\t\t%d \t\t %d\n", cd[a].used_proc, (cd[a].max_proc - cd[a].used_proc));
            printf("\n");
            int i;
            printf("PID\tPID(local)\tMEM\tNAME\tCONT_NAME\t\n");
            for(i = 0; i < pf.numproc; i ++) {
                if(strcmp(pd[i].contname, cd[a].cname)==0){
                    printf("%d\t%d\t\t%d\t%s\t%s\n", pd[i].pid, pd[i].local_pid, pd[i].mem, pd[i].name, pd[i].contname);
                }
            }
            printf("\n");
            printf("----------------------------------\n");
            printf("\n");
        }

    }
    exit(0);
}