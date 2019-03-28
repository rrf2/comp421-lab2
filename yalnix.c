#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


struct pfn_list_entry {
    unsigned int pfn    : 20;
    struct pfn_list_entry *next;
};

//I MADE THESE CHANGES 3/20/19 -- Lucy
struct pcb {
    int init;
    ExceptionInfo *info;
    unsigned int pid;
    SavedContext *ctx;
    struct pte *r0_pointer;
    // void *brk;
    unsigned long brk;
    int queue;
    // unsigned int kstack_pfns[KERNEL_STACK_PAGES];
    struct queue_status *status_pointer;
    struct pcb *parent;

    int delay;
    int exited;

    int end_of_delay;


};

struct queue_elem {
    struct pcb *proc;
    struct queue_elem *next;
};

struct queue_status {
	struct pcb *proc;
	int status;
	struct queue_status *next;
};

// struct page_table {
//  struct pte pte_entries[PAGE_TABLE_LEN];
// };

int time = 0;

int virtual_memory;
void *kernel_brk;

void *interrupt_vector[TRAP_VECTOR_SIZE];
struct pte initial_r0_page_table[PAGE_TABLE_LEN];
struct pte *r0_page_table;
struct pte r1_page_table[PAGE_TABLE_LEN];


// void (*interrupt_vector[TRAP_VECTOR_SIZE])(ExceptionInfo)
// struct pfn_list free_pfn_list = malloc(pmem_size  / PAGESIZE * sizeof(struct pfn));

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args);
unsigned int pfnpop();
void pfnpush(unsigned int pfn);
struct pcb* ready_qpop();
void ready_qpush(struct pcb *proc);
struct pcb* delay_qpop();
void delay_qpush(struct pcb *proc);
int LoadProgram(char *name, char **args, ExceptionInfo *info);
void trap_kernel_handler(ExceptionInfo *info);
void trap_clock_handler(ExceptionInfo *info);
void trap_illegal_handler(ExceptionInfo *info);
void trap_memory_handler(ExceptionInfo *info);
void trap_math_handler(ExceptionInfo *info);
void trap_tty_receive_handler(ExceptionInfo *info);
void trap_tty_transmit_handler(ExceptionInfo *info);
int SetKernelBrk(void *addr);
void copyKernelStack(struct pcb* proc);
SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2);
SavedContext *MyCFunc(SavedContext *ctxp, void *p1, void *p2);

struct pte entry;
int used_pages_min;
int used_pages_max;
struct pfn_list_entry *free_pfn_head;
int _i;
struct pfn_list_entry list_entry;
unsigned int prot;
int num_free_pfn;


struct pcb *idle;
struct pcb *init;

unsigned int pid_counter = 0;
struct pcb *running_proc;

int delay_ticks = 0;
struct pcb *delay_proc;

struct queue_elem dummy;
struct queue_elem *ready_head = &dummy;
struct queue_elem *ready_tail = &dummy;

struct queue_elem *delay_head = &dummy;
struct queue_elem *delay_tail = &dummy;
int num_delay_procs = 0;

struct queue_elem *waiting = &dummy;

struct queue_status dummy2;

unsigned int
pfnpop() {
    unsigned int pfn = free_pfn_head->pfn;
    free_pfn_head = free_pfn_head->next;
    num_free_pfn --;
    // TracePrintf(1, "PFNPOP - old: %d\t new: %d\t\n", pfn, free_pfn_head->pfn);
    return pfn;
}

void
pfnpush(unsigned int pfn) {
    // TracePrintf(1, "PUSH: %d\n", pfn);
    struct pfn_list_entry *new_pfn_entry = malloc(sizeof (struct pfn_list_entry*));
    new_pfn_entry->pfn = pfn;
    new_pfn_entry->next = free_pfn_head;
    free_pfn_head = new_pfn_entry;
    num_free_pfn ++;
    // TracePrintf(1, "PUSHED PFN: %d\t next: %d\n", pfn, free_pfn_head->next->pfn);
}

struct pcb*
ready_qpop() {
    struct pcb *proc = ready_head->proc;
    int head_proc_pid = -1;
    if (proc != NULL) {
        // TracePrintf(1, "head addr: %x\n", head);
        // TracePrintf(1, "dummy addr: %x\n", &dummy);
    	ready_head = ready_head->next;
        // TracePrintf(1, "head addr: %x\n", head);
        // if (dummy.proc == NULL) {
        //     // TracePrintf(1, "dummy proc is null\n");
        // }
        if (ready_head->proc != NULL) {
            head_proc_pid = ready_head->proc->pid;
        }
    }
    if (proc == NULL) {
    	// TracePrintf(1, "proc is null\n");
    	ready_tail = &dummy;
    	return idle;
    }

    TracePrintf(1, "READY QPOP - popped pid %d\t new head pid: %d\n", proc->pid, head_proc_pid);
    return proc;
}

