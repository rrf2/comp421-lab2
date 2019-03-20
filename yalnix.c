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
int num_free_pfn;


//I MADE THESE CHANGES 3/20/19 -- Lucy
struct pcb {
	unsigned int pid;
	struct pte *r0_pointer;
	//TODO: definitely need to keep track of more crap. Probably like parent process n stuff?
}

*pcb idle;


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

	// CREATE THE FREE LIST AND INITIALIZE REGION 1 PAGE TABLE
	for (_i = MEM_INVALID_PAGES; _i < pmem_size / PAGESIZE; _i++) {
		if (_i < used_pages_min || _i > used_pages_max) {
			num_free_pfn ++;
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

	// INITIALIZE REGION 0 PAGE TABLE
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
	TracePrintf(1, "Loading Program\n");
	LoadProgram(cmd_args[0], cmd_args[1]);
	TracePrintf(1, "End of Kernel Start\n");


	// I MADE THESE CHANGES 3/20/19 -- Lucy
	// IDLE PROCESS
	idle = (pcb*) malloc(sizeof(pcb));
	idle -> pid = 0;
	r0_pointer -> r0_page_table;

	LoadProgram("idle", cmd_args, frame);

	//TODO: create actual loop thing. Question: is this literally a sepearte c program???

}

/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
int
LoadProgram(char *name, char **args)
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char **cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int data_bss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    if ((fd = open(name, O_RDONLY)) < 0) {
		TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
		return (-1);
    }

    status = LoadInfo(fd, &li);
    TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
    switch (status) {
	case LI_SUCCESS:
	    break;
	case LI_FORMAT_ERROR:
	    TracePrintf(0,
		"LoadProgram: '%s' not in Yalnix format\n", name);
	    close(fd);
	    return (-1);
	case LI_OTHER_ERROR:
	    TracePrintf(0, "LoadProgram: '%s' other error\n", name);
	    close(fd);
	    return (-1);
	default:
	    TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
	    close(fd);
	    return (-1);
    }
    TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
	li.text_size, li.data_size, li.bss_size);
    TracePrintf(0, "entry 0x%lx\n", li.entry);

    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
		size += strlen(args[i]) + 1;
    }
    argcount = i;
    TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);

    /*
     *  Now save the arguments in a separate buffer in Region 1, since
     *  we are about to delete all of Region 0.
     */
    cp = argbuf = (char *)malloc(size);
    for (i = 0; args[i] != NULL; i++) {
		strcpy(cp, args[i]);
		cp += strlen(cp) + 1;
    }

    /*
     *  The arguments will get copied starting at "cp" as set below,
     *  and the argv pointers to the arguments (and the argc value)
     *  will get built starting at "cpp" as set below.  The value for
     *  "cpp" is computed by subtracting off space for the number of
     *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
     *  terminate the auxiliary vector, a NULL pointer terminating
     *  the argv pointers, and a NULL pointer terminating the envp
     *  pointers) times the size of each (sizeof(void *)).  The
     *  value must also be aligned down to a multiple of 8 boundary.
     */
    cp = ((char *)USER_STACK_LIMIT) - size;
    cpp = (char **)((unsigned long)cp & (-1 << 4));	/* align cpp */
    cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

    text_npg = li.text_size >> PAGESHIFT;
    data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n",
	text_npg, data_bss_npg, stack_npg);

    /*
     *  Make sure we will leave at least one page between heap and stack
     */
    if (MEM_INVALID_PAGES + text_npg + data_bss_npg + stack_npg +
	1 + KERNEL_STACK_PAGES >= PAGE_TABLE_LEN) {
		TracePrintf(0, "LoadProgram: program '%s' size too large for VM\n",
		    name);
		free(argbuf);
		close(fd);
		return (-1);
    }

    /*
     *  And make sure there will be enough physical memory to
     *  load the new program.
     */
    // >>>> The new program will require text_npg pages of text,
    // >>>> data_bss_npg pages of data/bss, and stack_npg pages of
    // >>>> stack.  In checking that there is enough free physical
    // >>>> memory for this, be sure to allow for the physical memory
    // >>>> pages already allocated to this process that will be
    // >>>> freed below before we allocate the needed pages for
    // >>>> the new program being loaded.
    // EDITED
    if (text_npg + data_bss_npg + stack_npg > num_free_pfn) {
		TracePrintf(0,
		    "LoadProgram: program '%s' size too large for physical memory\n",
		    name);
		free(argbuf);
		close(fd);
		return (-1);
    }

    // >>>> Initialize sp for the current process to (char *)cpp.
    // >>>> The value of cpp was initialized above.

    char *sp = (char *)cpp // I Just copied what's in the above comment


    /*
     *  Free all the old physical memory belonging to this process,
     *  but be sure to leave the kernel stack for this process (which
     *  is also in Region 0) alone.
     */
    // >>>> Loop over all PTEs for the current process's Region 0,
    // >>>> except for those corresponding to the kernel stack (between
    // >>>> address KERNEL_STACK_BASE and KERNEL_STACK_LIMIT).  For
    // >>>> any of these PTEs that are valid, free the physical memory
    // >>>> memory page indicated by that PTE's pfn field.  Set all
    // >>>> of these PTEs to be no longer valid.
    for (i = 0; i < PAGE_TABLE_LEN) {
    	struct pte entry = r0_page_table[i];
    	// Check to see if the memory should be freed and free the memory, setting the valid bit to 0
    	if (entry.valid && (entry.pfn * PAGESIZE < KERNEL_STACK_BASE || entry.pfn * PAGESIZE > KERNEL_STACK_LIMIT)) {
    		entry.valid = 0;
    		struct pfn_list_entry new_pfn_entry;
			new_pfn_entry.pfn = entry.pfn;
			new_pfn_entry.next = &free_pfn_head;
			free_pfn_head = new_pfn_entry;
			num_free_pfn ++;
    	}
    }

    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    >>>> Leave the first MEM_INVALID_PAGES number of PTEs in the
    >>>> Region 0 page table unused (and thus invalid)

    /* First, the text pages */
    >>>> For the next text_npg number of PTEs in the Region 0
    >>>> page table, initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_EXEC
    >>>>     pfn   = a new page of physical memory

    /* Then the data and bss pages */
    >>>> For the next data_bss_npg number of PTEs in the Region 0
    >>>> page table, initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_WRITE
    >>>>     pfn   = a new page of physical memory

    /* And finally the user stack pages */
    >>>> For stack_npg number of PTEs in the Region 0 page table
    >>>> corresponding to the user stack (the last page of the
    >>>> user stack *ends* at virtual address USER_STACK_LIMIT),
    >>>> initialize each PTE:
    >>>>     valid = 1
    >>>>     kprot = PROT_READ | PROT_WRITE
    >>>>     uprot = PROT_READ | PROT_WRITE
    >>>>     pfn   = a new page of physical memory

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size)
	!= li.text_size+li.data_size) {
	TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
	free(argbuf);
	close(fd);
	>>>> Since we are returning -2 here, this should mean to
	>>>> the rest of the kernel that the current process should
	>>>> be terminated with an exit status of ERROR reported
	>>>> to its parent process.
	return (-2);
    }

    close(fd);			/* we've read it all now */

    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */
    >>>> For text_npg number of PTEs corresponding to the user text
    >>>> pages, set each PTE's kprot to PROT_READ | PROT_EXEC.

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
	'\0', li.bss_size);

    /*
     *  Set the entry point in the exception frame.
     */
    >>>> Initialize pc for the current process to (void *)li.entry

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
	*cpp++ = cp;
	strcpy(cp, cp2);
	cp += strlen(cp) + 1;
	cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;	/* the last argv is a NULL pointer */
    *cpp++ = NULL;	/* a NULL pointer for an empty envp */
    *cpp++ = 0;		/* and terminate the auxiliary vector */

    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     */
    >>>> Initialize regs[0] through regs[NUM_REGS-1] for the
    >>>> current process to 0.
    >>>> Initialize psr for the current process to 0.

    return (0);
}

