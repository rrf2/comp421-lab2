#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern struct pcb *running_proc;

void
funFunc(char name, int pid) {
    printf("%s%d\n", "Running child: ", GetPid());
    Delay(5);
    Exit(1);

}

int
main(int argc, char **argv)
{
    printf("INIT pid: %d\n", GetPid());
    int pid = Fork();

    printf("CHILD PID: %d\n", pid);

    if (pid == 0) {
        funFunc("child", pid);
    }
    printf("Waiting... %d...\n", pid);
    int status;
    int exited_pid = Wait(&status);
    printf("WAIT %d...\n", pid);

    printf("Done with waittest\n");

    // printf("This is the child process info: %d", running_proc -> status_pointer -> proc -> pid);
    Exit(0);
}