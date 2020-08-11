/* schedtest.c - run n processes for t wall ticks and report allocations 
We want the output to be printed in child order so we have each child send
its statistics back to the parent via a pipe.
*/

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define MAX_PROCS 10

struct child_info {
  char name[16];
  uint ticks;
};

struct child_pipe {
  int fds[2];
};

void
setchildname(char *filename, int i)
{
  strcpy(filename, "Child:");
  itoa(i, &filename[6], 10);
}

void
sched_child(struct child_info *ci, int child_num, uint wall_ticks, uint end_ticks)
{
  setchildname(ci->name, child_num);
  while(uptime() < end_ticks) {
    ;    
  }
  ci->ticks = getticks();
}

int
main(int argc, char *argv[])
{
  int i;
  int id;
  int nprocs = 0;
  uint wall_ticks = 0;
  uint start_ticks = 0;
  uint end_ticks = 0;
  struct child_info child_infos[MAX_PROCS];
  struct child_pipe child_pipes[MAX_PROCS];

  if (argc != 3) {
    printf("usage: schedtest <num_procs> <wall_ticks>\n");
    exit(0);
  }

  nprocs = atoi(argv[1]);

  if (nprocs >= MAX_PROCS) {
    printf("%d procs maximum, exiting\n", MAX_PROCS);
    exit(0);
  }

  wall_ticks = atoi(argv[2]);
  start_ticks = uptime();
  end_ticks = start_ticks + wall_ticks;

  printf( "schedtest: started\n");

  /* Create 1 pipe for each child */
  for (i = 0; i < nprocs; i++) {
    pipe(child_pipes[i].fds);
  }

  /* Start the children */
  for (i = 0; i < nprocs; i++) {
    id = fork();
    if (id == 0) {
      sched_child(&child_infos[i], i, wall_ticks, end_ticks);
      /* Send child_info stats back to parent */
      write(child_pipes[i].fds[1], (void *) &child_infos[i], sizeof(struct child_info));
      exit(0);
    }
  }

  /* Wait for childern to exit() */
  for (i = 0; i < nprocs; i++) {
    wait(0);
  }

  /* Print run time statistics. */
  for (i = 0; i < nprocs; i++) {
    read(child_pipes[i].fds[0], (void *) &child_infos[i], sizeof(struct child_info));
    printf("Process [%s] ran for %d ticks out of %d total ticks\n",
           child_infos[i].name, child_infos[i].ticks, wall_ticks);
  }

  printf("schedtest: finished\n");
  exit(0);
}