void
ready_qpush(struct pcb *proc) {
	TracePrintf(1, "READY QPUSH pid: %d\n", proc -> pid);
    struct queue_elem *new_queue_elem = malloc(sizeof (struct queue_elem*));
    new_queue_elem -> proc = malloc(sizeof(struct pcb));
    new_queue_elem -> proc = malloc(sizeof(struct queue_elem));

    new_queue_elem -> proc = proc;

    if (ready_head -> proc == NULL) {
    	ready_head = new_queue_elem;
        ready_head->next = ready_tail;
    } else {
    	ready_tail -> next = new_queue_elem;
    }

    ready_tail = new_queue_elem;
    ready_tail->next = &dummy;

    int next_proc_pid = -1;

}

struct pcb*
delay_qpop() {
    struct pcb *proc = delay_head->proc;
    int head_proc_pid = -1;
    if (proc != NULL) {

        delay_head = delay_head->next;

        if (delay_head->proc != NULL) {
            head_proc_pid = delay_head->proc->pid;
        }
    }
    if (proc == NULL) {
        // TracePrintf(1, "proc is null\n");
        delay_tail = &dummy;
        return idle;
    }

    TracePrintf(1, "DELAY QPOP - popped pid %d\t new head pid: %d\n", proc->pid, head_proc_pid);
    return proc;
}

void
delay_qpush(struct pcb *proc) {
    TracePrintf(1, "DELAY QPUSH pid: %d\n", proc -> pid);
    struct queue_elem *new_queue_elem = malloc(sizeof (struct queue_elem*));
    new_queue_elem -> proc = malloc(sizeof(struct pcb));
    new_queue_elem -> proc = malloc(sizeof(struct queue_elem));

    new_queue_elem -> proc = proc;

    if (delay_head -> proc == NULL) {
        delay_head = new_queue_elem;
        delay_head->next = delay_tail;
    } else {
        delay_tail -> next = new_queue_elem;
    }

    delay_tail = new_queue_elem;
    delay_tail->next = &dummy;

    TracePrintf(1, "delay_head pid: %d end_of_delay: %d\n", delay_head->proc->pid, delay_head->proc->end_of_delay);
    int next_proc_pid = -1;

}



