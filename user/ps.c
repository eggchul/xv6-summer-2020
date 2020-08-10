#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pinfo.h"
int main(int argc, char *argv[])
{
    struct pinfo run_proc;
    int rv = psinfo(&run_proc);
    struct pdata *pd = run_proc.pdata_array;
    if(rv == 6){
        int i;
        printf("PID\tPID(local)\tMEM\tNAME\tCONT_NAME\n");
        for(i = 0; i < run_proc.numproc; i ++) {
            printf("%d\t%d\t\t%d\t%s\t%s\n", pd[i].pid, pd[i].local_pid, pd[i].mem, pd[i].name, pd[i].contname);
        }
    }else if(rv == 0){
        int i;
        printf("PID\tMEM\tNAME\n");
        for(i = 0; i < run_proc.numproc; i ++) {
            printf("%d\t%d\t%s\n", pd[i].local_pid, pd[i].mem, pd[i].name);
        }
    }else{
        printf("failed to get proc info\n");
    }
    exit(0);
}
