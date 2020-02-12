#include <signal.h>

char *strsignal(int sig) {
    if (sig < 0 || sig > _NSIG) {
        sig = 0;
    }

    return (char *) sys_siglist[sig];
}

const char *const sys_siglist[_NSIG] = { "Invalid signal number",
                                         "TTY hang up",
                                         "Interrupted",
                                         "Quit",
                                         "Bus",
                                         "Trap",
                                         "Aborted",
                                         "Continued",
                                         "Floating point exeception",
                                         "Killed",
                                         "Read from tty",
                                         "Write to tty",
                                         "Illegal instruction",
                                         "Pipe error",
                                         "Alarm",
                                         "Terminated",
                                         "Segmentation fault",
                                         "Stopped",
                                         "Stopped by tty",
                                         "User 1",
                                         "User 2",
                                         "Poll",
                                         "Profile",
                                         "Invalid system call",
                                         "Urge",
                                         "Virtual alarm",
                                         "CPU exceeded time limit",
                                         "File size limit exceeded",
                                         "Child state change",
                                         "Window size change",
                                         "__PTHREAD_TIMER_SIGNAL",
                                         "__PTHREAD_CANCEL_SIGNAL",
                                         "SIGRTMIN",
                                         "SIGRTMIN+1",
                                         "SIGRTMIN+2",
                                         "SIGRTMIN+3",
                                         "SIGRTMIN+4",
                                         "SIGRTMIN+5",
                                         "SIGRTMIN+6",
                                         "SIGRTMIN+7",
                                         "SIGRTMIN+8",
                                         "SIGRTMIN+9",
                                         "SIGRTMIN+10",
                                         "SIGRTMIN+11",
                                         "SIGRTMIN+12",
                                         "SIGRTMIN+13",
                                         "SIGRTMIN+14",
                                         "SIGRTMIN+15",
                                         "SIGRTMIN+16",
                                         "SIGRTMIN+17",
                                         "SIGRTMIN+18",
                                         "SIGRTMIN+19",
                                         "SIGRTMIN+20",
                                         "SIGRTMIN+21",
                                         "SIGRTMIN+22",
                                         "SIGRTMIN+23",
                                         "SIGRTMIN+24",
                                         "SIGRTMIN+25",
                                         "SIGRTMIN+26",
                                         "SIGRTMIN+27",
                                         "SIGRTMIN+28",
                                         "SIGRTMIN+29",
                                         "SIGRTMIN+30",
                                         "SIGRTMAX" };