#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
fib_iter_c(int n) {
    int prev_prev_num = 0, prev_num = 0, cur_num = 1;

    if (n == 0) {
        return 0;
    }
    for (int i = 1; i < n ; i++) {
        prev_prev_num = prev_num;
        prev_num = cur_num;
        cur_num = prev_prev_num + prev_num;
    }
    return cur_num;
}


int
main(int argc, char *argv[])
{
    int i = 0;
    int rv;

    while (1) {
        rv = fib_iter_c(i);
        fprintf(1, "fib_iter(%d) = %d\n", i, rv);
        sleep(30);
        i = i + 1;
    }

    exit(0);
}
