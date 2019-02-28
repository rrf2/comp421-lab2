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
// struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args);
int SetKernelBrk(void *addr);



void
KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
	// KEEP TRACK OF THE KERNEL BRK and VIRTUAL MEMORY FLAG
	kernel_brk = orig_brk;
	virtual_memory = 0;

	// CREATE INrERRUPT VECTOR
	interruptVector[TRAP_KERNEL] = trap_kernel_handler;
	interruptVector[TRAP_CLOCK] = trap_clock_handler;
	interruptVector[TRAP_ILLEGAL] = trap_illegal_handler;
	interruptVector[TRAP_MEMORY] = trap_memory_handler;
	interruptVector[TRAP_MATH] = trap_math_handler;
	interruptVector[TRAP_TTY_RECEIVE] = trap_tty_receive_handler;
	interruptVector[TRAP_TTY_TRANSMIT] = trap_tty_transmit_handler;

	// WRITE TO REG_VECTOR_BASE
	WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interrupt_vector);


	// struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

	// DETERMINE ALREADY USED PAGES
	struct pfn used_pages_min = DOWN_TO_PAGE(VMEM_1_BASE);
	struct pfn used_pages_max = UP_TO_PAGE(orig_brk)
	printf("%d", used_pages_min)
	printf("%d", used_pages_max)
	// USED PAGES -> VEM_BASE_1 down to page, and orig_brk up to page

	// CREATE THE HEAD OF THE FREE LIST
	struct pfn_list_entry free_pfn_head;
	free_pfn_head.pfn = 0;
	free_pfn_head.next = NULL;

	// CREATE THE FREE LIST
	int i;
	for (i = MEM_INVALID_PAGES; i < pmemsize / PAGESIZE; i++) {
		if (i > used_pages_min || i < used_pages_max) {
			struct pfn_list_entry entry;
			entry.pfn = i;
			entry.next = free_pfn_head;
			free_pfn_head = entry;
		}
	}
}

int
SetKernelBrk(void *addr) {
	if (virtual_memory) {

	} else {
		kernel_break = addr;
	}
}


void trap_kernel_handler(ExceptionInfo *info) {

}

void trap_clock_handler(ExceptionInfo *info)

void trap_illegal_handler(ExceptionInfo *info) {
	if (info -> code == TRAP_ILLEGAL_ILLOPC) {
		printf("%s\n", "Illegal opcode");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLOPN) {
		printf("%s\n", "Illegal operand");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLADR) {
		printf("%s\n", "Illegal addressing mode");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLTRP) {
		printf("%s\n", "Illegal software trap");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_PRVOPC) {
		printf("%s\n", "Privileged opcode");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_PRVREG) {
		printf("%s\n", "Privileged register");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_COPROC) {
		printf("%s\n", "Coprocessor error");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_BADSTK) {
		printf("%s\n", "Bad stack");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_KERNELI) {
		printf("%s\n", "Linux kernel sent SIGILL");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_USERIB) {
		printf("%s\n", "Received SIGILL or SIGBUS from user");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ADRALN) {
		printf("%s\n", "Invalid address alignment");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ADRERR) {
		printf("%s\n", "Non-existent physical address");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_OBJERR) {
		printf("%s\n", "Object-specific HW error");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_KERNELB) {
		printf("%s\n", "Linux kernel sent SIGBUS");
		return;
	}

	else { return; }
}

void trap_memory_handler(ExceptionInfo *info) {
	if (info -> code == TRAP_MEMORY_MAPERR) {
		printf("%s\n", "No mapping at addr");
		return;
	}

	if (info -> code == TRAP_MEMORY_ACCERR) {
		printf("%s\n", "Protection violation at addr");
		return;
	}

	if (info -> code == TRAP_MEMORY_KERNEL) {
		printf("%s\n", "Linux kernel sent SIGSEGV at addr");
		return;
	}

	if (info -> code == TRAP_MEMORY_USER) {
		printf("%s\n", "Received SIGSEGV from user");
		return;
	}

	else { return; }
}

void trap_math_handler(ExceptionInfo *info) {
	if (info -> code == TRAP_MATH_INTDIV) {
		printf("%s\n", "Integer divide by zero");
		return;
	}

	if (info -> code == TRAP_MATH_INTOVF) {
		printf("%s\n", "Integer overflow");
		return;
	}

	if (info -> code == TRAP_MATH_FLTDIV) {
		printf("%s\n", "Floating divide by zero");
		return;
	}

	if (info -> code == TRAP_MATH_FLTOVF) {
		printf("%s\n", "Floating overflow");
		return;
	}

	if (info -> code == TRAP_MATH_FLTUND) {
		printf("%s\n", "Floating underflow");
		return;
	}

	if (info -> code == TRAP_MATH_FLTRES) {
		printf("%s\n", "Floating inexact result");
		return;
	}

	if (info -> code == TRAP_MATH_FLTINV) {
		printf("%s\n", "Invalid floating operation");
		return;
	}

	if (info -> code == TRAP_MATH_FLTSUB) {
		printf("%s\n", "FP subscript out of range");
		return;
	}

	if (info -> code == TRAP_MATH_KERNEL) {
		printf("%s\n", "Linux kernel sent SIGFPE");
		return;
	}

	if (info -> code == TRAP_MATH_USER) {
		printf("%s\n", "Received SIGFPE from user");
		return;
	}

	else { return; }
}

void trap_tty_receive_handler(ExceptionInfo *info)

void trap_tty_transmit_handler(ExceptionInfo *info)