void
KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
	TracePrintf(1, "address of dummy %x, address of head %x\n", &dummy, ready_head);
    // KEEP TRACK OF THE KERNEL BRK and VIRTUAL MEMORY FLAG
    kernel_brk = orig_brk;
    virtual_memory = 0;

    // CREATE INTERRUPT VECTOR
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
    // USED PAGES -> VEM_BASE_1 down to page, and orig_brk up to page

    // CREATE THE HEAD OF THE FREE LIST
    free_pfn_head = malloc(sizeof (struct pfn_list_entry*));
    free_pfn_head->pfn = 0;
    // free_pfn_head.next = NULL;

    //INITIALIZE Region 1 PAGE TABLE
    r0_page_table = &initial_r0_page_table;
    TracePrintf(1, "r0_page_table addr: %x\n", r0_page_table);
    TracePrintf(1, "initial_r0_page_table addr: %x\n", &initial_r0_page_table);
    TracePrintf(1, "r1_page_table addr: %x\n", r1_page_table);
    TracePrintf(1, "pmemsize: %d\n", pmem_size);
    TracePrintf(1, "PAGE_TABLE_LEN: %d\n", PAGE_TABLE_LEN);

    // CREATE THE FREE LIST AND INITIALIZE REGION 1 PAGE TABLE
    for (_i = MEM_INVALID_PAGES; _i < pmem_size / PAGESIZE; _i++) {
        if (_i < used_pages_min || _i > used_pages_max) {
            // num_free_pfn ++;
            // list_entry.pfn = _i;
            // list_entry.next = &free_pfn_head;
            // free_pfn_head = list_entry;
            pfnpush(_i);
        } else {
            // TracePrintf(1, "PTE %d\n", _i);
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

    for (_i = used_pages_max; _i < UP_TO_PAGE(kernel_brk) / PAGESIZE; _i++) {
        TracePrintf(1, "Kernel Heap VPN: %d\n", _i);
        entry.pfn = _i;
        entry.valid = 1;
        entry.uprot = PROT_NONE;
        entry.kprot = PROT_READ|PROT_WRITE;
        r1_page_table[_i - used_pages_min] = entry;
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
        initial_r0_page_table[_i] = entry;
    }

    // I MADE THESE CHANGES 3/20/19 -- Lucy


    WriteRegister(REG_PTR0, (RCS421RegVal) r0_page_table);
    WriteRegister(REG_PTR1, (RCS421RegVal) r1_page_table);
    TracePrintf(1, "Enabling Virtual Memory\n");
    WriteRegister(REG_VM_ENABLE, (RCS421RegVal) 1);
    virtual_memory = 1;

    // Add back MEM_INVALID_PAGES
    for (_i=0; _i<MEM_INVALID_PAGES; _i++) {
        pfnpush(_i);
    }


    // head -> proc = malloc(sizeof(struct pcb));
    // head -> next = malloc(sizeof(struct queue_elem));
    // IDLE PROCESS
    idle = malloc(sizeof (struct pcb));
    idle -> pid = pid_counter;
    idle -> info = malloc(sizeof(ExceptionInfo));
    memcpy(idle->info, info, sizeof(ExceptionInfo));
    pid_counter++;
    idle-> r0_pointer = r0_page_table;
    idle->queue = 0;
    idle->ctx = malloc(sizeof(SavedContext));
    idle -> status_pointer = malloc(sizeof(struct queue_status));


    init = malloc(sizeof (struct pcb));
    init -> pid = pid_counter;
    init -> init = 0;
    init->queue = 1;
    init -> info = malloc(sizeof(ExceptionInfo));
    memcpy(init->info, info, sizeof(ExceptionInfo));
    pid_counter++;
    init->ctx = malloc(sizeof(SavedContext));
    init -> status_pointer = malloc(sizeof(struct queue_status));
    init -> r0_pointer = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
    memcpy(init->r0_pointer, &initial_r0_page_table, PAGE_TABLE_LEN * sizeof(struct pte));
    // TracePrintf(1, "malloc\n");

    // TracePrintf(1, "&init->ctx:%x\tinit->r0_pointer:%x\n", &init->ctx, init->r0_pointer);
    // init->kstack_pfns = malloc(KERNEL_STACK_PAGES * sizeof(unsigned int));
    // memcpy(init->r0_pointer, r0_page_table, PAGE_TABLE_LEN * sizeof(struct pte)); - removed 3/25

    running_proc = idle;


    ContextSwitch(MySwitchFunc, idle->ctx, idle, idle);
    TracePrintf(1, "Switched idle-idle\n");
    LoadProgram("idle", cmd_args, info);
    TracePrintf(1, "Loaded idle\n");
    // ContextSwitch(MyCFunc, init->ctx, init, init);
    // TracePrintf(1, "cloned init\n");
    ContextSwitch(MySwitchFunc, init->ctx, idle, init);
    TracePrintf(1,"Switched Context idle to init - running_proc pid: %d\n", running_proc->pid);
    if (running_proc->pid == 1) {
        LoadProgram(cmd_args[0], cmd_args, info);
    }
    TracePrintf(1, "End of Kernel Start\n");
    return;
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
LoadProgram(char *name, char **args, ExceptionInfo *info)
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
    cp = argbuf = (char *) malloc(size);
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
    cpp = (char **)((unsigned long)cp & (-1 << 4)); /* align cpp */
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
    info->sp = (char *)cpp; // I Just copied what's in the above comment


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
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        struct pte entry = r0_page_table[i];
        // Check to see if the memory should be freed and free the memory, setting the valid bit to 0
        // TracePrintf(1, "PTE num: %d\n", i);
        // if (entry.valid && (entry.pfn * PAGESIZE < KERNEL_STACK_BASE)  || (entry.pfn * PAGESIZE) > KERNEL_STACK_LIMIT)) {
        if (entry.valid && (i * PAGESIZE < KERNEL_STACK_BASE || i * PAGESIZE > KERNEL_STACK_LIMIT)) {
            entry.valid = 0;
            // TracePrintf(1, "PUSHING back pfn: %d\n", entry.pfn);
            pfnpush(entry.pfn);
            // struct pfn_list_entry new_pfn_entry;
            // new_pfn_entry.pfn = entry.pfn;
            // new_pfn_entry.next = &free_pfn_head;
            // free_pfn_head = new_pfn_entry;
            // num_free_pfn ++;
        }
    }

    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    // >>>> Leave the first MEM_INVALID_PAGES number of PTEs in the
    // >>>> Region 0 page table unused (and thus invalid)
    int vpn = 0;
    for (i=0; i<MEM_INVALID_PAGES;i++) {
        r0_page_table[vpn].valid = 0;
        vpn ++;
    }

    /* First, the text pages */
    // >>>> For the next text_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_EXEC
    // >>>>     pfn   = a new page of physical memory


    // struct pfn_list_entry *curr = &free_pfn_head;
    // while(curr != NULL) {
    //     TracePrintf(1, "-- %d", curr->pfn);
    //     curr = curr->next;
    // }
    TracePrintf(1, "Allocating text PTEs\n");
    for (i=0; i<text_npg; i++) {
        r0_page_table[vpn].valid = 1;
        r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
        r0_page_table[vpn].uprot = PROT_READ | PROT_EXEC;
        r0_page_table[vpn].pfn = pfnpop();
        TracePrintf(1, "VPN: %d\t PFN: %d\t addr: %x\n", vpn, r0_page_table[vpn].pfn, r0_page_table[vpn].pfn * PAGESIZE);
        vpn ++;
        // free_pfn_head.pfn;
        // free_pfn_head = free_pfn_head.next;
        // num_free_pfn --;
    }


    // /* Then the data and bss pages */
    // >>>> For the next data_bss_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    for (i=0; i<data_bss_npg; i++) {
        r0_page_table[vpn].valid = 1;
        r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
        r0_page_table[vpn].uprot = PROT_READ | PROT_WRITE;
        r0_page_table[vpn].pfn = pfnpop();
        vpn ++;
    }
    running_proc->brk = vpn * PAGESIZE;
    TracePrintf(1, "BRK set to: %x\n", running_proc->brk);

    /* And finally the user stack pages */
    // >>>> For stack_npg number of PTEs in the Region 0 page table
    // >>>> corresponding to the user stack (the last page of the
    // >>>> user stack *ends* at virtual address USER_STACK_LIMIT),
    // >>>> initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    vpn = DOWN_TO_PAGE(USER_STACK_LIMIT) / PAGESIZE - 1;
    for (i=0; i<stack_npg; i++) {
        TracePrintf(1, "User Stack vpn: %d\t%x  -  %x\n", vpn, vpn*PAGESIZE, (vpn+1)*PAGESIZE);
        r0_page_table[vpn].valid = 1;
        r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
        r0_page_table[vpn].uprot = PROT_READ | PROT_WRITE;
        r0_page_table[vpn].pfn = pfnpop();
        vpn --;
    }

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size) != li.text_size+li.data_size) {
        TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
        free(argbuf);
        close(fd);
        // >>>> Since we are returning -2 here, this should mean to
        // >>>> the rest of the kernel that the current process should
        // >>>> be terminated with an exit status of ERROR reported
        // >>>> to its parent process.
        //TODO: THIS
        return (-2);
    }
    close(fd);          /* we've read it all now */


    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */
    // >>>> For text_npg number of PTEs corresponding to the user text
    // >>>> pages, set each PTE's kprot to PROT_READ | PROT_EXEC.
    for (vpn = MEM_INVALID_PAGES; vpn < MEM_INVALID_PAGES + text_npg; vpn++) {
        r0_page_table[vpn].kprot = PROT_READ | PROT_EXEC;
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
    '\0', li.bss_size);

    /*
     *  Set the entry point in the exception frame.
     */
    // >>>> Initialize pc for the current process to (void *)li.entry
    info->pc = (void *)li.entry; // USING THE PASSED IN EXCEPTION INFO

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;      /* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
    *cpp++ = cp;
    strcpy(cp, cp2);
    cp += strlen(cp) + 1;
    cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;  /* the last argv is a NULL pointer */
    *cpp++ = NULL;  /* a NULL pointer for an empty envp */
    *cpp++ = 0;     /* and terminate the auxiliary vector */

    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     */
    // >>>> Initialize regs[0] through regs[NUM_REGS-1] for the
    // >>>> current process to 0.
    // >>>> Initialize psr for the current process to 0.
    for(i=0; i<NUM_REGS; i++) {
        // TracePrintf(1, "i: %d", i);
        info->regs[i] = 0;
    }
    info->psr = 0;
    TracePrintf(1, "Finished loading program\n");
    return (0);
}

