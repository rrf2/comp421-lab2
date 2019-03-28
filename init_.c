#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
    printf("Forking\n");
    int pid;
    if ((pid = Fork()) == 0) {
    	printf("Child\n");
	    char **argvec = malloc(sizeof(char*));
		Exec("initbasic", argvec);
		Exit(0);
    } else {
    	printf("Parent\n");
    	Exit(0);
    }
}
