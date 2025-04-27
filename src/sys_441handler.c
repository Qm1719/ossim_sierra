#include "common.h"
#include "syscall.h"
#include <stdio.h>

int __sys_441handler(struct pcb_t *caller, struct sc_regs *regs) {
    printf("Hello from sys_hellohandler! Parameter: %d\n", regs->a1);
    return 0;
}