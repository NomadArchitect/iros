#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

#include <kernel/fs/vfs.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/irqs/handlers.h>
#include <kernel/sched/process_sched.h>
#include <kernel/hal/output.h>
#include <kernel/proc/process_state.h>

// #define PROC_SIGNAL_DEBUG

static struct process *current_process;
static struct process initial_kernel_process;

/* Copying args and envp is necessary because they could be saved on the program stack we are about to overwrite */
uintptr_t map_program_args(uintptr_t start, char **argv, char **envp) {
    size_t argc = 0;
    size_t args_str_length = 0;
    while (argv[argc++] != NULL) {
        args_str_length += strlen(argv[argc - 1]) + 1;
    }

    size_t envc = 0;
    size_t env_str_length = 0;
    while (envp[envc++] != NULL) {
        env_str_length += strlen(envp[envc - 1]) + 1;
    }

    char **args_copy = calloc(argc, sizeof(char**));
    char **envp_copy = calloc(envc, sizeof(char**));

    char *args_buffer = malloc(args_str_length);
    char *env_buffer = malloc(env_str_length);

    ssize_t j = 0;
    ssize_t i = 0;
    while (argv[i] != NULL) {
        ssize_t last = j;
        while (argv[i][j - last] != '\0') {
            args_buffer[j] = argv[i][j - last];
            j++;
        }
        args_buffer[j++] = '\0';
        args_copy[i++] = args_buffer + last;
    }
    args_copy[i] = NULL;

    j = 0;
    i = 0;
    while (envp[i] != NULL) {
        ssize_t last = j;
        while (envp[i][j - last] != '\0') {
            env_buffer[j] = envp[i][j - last];
            j++;
        }
        env_buffer[j++] = '\0';
        envp_copy[i++] = env_buffer + last;
    }
    envp_copy[i] = NULL;

    char **argv_start = (char**) (start - sizeof(char**));

    size_t count = argc + envc;
    char *args_start = (char*) (argv_start - count);

    for (i = 0; args_copy[i] != NULL; i++) {
        args_start -= strlen(args_copy[i]) + 1;
        strcpy(args_start, args_copy[i]);
        argv_start[i - argc] = args_start;
    }

    argv_start[0] = NULL;

    for (i = 0; envp_copy[i] != NULL; i++) {
        args_start -= strlen(envp_copy[i]) + 1;
        strcpy(args_start, envp_copy[i]);
        argv_start[i - count] = args_start;
    }

    argv_start[-(argc + 1)] = NULL;

    args_start = (char*) ((((uintptr_t) args_start) & ~0x7) - 0x08);

    args_start -= sizeof(size_t);
    *((size_t*) args_start) = argc - 1;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - argc;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - count;

    free(args_copy);
    free(envp_copy);
    free(args_buffer);
    free(env_buffer);

    return (uintptr_t) args_start;
}

void init_kernel_process() {
    current_process = &initial_kernel_process;

    arch_init_kernel_process(current_process);

    initial_kernel_process.kernel_process = true;
    initial_kernel_process.pid = 1;
    initial_kernel_process.sched_state = RUNNING;
    initial_kernel_process.next = NULL;
    initial_kernel_process.pgid = 1;
    initial_kernel_process.ppid = 1;
    initial_kernel_process.tty = -1;

    sched_add_process(&initial_kernel_process);
}

struct process *load_kernel_process(uintptr_t entry) {
    struct process *process = calloc(1, sizeof(struct process));
    process->pid = get_next_pid();
    process->pgid = process->pid;
    process->ppid = initial_kernel_process.pid;
    process->process_memory = NULL;
    process->kernel_process = true;
    process->sched_state = READY;
    process->cwd = malloc(2);
    process->tty = -1;
    strcpy(process->cwd, "/");
    process->next = NULL;

    arch_load_kernel_process(process, entry);

    debug_log("Loaded Kernel Process: [ %d ]\n", process->pid);
    return process;
}

struct process *load_process(const char *file_name) {
    int error = 0;
    struct file *program = fs_open(file_name, O_RDONLY, &error);
    assert(program != NULL && error == 0);

    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    fs_close(program);

    assert(elf64_is_valid(buffer));

    struct process *process = calloc(1, sizeof(struct process));
    process->pid = get_next_pid();
    process->pgid = process->pid;
    process->ppid = initial_kernel_process.pid;
    process->process_memory = NULL;
    process->kernel_process = false;
    process->sched_state = READY;
    process->cwd = malloc(2);
    process->tty = -1;
    strcpy(process->cwd, "/");
    process->next = NULL;

    uintptr_t old_paging_structure = get_current_paging_structure();
    uintptr_t structure = create_paging_structure(process->process_memory, false);
    load_paging_structure(structure);

    elf64_load_program(buffer, length, process);
    elf64_map_heap(buffer, process);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    process_stack->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    process_stack->type = VM_PROCESS_STACK;
    process_stack->start = find_first_kernel_vm_region()->start - 2 * PAGE_SIZE;
    process_stack->end = process_stack->start + PAGE_SIZE;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    map_vm_region(process_stack);

    arch_load_process(process, elf64_get_entry(buffer));
    free(buffer);

    load_paging_structure(old_paging_structure);

    process->files[0] = fs_open("/dev/serial", O_RDWR, NULL);
    process->files[1] = fs_open("/dev/serial", O_RDWR, NULL);
    process->files[2] = fs_open("/dev/serial", O_RDWR, NULL);

