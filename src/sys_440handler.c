#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "string.h"
#include "queue.h"
#include <pthread.h>

static pthread_mutex_t queue_lock;
int __sys_440handler(struct pcb_t *caller, struct sc_regs* regs)
{
    pthread_mutex_lock(&queue_lock);
    struct queue_t *rl = caller->running_list;
    for (int i = 0; i < rl->size; i++) {  
        struct pcb_t *proc = rl->proc[i];
        char* proc_path = proc->path + 11; 
        printf("Process at idx %i of running queue: %s\n", i, proc_path);
    } 
    for (int i = 0; i < MAX_PRIO; i++) {
        struct queue_t *mlq = &caller->mlq_ready_queue[i]; 
        for (int  j = 0; j < mlq->size; j++){
            struct pcb_t *proc = mlq->proc[j];
            char* proc_path = proc->path + 11; 
            printf("Process at IDX: %i MLQ_Queue[%i]: %s\n", j, i, proc_path);
        }
    } 
    pthread_mutex_unlock(&queue_lock);
    return 0;
}