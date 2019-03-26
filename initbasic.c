#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
	printf("INIT pid: %d\n", GetPid());
    Delay(5);
    printf("DONE DELAYING1\n");
    printf("BrK(%x)\n", VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 2);
    Brk((void *) (VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 2));
    printf("BrK(%x)\n", VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 4);
    Brk((void *) (VMEM_0_BASE + PAGE_TABLE_LEN * PAGESIZE / 4));
    return 0;
}
