#include <common.h>
#include <syscall.h>
#include <stdio.h>

int __sys_xxxhandler(struct pcb_t *caller, struct sc_regs *reg){

    printf("The first system call parameter %ld\n", reg->a1);
    return 0;
}