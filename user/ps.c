#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pinfo.h"
int main(int argc, char *argv[])
{
    struct pinfo run_proc;
    psinfo(&run_proc);
    struct pdata *pd = run_proc.pdata_array;
    int i;
    printf("PID\tMEM\tNAME\n");
    for(i = 0; i < run_proc.numproc; i ++) {
        printf("%d\t%d\t%s\n", pd[i].pid, pd[i].mem, pd[i].name);
    }
    exit(0);
}