int
SetKernelBrk(void *addr) {
    if (virtual_memory) {
        int num_pages_needed = (UP_TO_PAGE(addr) - UP_TO_PAGE(kernel_brk)) / PAGESIZE;
        TracePrintf(1, "Current brk: %x\tnext brk: %x\tnum pages needed: %d\n", kernel_brk, addr, num_pages_needed);
        for (_i = num_pages_needed; _i > 0; _i--) {
            if (num_free_pfn <= 0) {
                return -1;
            }
            unsigned int pfn = pfnpop();
            int vpn = ((unsigned long)kernel_brk - VMEM_1_BASE) / PAGESIZE;
            TracePrintf(1, "Mallocing VPN: %d\t to PFN: %d\n", vpn, pfn);
            r1_page_table[vpn].valid = 1;
            r1_page_table[vpn].pfn = pfn;
            r1_page_table[vpn].uprot = PROT_NONE;
            r1_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
            kernel_brk = VMEM_1_BASE + ((vpn + 1) * PAGESIZE);
            num_free_pfn --;
        }
    }
    kernel_brk = addr;
    TracePrintf(1, "New brk: %x\n", kernel_brk);
    return (0);
}

SavedContext*
MySwitchFunc(SavedContext *ctxp, void *p1, void *p2) {

	TracePrintf(1, "HERE1%p\n", p2);
    struct pcb *pcb1 = (struct pcb*)p1;
    struct pcb *pcb2 = (struct pcb*)p2;

    TracePrintf(1, "CONTEXT SWITCH pid  %d to %d\n", pcb1->pid, pcb2->pid);
    TracePrintf(1, "ctxp: %x\n", ctxp);


    // Save current r0 page table to PCB1
    // memcpy(pcb1->r0_pointer, &r0_page_table, PAGE_TABLE_LEN * sizeof(struct pte));
    if (delay_head->proc != NULL)
        TracePrintf(1, "1 - delay_head pid1: %d end_of_delay: %d\n", delay_head->proc->pid, delay_head->proc->end_of_delay);
    if (pcb2->init == 0) {
        pcb2->init = 1;
        copyKernelStack(pcb2);
    }

    r0_page_table = pcb2->r0_pointer;
    int physaddr = r1_page_table[DOWN_TO_PAGE(r0_page_table) / PAGESIZE - PAGE_TABLE_LEN].pfn * PAGESIZE;
    physaddr += (int)r0_page_table & PAGEOFFSET;
    r0_page_table = pcb2->r0_pointer;
    int physaddr = r1_page_table[DOWN_TO_PAGE(r0_page_table) / PAGESIZE - PAGE_TABLE_LEN].pfn * PAGESIZE;
    physaddr += (int)r0_page_table & PAGEOFFSET;
    WriteRegister(REG_PTR0, (RCS421RegVal) physaddr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    if (pcb1 -> pid != 0 && pcb1->queue) {
    	ready_qpush(pcb1);
    }
    running_proc = pcb2;
    return pcb2->ctx;
}

void
copyKernelStack(struct pcb *proc) {
    TracePrintf(1, "Copying Kernel stack\n");
    int vpn = MEM_INVALID_PAGES;
    while (r0_page_table[vpn].valid) {
        vpn ++;
    }
    TracePrintf(1, "tempvpn: %d\n", vpn);
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) vpn);
    int temp_pfn = r0_page_table[vpn].pfn;
    TracePrintf(1, "temppfn: %d\n", temp_pfn);
    r0_page_table[vpn].valid = 1;
    r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
    for (_i = 0; _i < KERNEL_STACK_PAGES; _i++) {
        unsigned int pfn = pfnpop();
        r0_page_table[vpn].pfn = pfn;
        proc->r0_pointer[PAGE_TABLE_LEN - KERNEL_STACK_PAGES + _i].pfn = pfn;
        TracePrintf(1, "Copying to vpn: %d\t addr: %x\t from vpn: %d\t w/ pfn:%d\n", vpn, vpn * PAGESIZE, KERNEL_STACK_BASE / PAGESIZE + _i, pfn);
        memcpy(vpn * PAGESIZE, KERNEL_STACK_BASE + (_i * PAGESIZE), PAGESIZE);
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) (vpn * PAGESIZE));
    }
    // TracePrintf(1, "r0_pointer[508].pfn: %d\n", proc->r0_pointer[508].pfn);
    r0_page_table[vpn].valid = 0;
    r0_page_table[vpn].pfn = temp_pfn;
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) (vpn * PAGESIZE));
}

