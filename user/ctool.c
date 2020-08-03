#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if (argc < 2) {
		printf("usage: ctool <cmd> [<arg> ...]");
		exit(0);
	}

    // int i = 1;
    // while(argv[i]){
    //     if (strcmp(argv[1], "create") == 0)
    //         create(argc, argv);
    //     else if (strcmp(argv[1], "start") == 0)
    //         cont_start(argc, argv);
    //     else if (strcmp(argv[1], "info") == 0)
    //         cont_info();
    //     else if (strcmp(argv[1], "resume") == 0)
    //         cont_resume(argc, argv);
    //     else if (strcmp(argv[1], "stop") == 0)
    //         cont_stop(argc, argv);
    //     else if (strcmp(argv[1], "pause") == 0)
    //         cont_pause(argc, argv);
    //     else if (strcmp(argv[1], "ps") == 0)
    //         psinfo(argc, argv);
    //     else 
    //         printf("ctool: command not found.\n"); 
    // }

	exit(0);
}