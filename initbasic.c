#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern struct pcb *running_proc;

void
funFunc(char name, int pid) {
    int status;
    // Delay(5);
    // printf("CHILD PID %d DONE DELAYING\n", pid);
    printf("%s%d\n", "Running child: ", GetPid());
    Exit(1);
    printf("Child exited %d...\n", pid);
    int exited_pid = Wait(&status);
    printf("WAIT %d...\n", pid);
    
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
    Delay(5);
    printf("PARENT DONE DELAYING\n");
    printf("BrK(%x)\n", VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 2);
    Brk((void *) (VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 2));
    printf("BrK(%x)\n", VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 4);
    Brk((void *) (VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 4));
    char **argvec = malloc(sizeof(char*));
    printf("Done with initbasic\n");

    // printf("This is the child process info: %d", running_proc -> status_pointer -> proc -> pid);
    Exit(0);
}