void
copyRegion0(struct pcb *proc) {
    TracePrintf(1, "Copying Region0\n");
    int vpn = MEM_INVALID_PAGES;
    while (r0_page_table[vpn].valid) {
        vpn ++;
    }
    TracePrintf(1, "tempvpn: %d\n", vpn);
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) vpn);
    int temp_pfn = r0_page_table[vpn].pfn;
    TracePrintf(1, "temppfn: %d\n", temp_pfn);
    r0_page_table[vpn].valid = 1;
    r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
    for (_i = MEM_INVALID_PAGES; _i < PAGE_TABLE_LEN; _i++) {
        if (r0_page_table[_i].valid && _i != vpn) {
            unsigned int pfn = pfnpop();
            r0_page_table[vpn].pfn = pfn;
            memcpy(&proc->r0_pointer[_i], &r0_page_table[_i], sizeof(struct pte));
            proc->r0_pointer[_i].pfn = pfn;
            TracePrintf(1, "Copying vpn:%d\t with vpn:%d\t and addr:%x\t from vpn:%d\t to pfn:%d\n", _i, vpn, vpn * PAGESIZE, _i, pfn);
            memcpy(vpn * PAGESIZE, _i * PAGESIZE, PAGESIZE);
            WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) (vpn * PAGESIZE));
        }
    }
    // TracePrintf(1, "r0_pointer[508].pfn: %d\n", proc->r0_pointer[508].pfn);
    r0_page_table[vpn].valid = 0;
    r0_page_table[vpn].pfn = temp_pfn;
    // TracePrintf(1, "r0_page_table[508].valid: %d\n", r0_page_table[508].valid);
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) (vpn * PAGESIZE));
}

