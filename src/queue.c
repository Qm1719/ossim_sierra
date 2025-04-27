#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (q->size >= MAX_QUEUE_SIZE) {
                fprintf(stderr, "Queue is full\n");
                return;
        }
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (empty(q)) {return NULL;}
        int min_prio = MAX_PRIO;
        int min_index = -1;
        for (int i = 0; i < q->size; i++) {
                if (q->proc[i]->priority < min_prio) {
                        min_prio = q->proc[i]->priority;
                        min_index = i;
                }
        }
        struct pcb_t * proc = q->proc[min_index];
        for (int i = min_index; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;
        q->proc[q->size] = NULL;
        return proc;
}

