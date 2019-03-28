#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
	printf("FORKTEST\n");
    if (Fork() == 0) {
		printf("CHILD\n");
    }
    else {
		printf("PARENT\n");
		Delay(8);
		printf("Done Delaying\n");
    }

    Exit(0);
}
