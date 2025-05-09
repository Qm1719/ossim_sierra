/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>
 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
 
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
 
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
 
   /* Enlist the new region */
   mm->mmap->vm_freerg_list = rg_elmt;
 
   return 0;
 }
 
 /*get_symrg_byid - get mem region by region ID
  *@mm: memory region
  *@rgid: region ID act as symbol index of variable
  *
  */
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
 
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
 {
   /*Allocate at the toproof */
   struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
 
   /* TODO: commit the vmaid */
   // rgnode->vmaid
   if (get_free_vmrg_area(caller, vmaid, size, rgnode) == 0)
   {
     caller->mm->symrgtbl[rgid].rg_start = rgnode->rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode->rg_end;
 
     *alloc_addr = rgnode->rg_start;
     free(rgnode);
     return 0;
   }
 
   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
 
   /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
   /*Attempt to increate limit to get space */
   // struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   int inc_sz = PAGING_PAGE_ALIGNSZ(size);
   int inc_limit_ret = -1;
 
   /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
   // int old_sbrk = cur_vma->sbrk;
 
   /* TODO INCREASE THE LIMIT as inovking systemcall
    * sys_memap with SYSMEM_INC_OP
    */
   struct sc_regs regs;
   regs.a1 = SYSMEM_INC_OP;
   regs.a2 = vmaid;
   regs.a3 = inc_sz;
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
 
   /* TODO: commit the limit increment */
   if (get_free_vmrg_area(caller, vmaid, size, rgnode) == 0)
   {
     caller->mm->symrgtbl[rgid].rg_start = rgnode->rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode->rg_end;
     *alloc_addr = rgnode->rg_start;
     inc_limit_ret = 0;
   }
   /* TODO: commit the allocation address
   // *alloc_addr = ...
   */
   free(rgnode);
   return inc_limit_ret;
 }
 
 /*__free - remove a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __free(struct pcb_t *caller, int vmaid, int rgid)
 {
   struct vm_rg_struct *rgnode = get_symrg_byid(caller->mm, rgid);
   if (rgnode == NULL)
     return -1;
 
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (cur_vma == NULL)
     return -1;
 
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list, *rgit_prev = NULL;
 
   if (rgit == NULL)
   {
     struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
     *newrg = *rgnode;
     enlist_vm_freerg_list(caller->mm, newrg);
     rgnode->rg_start = rgnode->rg_end = -1;
     return 0;
   }
 
   while (rgit != NULL && rgit->rg_start < rgnode->rg_end)
   {
     rgit_prev = rgit;
     rgit = rgit->rg_next;
   }
 
   if (rgit_prev == NULL)
   {
     struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
     *newrg = *rgnode;
     enlist_vm_rg_node(&cur_vma->vm_freerg_list, newrg);
     rgnode->rg_start = rgnode->rg_end = -1;
     return 0;
   }
 
   int case_id = ((rgit && rgit->rg_start == rgnode->rg_end) ? 1 : 0) +
                 ((rgit_prev->rg_end == rgnode->rg_start) ? 2 : 0);
 
   switch (case_id)
   {
   case 1:
     rgit->rg_start = rgnode->rg_start;
     if (cur_vma->sbrk == rgnode->rg_end)
       cur_vma->sbrk = rgnode->rg_start;
     break;
 
   case 2:
     rgit_prev->rg_end = rgnode->rg_end;
     break;
 
   case 3:
     rgit_prev->rg_end = rgit->rg_end;
     rgit_prev->rg_next = rgit->rg_next;
     free(rgit);
     if (cur_vma->sbrk == rgnode->rg_end)
       cur_vma->sbrk = rgit_prev->rg_start;
     break;
 
   case 0:
   {
     struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
     *newrg = *rgnode;
     rgit_prev->rg_next = newrg;
     newrg->rg_next = rgit;
     break;
   }
   }
 
   rgnode->rg_start = rgnode->rg_end = -1;
   return 0;
 }
 
 /*liballoc - PAGING-based allocate a region memory
  *@proc:  Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   /* TODO Implement allocation on vm area 0 */
   int addr;
   pthread_mutex_lock(&mmvm_lock);
   /* By default using vmaid = 0 */
   int ret_value = __alloc(proc, 0, reg_index, size, &addr);
   if (ret_value == 0)
     proc->regs[reg_index] = addr;
 
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
   printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n", proc->pid, reg_index, addr, size);
 #ifdef PAGETBL_DUMP
   if (ret_value == 0)
     print_pgtbl(proc, 0, -1);
 #endif
   printf("====================================================================\n");
 #endif
   pthread_mutex_unlock(&mmvm_lock);
   return ret_value;
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   /* TODO Implement free region */
 
   /* By default using vmaid = 0 */
   pthread_mutex_lock(&mmvm_lock);
   int ret_value = __free(proc, 0, reg_index);
   if (ret_value == 0)
     proc->regs[reg_index] = 0;
 
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
   printf("PID=%d - Region=%d\n", proc->pid, reg_index);
 #ifdef PAGETBL_DUMP
   if (ret_value == 0)
     print_pgtbl(proc, 0, -1);
 #endif
   printf("====================================================================\n");
 #endif
   pthread_mutex_unlock(&mmvm_lock);
 
   return ret_value;
 }
 
 /*pg_getpage - get the page in ram
  *@mm: memory region
  *@pagenum: PGN
  *@framenum: return FPN
  *@caller: caller
  *
  */
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
   uint32_t pte = mm->pgd[pgn];
 
   if (!PAGING_PAGE_PRESENT(pte))
   { /* Page is not online, make it actively living */
     int vicpgn, vicfpn, swpfpn, freefpn, tgtfpn;
     uint32_t vicpte;
 
     // int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable
 
     /* TODO: Play with your paging theory here */
     /* Find free page first, then victim page */
     if (MEMPHY_get_freefp(caller->mram, &freefpn) == 0)
     {
       /* TODO copy target frame from swap to mem
        * SWP(tgtfpn <--> freefpn)
        * SYSCALL 17 sys_memmap
        * with operation SYSMEM_SWP_OP
        */
       /* TODO copy target frame from swap to mem */
       tgtfpn = PAGING_PTE_SWP(pte);
       __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, freefpn);
       pte_set_fpn(&(mm->pgd[pgn]), freefpn); //Cập nhật bảng trang 
     }
     else
     {
       if (find_victim_page(caller->mm, &vicpgn) != 0) //Nếu không tìm thấy trang nào
         return -1;
       vicpte = mm->pgd[vicpgn];
       vicfpn = PAGING_PTE_FPN(vicpte);
       /* Get free frame in MEMSWP */
       if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn))
         return -1;
 
       /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
 
       /* TODO copy victim frame to swap
        * SWP(vicfpn <--> swpfpn)
        * SYSCALL 17 sys_memmap
        * with operation SYSMEM_SWP_OP
        */
       struct sc_regs regs;
       regs.a1 = SYSMEM_SWP_OP;
       regs.a2 = vicfpn;
       regs.a3 = swpfpn;
 
       /* SYSCALL 17 sys_memmap */
       syscall(caller, 17, &regs);
       /* TODO copy target frame form swap to mem
        * SWP(tgtfpn <--> vicfpn)
        * SYSCALL 17 sys_memmap
        * with operation SYSMEM_SWP_OP
        */
       /* TODO copy target frame form swap to mem */
       tgtfpn = PAGING_PTE_SWP(pte);
       __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
 
       /* Update page table */
       pte_set_swap(&(mm->pgd[vicpgn]), 0, swpfpn);
 
       /* Update its online status of the target page */
       pte_set_fpn(&(mm->pgd[pgn]), vicfpn);
     }
     PAGING_PTE_SET_PRESENT(mm->pgd[pgn]);
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
   }
 
   *fpn = PAGING_FPN(mm->pgd[pgn]);
 
   return 0;
 }
 
 /*pg_getval - read value at given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_read(caller->mram, phyaddr, data);
    *  MEMPHY READ
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
    */
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_READ;
   regs.a2 = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
   // Update data
   *data = (BYTE)regs.a3;
 
   return 0;
 }
 
 /*pg_setval - write value to given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_write(caller->mram, phyaddr, value);
    *  MEMPHY WRITE
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_WRITE
    */
   // int phyaddr
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_WRITE;
   regs.a2 = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
   regs.a3 = value;
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
   // Update data
   // data = (BYTE)
 
   return 0;
 }
 
 /*__read - read value in region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
 
   return 0;
 }
 
 /*libread - PAGING-based read a region memory */
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t *destination)
 {
   BYTE data;
   pthread_mutex_lock(&mmvm_lock);
   int val = __read(proc, 0, source, offset, &data);
   if (val == 0)
     *destination = (uint32_t)data;
   /* TODO update result of reading action*/
   // destination
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER READING =====\n");
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
   printf("====================================================================\n");
 #endif
   pthread_mutex_unlock(&mmvm_lock);
   return val;
 }
 
 /*__write - write a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
 
   return 0;
 }
 
 /*libwrite - PAGING-based write a region memory */
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
   pthread_mutex_lock(&mmvm_lock);
   int ret_value = __write(proc, 0, destination, offset, data);
 
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
   printf("====================================================================\n");
 #endif
   pthread_mutex_unlock(&mmvm_lock);
   return ret_value;
 }
 
 /*free_pcb_memphy - collect all memphy of pcb
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@incpgnum: number of page
  */
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
 
   for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
   {
     pte = caller->mm->pgd[pagenum];
 
     if (!PAGING_PAGE_PRESENT(pte))
     {
       fpn = PAGING_PTE_FPN(pte);
       MEMPHY_put_freefp(caller->mram, fpn);
     }
     else
     {
       fpn = PAGING_PTE_SWP(pte);
       MEMPHY_put_freefp(caller->active_mswp, fpn);
     }
   }
 
   return 0;
 }
 
 /*find_victim_page - find victim page
  *@caller: caller
  *@pgn: return page number
  *
  */
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   struct pgn_t *pg = mm->fifo_pgn;
   if (!pg)
     return -1;
   /* TODO: Implement the theorical mechanism to find the victim page */
   *retpgn = pg->pgn;
   mm->fifo_pgn = pg->pg_next;
   free(pg);
 
   return 0;
 }
 
 /*get_free_vmrg_area - get a free vm region
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@size: allocated size
  *
  */
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (cur_vma == NULL)
     return -1;
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list; // the first address of cur_vma
 
   if (rgit == NULL)
     return -1;
 
   /* Probe unintialized newrg */
   struct vm_rg_struct *rgit_prev = NULL, *best_fit = NULL, *best_fit_prev = NULL; 
   // best fit = smallest region that is larger than size
   /* TODO Traverse on list of free vm region to find a fit space */
   // while (...)
   // freelist-> [0..9] -> [12 ...22]
   // [0->3]
   while (rgit != NULL)
   {
     if (rgit->rg_end - rgit->rg_start >= size && 
         (!best_fit || best_fit->rg_end - best_fit->rg_start > rgit->rg_end - rgit->rg_start))
     {
       best_fit = rgit;
       best_fit_prev = rgit_prev;
     }
     rgit_prev = rgit;
     rgit = rgit->rg_next;
   }
 
   if (best_fit == NULL) // no fit
     return -1;
   
   newrg->rg_start = best_fit->rg_start;
   newrg->rg_end = best_fit->rg_start + size;
   if (newrg->rg_start == cur_vma->sbrk)
     cur_vma->sbrk = newrg->rg_end;
 
   if (newrg->rg_end == best_fit->rg_end)
   {
     if (best_fit_prev == NULL) // firstnode
       cur_vma->vm_freerg_list = best_fit->rg_next;
     else
       best_fit_prev->rg_next = best_fit->rg_next; 
     free(best_fit);
   } else
     best_fit->rg_start = newrg->rg_end;
   
   return 0;
 }
 
 // #endif