SavedContext*
MyCloneFunc(SavedContext *ctxp, void *p1, void *p2) {
    TracePrintf(1, "Cloning\n");
    struct pcb *pcb1 = (struct pcb*)p1;
    struct pcb *pcb2 = (struct pcb*)p2;
    TracePrintf(1, "ctxp: %x\n", ctxp);

    pcb2->r0_pointer = malloc(PAGE_TABLE_LEN * sizeof(struct pte));

    // memcpy(pcb2->r0_pointer, pcb1->r0_pointer, PAGE_TABLE_LEN * sizeof(struct pte));

    copyRegion0(pcb2);
    TracePrintf(1, "Finished copying kernel stack\n");

    pcb2->info = malloc(sizeof (ExceptionInfo));
    TracePrintf(1, "Malloced info\n");
    memcpy(pcb2->info, pcb1->info, sizeof(ExceptionInfo));

    TracePrintf(1, "Returning from MyCloneFunc\n");
    TracePrintf(1, "pcb2->ctx: %x\n", ctxp);

    return pcb2->ctx;
}

int
_Fork() {
    TracePrintf(1, "Forking\n");
    struct pcb *child_proc = malloc(sizeof (struct pcb));
    child_proc->ctx = malloc(sizeof (SavedContext));
    TracePrintf(1, "child proc ctx addr: %x\t end_of_delay addr: %x\t size: %x\n", child_proc->ctx, &child_proc->end_of_delay, sizeof(SavedContext));
    int new_pid = pid_counter;
    pid_counter ++;
    child_proc->queue = 0;
    child_proc->pid = new_pid;
    child_proc->init = 1;
    child_proc->brk = running_proc->brk;
    child_proc -> parent = running_proc;
    // ContextSwitch(&running_proc->ctx, running_proc, running_proc);
    ContextSwitch(MyCloneFunc, child_proc->ctx, running_proc, child_proc);
    TracePrintf(1, "between switches\n");
    ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, child_proc);

    TracePrintf(1, "CURRENT READY QUEUE: %d\n", ready_head -> proc -> pid);
    if (running_proc->pid == new_pid){
        TracePrintf(1, "FORK RETURN CHILD\n");
        return 0;
    } else {
        TracePrintf(1, "FORK RETURN PARENT\n");
        TracePrintf(1, "delay_head pid: %d end_of_delay: %d\n", delay_head->proc->pid, delay_head->proc->end_of_delay);
        return new_pid;
    }
}

int
_Exec(char *filename, char **argvec, ExceptionInfo *info) {
    TracePrintf(1, "EXEC\n");
    // running_proc->init=1;
    // ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, running_proc);
    int val = LoadProgram(filename, argvec, info);
    if (val < 0) {
    	return ERROR;
    }

    return 0;

}

