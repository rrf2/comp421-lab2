#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdio.h>

int
main() {
	TracePrintf(1, "IDLE\n");
	while(1) {
		// printf("%s\n", "idle...\n");
		Pause();
	}
}
