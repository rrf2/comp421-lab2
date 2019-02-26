#include <comp421/hardware.h>
#include <comp421/yalnix.h>


struct pfn {
	unsigned int pfn	: 20;	/* page frame number */
}

struct pfn_list_entry {
	struct pfn page_frame_number;
	struct *pfn_list_entry next;
}

struct page_table {
	struct pte[PAGE_TABLE_LEN];
}

int virtual_memory;
void *kernel_brk;

void *interrupt_vector[TRAP_VECTOR_SIZE];
struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args);
int SetKernelBrk(void *addr);



void
KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
	//TODO: initialize interrupt vector table

	kernel_brk = orig_brk;

	// interrupt_vector = malloc(TRAP_VECTOR_SIZE * sizeof(void *))

	WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interrupt_vector);

	struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

	// USED PAGES -> &_etext down to page, and orig_brk up to page

	// struct page_table kernel_page_table = &_etext;

	virtual_memory = 0;
}

int
SetKernelBrk(void *addr) {
	if (virtual_memory) {

	} else {
		kernel_break = addr;
	}
}