void
_Exit(int status) {
    TracePrintf(1, "EXIT\n");
    struct pcb *next_proc = ready_qpop();
    if (next_proc->pid == 0) {

        if (delay_ticks > 0) {
            ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, idle);
        }
        TracePrintf(1, "No more waiting procedures, halting");
        Halt();
    }
    else {
    	TracePrintf(1, "HERE7\n");
        TracePrintf(1, "Switching to next process with pid: %d and %d\n", running_proc -> pid, next_proc->pid);
        ContextSwitch(MySwitchFunc, &running_proc->ctx, running_proc, next_proc);

    }



    //TODO, CLEAR MEMORY
    //returning status to parent
    if (running_proc -> parent -> exited != 1) {

    	struct queue_status *update_status_elem = running_proc -> parent -> status_pointer;

	    while (update_status_elem -> next != NULL) {
	    	update_status_elem = update_status_elem -> next;
	    }
	    update_status_elem -> next = malloc(sizeof(struct queue_status));
	    update_status_elem -> next -> proc = running_proc;
	    update_status_elem -> next -> status = status;
    }

    // //"orphaning" children in ready queue
    // struct queue_elem *next_queue_elem;
    // next_queue_elem = ready_head;

    // while (next_queue_elem != NULL) {
    // 	if (next_queue_elem -> proc -> parent == running_proc) {
    // 		next_queue_elem -> proc -> parent = NULL;
    // 		next_queue_elem = next_queue_elem -> next;
    // 	}
    // }

    // //"orphaning" children in delay queue
    // next_queue_elem = delay_head;
    // while (next_queue_elem != NULL) {
    // 	if (next_queue_elem -> proc -> parent == running_proc) {
    // 		next_queue_elem -> proc -> parent = NULL;
    // 		next_queue_elem = next_queue_elem -> next;
    // 	}
    // }
    // //"orphaning" children in waiting queue
    // next_queue_elem = running_proc -> waiting_head;
    // while (next_queue_elem != NULL) {
    // 	if (next_queue_elem -> proc -> parent == running_proc) {
    // 		next_queue_elem -> proc -> parent = NULL;
    // 		next_queue_elem = next_queue_elem -> next;
    // 	}
    // }

    //TODO: context switch into next element in waiting queue?
    ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, ready_qpop());


}

int
_Wait(int *status_ptr) {

	if (running_proc -> status_pointer == NULL) {
		ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, ready_qpop());
	}

	int pid_child = running_proc -> status_pointer -> proc -> pid;

    *(status_ptr)= running_proc -> status_pointer -> status;
    running_proc -> status_pointer = running_proc -> status_pointer -> next;

    return pid_child;
}

int
_GetPid() {
    TracePrintf(1, "running_proc -> pid: %\n", running_proc -> pid);
    return running_proc -> pid;
}

int
_Brk(void *addr) {
    TracePrintf(1, "BRK\n");
    // if (brk > running_proc->brk) {
    int num_pages = (UP_TO_PAGE(addr) - UP_TO_PAGE(running_proc->brk)) / PAGESIZE;
    if (num_pages > 0) {
        TracePrintf(1, "Allocating for proc pid: %d\n", running_proc->pid);
        // TracePrintf(1, "Running proc pid: %d\n", running_proc -> pid);

        TracePrintf(1, "Current brk: %x\tnext brk: %x\tnum pages needed: %d\n", running_proc->brk, addr, num_pages);

        for (_i = num_pages; _i > 0; _i--) {
            if (num_free_pfn <= 0) {
                return ERROR;
            }
            unsigned int pfn = pfnpop();
            int vpn = ((unsigned long)running_proc->brk) / PAGESIZE;
            TracePrintf(1, "Mallocing VPN: %d\t to PFN: %d\n", vpn, pfn);

            if (r0_page_table[vpn].valid) {
                // Ran into stack
                TracePrintf(1, "Ran into stack in Brk()\n");
                return ERROR;
            }

            r0_page_table[vpn].valid = 1;
            r0_page_table[vpn].pfn = pfn;
            r0_page_table[vpn].uprot = PROT_READ | PROT_WRITE;
            r0_page_table[vpn].kprot = PROT_READ | PROT_WRITE;
            running_proc->brk = VMEM_0_BASE + ((vpn + 1) * PAGESIZE);
            num_free_pfn --;
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
        }
    } else {
        TracePrintf(1, "Deallocating\n");
        int num_pages = (UP_TO_PAGE(running_proc->brk) - UP_TO_PAGE(addr)) / PAGESIZE;
        TracePrintf(1, "Current brk: %x\tnext brk: %x\tnum pages to free: %d\n", running_proc->brk, addr, num_pages);
        for (_i = num_pages; _i > 0; _i--) {
            int vpn = ((unsigned long)UP_TO_PAGE(running_proc->brk)) / PAGESIZE - 1;
            unsigned int pfn = r0_page_table[vpn].pfn;
            TracePrintf(1, "Freeing VPN: %d\t with PFN: %d\n", vpn, pfn);
            pfnpush(pfn);
            r0_page_table[vpn].valid = 0;
            running_proc->brk = vpn * PAGESIZE;
            num_free_pfn ++;
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
        }
    }
    TracePrintf(1, "New brk: %x\n", running_proc->brk);
    // }
    return 0;
}

