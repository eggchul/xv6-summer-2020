#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i, j;
  int n;
  uint ticks, t1, t2;


  if (argc != 2) {
  	printf(1, "usage: tickstest n\n");
  	exit(0);
  }

  n = atoi(argv[1]);

  t1 = uptime();

  for (i = 0; i < n; i++) {
    for (j = 0; j< 100000; j++) {
      ;
    }
  }

  t2 = uptime();

  ticks = getticks();
  printf("ticks = %d\n", ticks);
  printf("t1    = %d\n", t1);
  printf("t2    = %d\n", t2);
  printf("t2-t1 = %d\n", t2-t1);    

  exit(0);
}