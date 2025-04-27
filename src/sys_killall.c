// /*
//  * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
//  */

// /* Sierra release
//  * Source Code License Grant: The authors hereby grant to Licensee
//  * personal permission to use and modify the Licensed Source Code
//  * for the sole purpose of studying while attending the course CO2018.
//  */

// #include "common.h"
// #include "syscall.h"
// #include "stdio.h"
// #include "libmem.h"

// int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
// {
//     char proc_name[100];
//     uint32_t data;

//     //hardcode for demo only
//     uint32_t memrg = regs->a1;
    
//     /* TODO: Get name of the target proc */
//     //proc_name = libread..
//     int i = 0;
//     data = 0;
//     while(data != -1){
//         libread(caller, memrg, i, &data);
//         proc_name[i]= data;
//         if(data == -1) proc_name[i]='\0';
//         i++;
//     }
//     printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

//     /* TODO: Traverse proclist to terminate the proc
//      *       stcmp to check the process match proc_name
//      */
//     //caller->running_list
//     //caller->mlq_ready_queu

//     /* TODO Maching and terminating 
//      *       all processes with given
//      *        name in var proc_name
//      */

//     return 0; 
// }
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
 #include "queue.h"  // Add this include for queue_t definition
 #include <string.h> // Add this include for string functions
 #include <pthread.h>  // Add pthread support
 
 static pthread_mutex_t queue_lock;
 
 int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
 {
     char proc_name[100];
     uint32_t data;
 
     //hardcode for demo only
     uint32_t memrg = regs->a1;
     
     /* TODO: Get name of the target proc */
     //proc_name = libread..
     int i = 0;
     data = 0;
     while(data != -1){
         libread(caller, memrg, i, &data);
         proc_name[i]= data;
         if(data == -1) proc_name[i]='\0';
         i++;
     }
     printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);
 
     /* TODO: Traverse proclist to terminate the proc
      *       stcmp to check the process match proc_name
      */
     //caller->running_list
     //caller->mlq_ready_queu
 
     /* TODO Maching and terminating 
      *       all processes with given
      *        name in var proc_name
      */
 
     //return 0;
     
     /* Traverse process lists to terminate matching processes */
     int killed_count = 0;
     pthread_mutex_lock(&queue_lock);
     // Check processes in ready queues at all priority levels
     for (int prio = 0; prio < MAX_PRIO; prio++) {
         struct queue_t *ready_q = &caller->mlq_ready_queue[prio];
         i = 0;
         while (i < ready_q->size) {
             struct pcb_t *curr = ready_q->proc[i];
             
             // Extract filename from path (e.g., input/proc/s1 -> s1)
             char *filename = strrchr(curr->path, '/');
             if (filename != NULL) {
                 filename++; // Skip the '/' character
             } else {
                 filename = curr->path; // If no '/', use the whole path
             }
             
             // Check if the filename matches the target process name
             if (strcmp(filename, proc_name) == 0) {
                 // Mark the process as completed
                
                 curr->pc = curr->code->size;
                 printf("Marked process '%s' (PID=%d) as completed\n", filename, curr->pid);
                 
                 killed_count++;
                 
                 // Move last process to current position and decrease queue size
                 ready_q->proc[i] = ready_q->proc[ready_q->size - 1];
                 ready_q->size--;
                 // Don't increment i as we need to check the new element at position i
             } else {
                 i++; // Only increment i if we didn't remove an element
             }
         }
     }
 
     // Check processes in the running list
     struct queue_t *run_list = caller->running_list;
     i = 0;
     while (i < run_list->size) {
         struct pcb_t *curr = run_list->proc[i];
         
         // Extract filename from path
         char *filename = strrchr(curr->path, '/');
         if (filename != NULL) {
             filename++; // Skip the '/' character
         } else {
             filename = curr->path; // If no '/', use the whole path
         }
         
         // Check if the filename matches the target process name
         if (strcmp(filename, proc_name) == 0) {
             // Mark the process as completed
             curr->pc = curr->code->size;
             printf("Marked process '%s' (PID=%d) as completed\n", filename, curr->pid);
             killed_count++;
             
             // Move last process to current position and decrease queue size
             run_list->proc[i] = run_list->proc[run_list->size - 1];
             run_list->size--;
             // Don't increment i as we need to check the new element at position i
         } else {
             i++; // Only increment i if we didn't remove an element
         }
     }
     pthread_mutex_unlock(&queue_lock);
     
     printf("Killed %d processes named \"%s\"\n", killed_count, proc_name);
     return killed_count;
 }
 