int
_Delay(int clock_ticks) {
    if (clock_ticks == 0) {
    	return 0;
    }
    if (clock_ticks < 0) {
    	return ERROR;
    }
    TracePrintf(1, "DELAY\n");

    running_proc->end_of_delay = time + clock_ticks;
    TracePrintf(1, "time: %d, clock_ticks: %d\n", time, clock_ticks);
    TracePrintf(1, "Pushing proc pid: %d to delay queue with end_of_delay: %d\n", running_proc->pid, running_proc->end_of_delay);
    TracePrintf(1, "running_proc->end_of_delay addr: %x\n", &running_proc->end_of_delay);
    delay_qpush(running_proc);
    num_delay_procs ++;
    running_proc->queue = 0;
    ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, ready_qpop());
    return 0;
}

int
_TtyRead(int tty_id, void *buf, int len) {
    return -1;
}

int
_TtyWrite(int tty_id, void *buf, int len) {
    return -1;
}

void trap_kernel_handler(ExceptionInfo *info) {
    TracePrintf(1, "Exception: Kernel\n");
    int code_num = info -> code;
    if (code_num == YALNIX_FORK) {
        info -> regs[0] = _Fork();
    }

    if (code_num == YALNIX_EXEC) {
        info -> regs[0] = _Exec((char*) info -> regs[1], (char**) info -> regs[2], info);
    }

    if (code_num == YALNIX_EXIT) {
        _Exit((int) info -> regs[1]);
    }

    if (code_num == YALNIX_WAIT) {
        info -> regs[0] = _Wait((int*) info -> regs[1]);
    }

    if (code_num == YALNIX_GETPID) {
        info -> regs[0] = _GetPid();
    }

    if (code_num == YALNIX_BRK) {
        info -> regs[0] = _Brk((void*) info -> regs[1]);
    }

    if (code_num == YALNIX_DELAY) {
        info -> regs[0] = _Delay((int)info -> regs[1]);
    }

    if (code_num == YALNIX_TTY_READ) {
        info -> regs[0] = _TtyWrite((int) info -> regs[1], (void*) info -> regs[2], (int) info -> regs[3]);
    }

    if (code_num == YALNIX_TTY_WRITE) {
        info -> regs[0] = _TtyWrite((int) info -> regs[1], (void*) info -> regs[2], (int) info -> regs[3]);
    }
}

void trap_clock_handler(ExceptionInfo *info) {
    time ++;

    TracePrintf(1, "CLOCK time: %d\n", time);

    // LOOP THROUG DELAY QUEUE
    int procs_done_delaying = 0;

    for (_i=0; _i<num_delay_procs; _i++) {
        struct pcb *proc = delay_qpop();
        TracePrintf(1, "Proc pid: %d popped from delay queue at time: %d\tend of delay: %d\n", proc->pid, time, proc->end_of_delay);
        TracePrintf(1, "proc->end_of_delay addr: %x\n", &proc->end_of_delay);
        if (time >= proc->end_of_delay) {
            TracePrintf(1, "Proc pid: %d done delaying\n", proc->pid);
            ready_qpush(proc);
            procs_done_delaying ++;
        } else {
            delay_qpush(proc);
        }
    }


    if (time % 2 == 0) {
        ContextSwitch(MySwitchFunc, running_proc->ctx, running_proc, ready_qpop());
    }

    num_delay_procs -= procs_done_delaying;
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
        printf("%s%p\n", "No mapping at addr: ", info->addr);
        TracePrintf(1, "addr: %x,\tpc: %x\tsp: %x\t\n", info->addr, info->pc, info->sp);
        // TracePrintf(1, "addr: ")
        Halt();
        return;
    }

    if (info -> code == TRAP_MEMORY_ACCERR) {
        printf("%s\n", "Protection violation at addr");
        return;
    }

    if (info -> code == TRAP_MEMORY_KERNEL) {
        printf("Linux kernel sent SIGSEGV at addr: %p\n", info->addr);
        Halt();
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