#include "common.h"
#include "syscall.h"
#include <stdio.h>

int main() {
    struct pcb_t dummy_proc; // Dummy process, fill as needed for your OS
    struct sc_regs regs = {0};
    regs.a1 = 123; // Test parameter

    // Call syscall 441
    int result = syscall(&dummy_proc, 441, &regs);

    printf("syscall 441 returned: %d\n", result);
    return 0;
}