int
SetKernelBrk(void *addr) {
	if (virtual_memory) {

	} else {
		kernel_brk = addr;
	}
}


void trap_kernel_handler(ExceptionInfo *info) {
	TracePrintf(1, "Exception: Kernel\n");
}

void trap_clock_handler(ExceptionInfo *info) {
	TracePrintf(1, "Exception: Clock\n");
}

void trap_illegal_handler(ExceptionInfo *info) {
	TracePrintf(1, "Exception: Illegal\n");
	if (info -> code == TRAP_ILLEGAL_ILLOPC) {
		printf("%s\n", "Illegal opcode");
		TracePrintf(1, "Illegal opcode\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLOPN) {
		printf("%s\n", "Illegal operand");
		TracePrintf(1, "Illegal operand\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLADR) {
		printf("%s\n", "Illegal addressing mode");
		TracePrintf(1, "Illegal addressing mode\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_ILLTRP) {
		printf("%s\n", "Illegal software trap");
		TracePrintf(1, "Illegal software trap\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_PRVOPC) {
		printf("%s\n", "Privileged opcode");
		TracePrintf(1, "Privileged opcode\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_PRVREG) {
		printf("%s\n", "Privileged register");
		TracePrintf(1, "Privileged register\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_COPROC) {
		printf("%s\n", "Coprocessor error");
		TracePrintf(1, "Coprocessor error\n");
		return;
	}

	if (info -> code == TRAP_ILLEGAL_BADSTK) {
		printf("%s\n", "Bad stack");
		TracePrintf(1, "Bad stack\n");
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
	TracePrintf(1, "Exception: Memory\n");
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
	TracePrintf(1, "Exception: Math\n");
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
	TracePrintf(1, "Exception: TTY Receive\n");
}

void trap_tty_transmit_handler(ExceptionInfo *info) {
	TracePrintf(1, "Exception: TTY Transmit\n");

}