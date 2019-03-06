#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdio.h>


// struct pfn {
// 	unsigned int pfn	: 20;	/* page frame number */
// };

struct pfn_list_entry {
	unsigned int pfn	: 20;
	struct pfn_list_entry *next;
};

// struct page_table {
// 	struct pte pte_entries[PAGE_TABLE_LEN];
// };

int virtual_memory;
void *kernel_brk;

void *interrupt_vector[TRAP_VECTOR_SIZE];
struct pte r0_page_table[PAGE_TABLE_LEN];
struct pte r1_page_table[PAGE_TABLE_LEN];


// void (*interrupt_vector[TRAP_VECTOR_SIZE])(ExceptionInfo)
// struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args);
void trap_kernel_handler(ExceptionInfo *info);
void trap_clock_handler(ExceptionInfo *info);
void trap_illegal_handler(ExceptionInfo *info);
void trap_memory_handler(ExceptionInfo *info);
void trap_math_handler(ExceptionInfo *info);
void trap_tty_receive_handler(ExceptionInfo *info);
void trap_tty_transmit_handler(ExceptionInfo *info);
int SetKernelBrk(void *addr);

struct pte entry;
int used_pages_min;
int used_pages_max;
struct pfn_list_entry free_pfn_head;
int _i;
struct pfn_list_entry list_entry;
unsigned int prot;


void
KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {

	TracePrintf(1, "Tracing\n");
	// KEEP TRACK OF THE KERNEL BRK and VIRTUAL MEMORY FLAG
	kernel_brk = orig_brk;
	virtual_memory = 0;

	// CREATE INrERRUPT VECTOR
	interrupt_vector[TRAP_KERNEL] = trap_kernel_handler;
	interrupt_vector[TRAP_CLOCK] = trap_clock_handler;
	interrupt_vector[TRAP_ILLEGAL] = trap_illegal_handler;
	interrupt_vector[TRAP_MEMORY] = trap_memory_handler;
	interrupt_vector[TRAP_MATH] = trap_math_handler;
	interrupt_vector[TRAP_TTY_RECEIVE] = trap_tty_receive_handler;
	interrupt_vector[TRAP_TTY_TRANSMIT] = trap_tty_transmit_handler;

	// WRITE TO REG_VECTOR_BASE
	WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interrupt_vector);



	// DETERMINE ALREADY USED PAGES
	used_pages_min = DOWN_TO_PAGE(VMEM_1_BASE) / PAGESIZE;
	used_pages_max = UP_TO_PAGE(orig_brk) / PAGESIZE;
	TracePrintf(1, "VMEM_1_BASE: %x\n", VMEM_1_BASE);
	TracePrintf(1, "orig_brk: %x\n", orig_brk);
	TracePrintf(1, "KERNEL_STACK_BASE: %x\n", KERNEL_STACK_BASE);
	TracePrintf(1, "Used pages min: %d\n", used_pages_min);
	TracePrintf(1, "Used pages max: %d\n", used_pages_max);
	// USED PAGES -> VEM_BASE_1 down to page, and orig_brk up to page

	// CREATE THE HEAD OF THE FREE LIST
	free_pfn_head.pfn = 0;
	// free_pfn_head.next = NULL;

	//INITIALIZE Region 1 PAGE TABLE

	TracePrintf(1, "r0_page_table addr: %x\n", r0_page_table);
	TracePrintf(1, "r1_page_table addr: %x\n", r1_page_table);

	// CREATE THE FREE LIST
	for (_i = MEM_INVALID_PAGES; _i < pmem_size / PAGESIZE; _i++) {
		if (_i < used_pages_min || _i > used_pages_max) {
			list_entry.pfn = _i;
			list_entry.next = &free_pfn_head;
			free_pfn_head = list_entry;
		} else {
			TracePrintf(1, "PTE %d\n", _i);
			entry.pfn = _i;
			entry.valid = 1;
			if (_i < UP_TO_PAGE(&_etext) / PAGESIZE) {
				prot = PROT_READ|PROT_EXEC;
			} else {
				prot = PROT_READ|PROT_WRITE;
			}
			entry.uprot = PROT_NONE;
			entry.kprot = prot;
			r1_page_table[_i - used_pages_min] = entry;
		}
	}

	for (_i = 0; _i < used_pages_min; _i ++) {
		// TracePrintf(1, "%d\n", _i);
		entry.pfn = _i;
		if (_i < KERNEL_STACK_BASE / PAGESIZE) {
			entry.valid = 0;
			entry.kprot = PROT_READ;
			entry.uprot = PROT_READ|PROT_WRITE; // NOT SURE WHERE TEXT IS
		} else {
			entry.valid = 1;
			entry.kprot = PROT_READ|PROT_WRITE;
			entry.uprot = PROT_NONE;
		}
		r0_page_table[_i] = entry;
	}

	WriteRegister(REG_PTR0, (RCS421RegVal) r0_page_table);
	WriteRegister(REG_PTR1, (RCS421RegVal) r1_page_table);
	TracePrintf(1, "About to enable virtual memory\n");
	WriteRegister(REG_VM_ENABLE, (RCS421RegVal) 1);
	virtual_memory = 1;
	TracePrintf(1, "End of Kernel Start\n");
}

int
SetKernelBrk(void *addr) {
	if (virtual_memory) {

	} else {
		kernel_brk = addr;
	}
}


void trap_kernel_handler(ExceptionInfo *info) {

}

void trap_clock_handler(ExceptionInfo *info) {

}

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

void trap_tty_receive_handler(ExceptionInfo *info) {

}

void trap_tty_transmit_handler(ExceptionInfo *info) {

}