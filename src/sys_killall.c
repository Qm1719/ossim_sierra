/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "common.h"
 #include "syscall.h"
 #include "stdio.h"
 #include "libmem.h"
 #include "string.h" 
 #include "queue.h"  
 #include "sched.h"  
 
 static const char *get_proc_name(const char *path){
     const char *base = strrchr(path, '/');
     if (base)
         return base + 1; 
     else
         return path;
 }
 
 int __sys_killall(struct pcb_t *caller, struct sc_regs *regs){
     char proc_name[100];
     uint32_t data;
     uint32_t memrg = regs->a1;
     int i = 0;
     int killed_count = 0;
 
     data = 0;
     while (i < (int)sizeof(proc_name) - 1){
         if (libread(caller, memrg, i, &data) < 0)
         {
             printf("Error reading memory region %d at offset %d\n", memrg, i);
             return -1; 
         }
         if (data == 0 || data == -1)
         {
             proc_name[i] = '\0';
             break;
         }
         proc_name[i] = (char)data;
         i++;
     }
 
     if (strlen(proc_name) == 0){
         printf("Error reading process name from region %d\n", memrg);
         return -1; 
     }
 
     printf("The process retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);
  
     struct queue_t *mlq_queues = caller->mlq_ready_queue;
     struct queue_t *running_queue = caller->running_list;
 
     if(!mlq_queues || !running_queue){
         printf("MLQ or running queue is not initialized.\n");
         return -1; 
     }
 
     for (int prio = 0; prio < MAX_PRIO; prio++){
         struct queue_t *current_ready_queue = &mlq_queues[prio];
         for (int j = current_ready_queue->size - 1; j >= 0; j--)
         {
             struct pcb_t *proc = current_ready_queue->proc[j];
             if (proc != NULL)
             {
                 const char *base_name = get_proc_name(proc->path);
                 printf("Ready queue process PID %d (name: %s)\n", proc->pid, base_name);
                 if (strcmp(proc_name, base_name) == 0)
                 {
                     proc->pc = proc->code->size;
 
                     for (int k = j; k < current_ready_queue->size - 1; k++)
                     {
                         current_ready_queue->proc[k] = current_ready_queue->proc[k + 1];
                     }
                     current_ready_queue->proc[current_ready_queue->size - 1] = NULL; 
                     current_ready_queue->size--;
                     killed_count++;
                 }
             }
         }
     }
 
     for (int j = running_queue->size - 1; j >= 0; j--){
         struct pcb_t *proc = running_queue->proc[j];
         if (proc != NULL)
         {
             const char *base_name = get_proc_name(proc->path);
             printf("Running process PID %d (name: %s)\n", proc->pid, base_name);
             if (strcmp(proc_name, base_name) == 0)
             {
                 proc->pc = proc->code->size;
                 for (int k = j; k < running_queue->size - 1; k++)
                 {
                     running_queue->proc[k] = running_queue->proc[k + 1];
                 }
                 running_queue->proc[running_queue->size - 1] = NULL; 
                 running_queue->size--;
                 killed_count++;
             }
         }
     }
     printf("Number of process terminated: %d, name: \"%s\".\n", killed_count, proc_name);
 
     libfree(caller, memrg); 
 
     return 0; // Thành công
 }
 