    debug_log("Loaded Process: [ %d, %s ]\n", process->pid, file_name);
    return process;
}

/* Must be called from unpremptable context */
void run_process(struct process *process) {
    if (current_process->sched_state == RUNNING) {
        current_process->sched_state = READY;
    }
    current_process = process;
    current_process->sched_state = RUNNING;

    arch_run_process(process);
}

struct process *get_current_process() {
    return current_process;
}

/* Must be called from unpremptable context */
void free_process(struct process *process, bool free_paging_structure) {
    struct process *current_save = current_process;
    current_process = process;
    arch_free_process(process, free_paging_structure);

    free(process->cwd);

    struct vm_region *region = process->process_memory;
    while (region != NULL) {
        if (region->type == VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES) {
            fs_munmap((void*) region->start, region->end);
        }

        region = region->next;
    }

    region = process->process_memory;
    while (region != NULL) {
        struct vm_region *temp = region->next;
        free(region);
        region = temp;
    }

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (process->files[i] != NULL) {
            fs_close(process->files[i]);
        }
    }

    free(process);
    current_process = current_save;
}

void proc_set_sig_pending(struct process *process, int signum) {
    process->sig_pending |= (1U << signum);
}

void proc_unset_sig_pending(struct process *process, int signum) {
    process->sig_pending &= ~(1U << signum);
}

int proc_get_next_sig(struct process *process) {
    if (!process->sig_pending) {
        return -1; // Indicates we did nothing
    }

    for (size_t i = 1; i < _NSIG; i++) {
        if ((process->sig_pending & (1U << i)) && !proc_is_sig_blocked(process, i)) {
            return i;
        }
    }

    return -1;
}

void proc_notify_parent(pid_t child_pid) {
    struct process *child = find_by_pid(child_pid);
    struct process *parent = find_by_pid(child->ppid);

    if (parent == NULL) {
        parent = &initial_kernel_process;
    }

    proc_set_sig_pending(parent, SIGCHLD);
}

enum sig_default_behavior {
    TERMINATE,
    TERMINATE_AND_DUMP,
    IGNORE,
    STOP,
    CONTINUE,
    INVAL
};

static enum sig_default_behavior sig_defaults[_NSIG] = {
    INVAL,              // INVAL
    TERMINATE,          // SIGHUP
    TERMINATE,          // SIGINT
    TERMINATE_AND_DUMP, // SIGQUIT
    TERMINATE_AND_DUMP, // SIGBUS
    TERMINATE_AND_DUMP, // SIGTRAP
    TERMINATE_AND_DUMP, // SIGABRT
    CONTINUE,           // SIGCONT
    TERMINATE_AND_DUMP, // SIGFPE
    TERMINATE,          // SIGKILL
    STOP,               // SIGTTIN
    STOP,               // SIGTTOU
    TERMINATE,          // SIGILL
    TERMINATE,          // SIGPIPE
    TERMINATE,          // SIGALRM
    TERMINATE,          // SIGTERM
    TERMINATE_AND_DUMP, // SIGSEGV
    STOP,               // SIGSTOP
    STOP,               // SIGTSTP
    TERMINATE,          // SIGUSR1
    TERMINATE,          // SIGUSR2
    TERMINATE,          // SIGPOLL
    TERMINATE,          // SIGPROF
    TERMINATE_AND_DUMP, // SIGSYS
    IGNORE,             // SIGURG
    TERMINATE,          // SIGVTALRM
    TERMINATE_AND_DUMP, // SIGXCPU
    TERMINATE_AND_DUMP, // SIGXFSZ
    IGNORE,             // SIGCHLD
    INVAL,              // INVAL
    INVAL,              // INVAL
    INVAL               // INVAL
};

void proc_do_sig(struct process *process, int signum) {
    assert(process->sig_pending & (1U << signum));
    assert(!proc_is_sig_blocked(process, signum));

#ifdef PROC_SIGNAL_DEBUG
    debug_log("Doing signal: [ %d, %d ]\n", process->pid, signum);
#endif /* PROC_SIGNAL_DEBUG */

    proc_unset_sig_pending(process, signum);

    if (process->sig_state[signum].sa_handler == SIG_IGN) {
        return;
    }

    if (process->sig_state[signum].sa_handler != SIG_DFL) {
        proc_do_sig_handler(process, signum);
    }

    assert(sig_defaults[signum] != INVAL);
    switch (sig_defaults[signum]) {
        case TERMINATE_AND_DUMP:
            debug_log("Should dump core: [ %d ]\n", process->pid);
            // Fall through
        case TERMINATE:
            if (process->sched_state == EXITING) { 
                break; 
            }
            process->sched_state = EXITING;
            invalidate_last_saved(process);
            proc_add_message(process->pid, proc_create_message(STATE_INTERRUPTED, signum));
            break;
        case STOP:
            if (process->sched_state == WAITING) { 
                break; 
            }
            process->sched_state = WAITING;
            proc_add_message(process->pid, proc_create_message(STATE_STOPPED, signum));
            break;
        case CONTINUE:
            if (process->sched_state == READY) { 
                break; 
            }
            process->sched_state = READY;
            proc_add_message(process->pid, proc_create_message(STATE_CONTINUED, signum));
            break;
        case IGNORE:
            break; 
        default:
            assert(false);
            break;
    }
}

bool proc_is_sig_blocked(struct process *process, int signum) {
    assert(signum >= 1 && signum < _NSIG);
    return process->sig_mask & (1U << (signum - 1));
}