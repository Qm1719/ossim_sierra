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
// int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
// {
//   /*Allocate at the toproof */
//   struct vm_rg_struct rgnode;

//   /* TODO: commit the vmaid */
//   // rgnode.vmaid
//   pthread_mutex_lock(&mmvm_lock);
//   if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
//   {
//     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
//     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
//     *alloc_addr = rgnode.rg_start;

//     struct framephy_struct *frames = NULL;
//     int page_count = PAGING_PAGE_COUNT(rgnode.rg_end - rgnode.rg_start);
//     alloc_pages_range(caller, page_count, &frames);
    
//     // Map virtual addresses to physical frames
//     struct vm_rg_struct ret_rg;
//     vmap_page_range(caller, rgnode.rg_start, page_count, frames, &ret_rg);
    
//     pthread_mutex_unlock(&mmvm_lock);
//     return 0;
//   }

//   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

//   /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
//   /*Attempt to increate limit to get space */
//   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

//   int inc_sz = PAGING_PAGE_ALIGNSZ(size);
//   //int inc_limit_ret;

//   /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
//   int old_sbrk = cur_vma->sbrk;

//   /* TODO INCREASE THE LIMIT as inovking systemcall 
//    * sys_memap with SYSMEM_INC_OP 
//    */
//   struct sc_regs regs;
//   regs.a1 = SYSMEM_INC_OP;
//   regs.a2 = vmaid;
//   regs.a3 = inc_sz;
  
//   /* SYSCALL 17 sys_memmap */
//   syscall(caller, 17, &regs);
//   //inc_limit_ret = regs.a0;
//   /* TODO: commit the limit increment */

//   /* TODO: commit the allocation address 
//   // *alloc_addr = ...
//   */
//   caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
//   caller->mm->symrgtbl[rgid].rg_end = old_sbrk + inc_sz;
//   *alloc_addr = old_sbrk;
//   struct framephy_struct *frames = NULL;
//   int page_count = PAGING_PAGE_COUNT(inc_sz);
//   alloc_pages_range(caller, page_count, &frames);
  
//   // Map virtual addresses to physical frames
//   struct vm_rg_struct ret_rg;
//   vmap_page_range(caller, old_sbrk, page_count, frames, &ret_rg);
  
//   // Update sbrk pointer
//   cur_vma->sbrk += inc_sz;
  
//   pthread_mutex_unlock(&mmvm_lock);
//   return 0;

// }
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  //pthread_mutex_lock(&mmvm_lock);
  if (get_free_vmrg_area(caller, vmaid, size, rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode->rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode->rg_end;
 
    *alloc_addr = rgnode->rg_start;

    // struct framephy_struct *frames = NULL;
    // int page_count = (rgnode->rg_end - rgnode->rg_start + PAGING_PAGESZ - 1) / PAGING_PAGESZ;
    // alloc_pages_range(caller, page_count, &frames);
    
    // // Map virtual addresses to physical frames
    // struct vm_rg_struct ret_rg;
    // vmap_page_range(caller, rgnode->rg_start, page_count, frames, &ret_rg);
    free(rgnode);
    //pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* Region allocation failed, increase memory limit */
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  //int old_sbrk = cur_vma->sbrk;
  int inc_limit_ret = -1;
  /* Invoke system call to increase memory limit */
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
  regs.a3 = inc_sz;
  
  syscall(caller, 17, &regs);
  
  /* Check if syscall was successful */
  // if (regs.a0 != 0) {
  //   pthread_mutex_unlock(&mmvm_lock);
  //   return -1;
  // }
  if (get_free_vmrg_area(caller, vmaid, size, rgnode) == 0)
   {
     caller->mm->symrgtbl[rgid].rg_start = rgnode->rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode->rg_end;
     *alloc_addr = rgnode->rg_start;
     inc_limit_ret = 0;
   }
   free(rgnode);
  /* Set up the region and allocate physical memory */
  // caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  // caller->mm->symrgtbl[rgid].rg_end = old_sbrk + inc_sz;
  // *alloc_addr = old_sbrk;
  
  // struct framephy_struct *frames = NULL;
  // int page_count = (inc_sz + PAGING_PAGESZ - 1) / PAGING_PAGESZ;
  // alloc_pages_range(caller, page_count, &frames);
  
  // // Map virtual addresses to physical frames
  // struct vm_rg_struct ret_rg;
  // vmap_page_range(caller, old_sbrk, page_count, frames, &ret_rg);
  
  // // Update sbrk pointer
  // cur_vma->sbrk += inc_sz;
  
  //pthread_mutex_unlock(&mmvm_lock);
  return inc_limit_ret;
}
/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
// int __free(struct pcb_t *caller, int vmaid, int rgid)
// {
//   //struct vm_rg_struct rgnode;

//   // Dummy initialization for avoding compiler dummay warning
//   // in incompleted TODO code rgnode will overwrite through implementing
//   // the manipulation of rgid later

//   if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
//     return -1;

//   /* TODO: Manage the collect freed region to freerg_list */
//   // Retrieve the region that was previously allocated
//    struct vm_rg_struct *freed_rg = malloc(sizeof(struct vm_rg_struct));
//    if (!freed_rg) return -1;  // allocation failed
 
//    freed_rg->rg_start = caller->mm->symrgtbl[rgid].rg_start;
//    freed_rg->rg_end = caller->mm->symrgtbl[rgid].rg_end;
//    freed_rg->rg_next = NULL;
 
//    // Sanity check
//    if (freed_rg->rg_start >= freed_rg->rg_end)
//    {
//      free(freed_rg);
//      return -1;
//    }

//   /*enlist the obsoleted memory region */
//   enlist_vm_freerg_list(caller->mm, freed_rg);

//   return 0;
// }
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
  // Allocate memory in vm_area 0 (default VMA)
  pthread_mutex_lock(&mmvm_lock);
  int ret = __alloc(proc, 0, reg_index, size, &addr);
  if (ret == 0)
     proc->regs[reg_index] = addr;
  
  //fflush(stdout);
  #ifdef IODUMP
    //printf("liballoc: reg=%d size=%d addr=0x%x\n", reg_index, size, addr);
    printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
  printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n",  proc->pid, reg_index, addr, size);
  #ifdef PAGETBL_DUMP
    if (ret == 0)
    print_pgtbl(proc, 0, -1); // Print entire page table
  #endif
    //MEMPHY_dump(proc->mram); // Print memory content
  #endif
  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  /* TODO Implement free region */
  pthread_mutex_lock(&mmvm_lock);
  int ret = __free(proc, 0, reg_index);
  if (ret == 0)
     proc->regs[reg_index] = 0;
  
  //fflush(stdout);
  #ifdef IODUMP
    //printf("libfree: reg=%d\n", reg_index);
    printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
  printf("PID=%d - Region=%d\n", proc->pid, reg_index);
  #ifdef PAGETBL_DUMP
  if (ret == 0)
    print_pgtbl(proc, 0, -1); // Print entire page table
  #endif
    //MEMPHY_dump(proc->mram); // Print memory content
  #endif
  pthread_mutex_unlock(&mmvm_lock);
  return ret;
  /* By default using vmaid = 0 */
  //return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
// int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
// {
//   uint32_t pte = mm->pgd[pgn];

//   if (!PAGING_PAGE_PRESENT(pte))
//   { /* Page is not online, make it actively living */
//     int vicpgn, swpfpn; 
//     int vicfpn;
//     //uint32_t vicpte;

//     int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable

//     /* TODO: Play with your paging theory here */
//     /* Find victim page */
//     find_victim_page(caller->mm, &vicpgn);

//     /* Get free frame in MEMSWP */
//     MEMPHY_get_freefp(caller->active_mswp, &swpfpn);

//     /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
//     vicfpn = PAGING_FPN(mm->pgd[vicpgn]);
//     /* TODO copy victim frame to swap 
//      * SWP(vicfpn <--> swpfpn)
//      * SYSCALL 17 sys_memmap 
//      * with operation SYSMEM_SWP_OP
//      */
//     struct sc_regs regs;
//     regs.a1 = SYSMEM_SWP_OP;        // syscall op: swap
//     regs.a2 = vicfpn;               // src: RAM frame (victim)
//     regs.a3 = swpfpn;               // dst: SWAP frame
    
//     //struct sc_regs regs;

//     /* SYSCALL 17 sys_memmap */
//     syscall(caller, 17, &regs);            // SYSCALL_MEMMAP
//     tgtfpn = vicfpn;
//     // Get the SWAP offset for the page to be loaded
//     int tgt_swpoff = (pte & PAGING_PTE_SWPOFF_MASK) >> PAGING_PTE_SWPOFF_LOBIT;
//     /* TODO copy target frame form swap to mem 
//      * SWP(tgtfpn <--> vicfpn)
//      * SYSCALL 17 sys_memmap
//      * with operation SYSMEM_SWP_OP
//      */
//     /* TODO copy target frame form swap to mem 
//     */
//     regs.a1 = SYSMEM_SWP_OP;
//     regs.a2 = tgt_swpoff;  // src: SWAP frame of target page
//     regs.a3 = tgtfpn;      // dst: free RAM frame

//     /* SYSCALL 17 sys_memmap */
//     syscall(caller, 17, &regs);             // SYSCALL_MEMMAP

//     /* Update page table */
//     //pte_set_swap() 
//     pte_set_swap(&mm->pgd[vicpgn], tgtfpn, tgt_swpoff);
//     //mm->pgd;
//     mm->pgd[pgn] = 0;

//     /* Update its online status of the target page */
//     //pte_set_fpn() &
//     pte_set_fpn(&mm->pgd[pgn], tgtfpn);
//     //mm->pgd[pgn];
//     //pte_set_fpn();

//     enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
//   }

//   *fpn = PAGING_FPN(mm->pgd[pgn]);

//   return 0;
// }
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
      /* TODO copy target frame form swap to mem
       * SWP(tgtfpn <--> freefpn)
       * SYSCALL 17 sys_memmap
       * with operation SYSMEM_SWP_OP
       */
      /* TODO copy target frame form swap to mem */
      tgtfpn = PAGING_PTE_SWP(pte);
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, freefpn);
      pte_set_fpn(&(mm->pgd[pgn]), freefpn);
    }
    else
    {
      if (find_victim_page(caller->mm, &vicpgn) != 0)
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
  int phyaddr = (fpn << PAGING_PAGESZ) + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;    // operation code
  regs.a2 = phyaddr;           // physical address
  regs.a3 = (int)(intptr_t)data; // destination buffer pointer

  /* SYSCALL 17 sys_memmap */
  syscall(caller, 17, &regs);             // SYSCALL_MEMMAP
  // Update data
  // data = (BYTE)
  // Store result from syscall (alternative to directly writing into *data)
  *data = (BYTE) regs.a3;
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
  int phyaddr = (fpn << PAGING_PAGESZ) + off;
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;         // Operation code: write
  regs.a2 = phyaddr;                 // Physical address
  regs.a3 = value;         // Data to write (passed by value)

  /* SYSCALL 17 sys_memmap */
  syscall(caller, 17, &regs);             // SYSCALL_MEMMAP
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
    uint32_t* destination)
{
  BYTE data;
  pthread_mutex_lock(&mmvm_lock);
  int val = __read(proc, 0, source, offset, &data);
  if (destination) *destination = data;
  /* TODO update result of reading action*/
  //destination 
  printf("===== PHYSICAL MEMORY AFTER READING =====\n");
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
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
{pthread_mutex_lock(&mmvm_lock);
  printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
pthread_mutex_unlock(&mmvm_lock);
  return __write(proc, 0, destination, offset, data);
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


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
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
// int find_victim_page(struct mm_struct *mm, int *retpgn)
// {
//   struct pgn_t *pg = mm->fifo_pgn;

//   /* TODO: Implement the theorical mechanism to find the victim page */
//   if (pg == NULL)
//   {
//     return -1;
//   }
//   if (pg->pg_next == NULL)
//     mm->fifo_pgn = NULL;
//   else {
//     struct pgn_t *prev;
//     while (pg->pg_next != NULL)
//     {
//       prev = pg;
//       pg = pg->pg_next;
//     }
//     prev->pg_next = NULL;
//   }
//   *retpgn = pg->pgn;
//   free(pg);

//   return 0;
// }
// int find_victim_page(struct mm_struct *mm, int *retpgn)
// {
//   struct pgn_t *pg = mm->fifo_pgn;

//   // Add debug prints
//   printf("DEBUG: find_victim_page called, fifo_pgn=%p\n", (void*)pg);
  
//   // Handle empty list
//   if (pg == NULL) {
//     printf("DEBUG: Empty FIFO queue, using default page 0\n");
//     *retpgn = 0;
//     return 0;
//   }

//   // Get the first element (oldest page in FIFO)
//   *retpgn = pg->pgn;
//   printf("DEBUG: Victim page selected: PGN=%d\n", *retpgn);
  
//   // Update head pointer to remove first element
//   mm->fifo_pgn = pg->pg_next;
  
//   // Free the node
//   free(pg);

//   return 0;
// }
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
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  //newrg->rg_start = newrg->rg_end = -1;
  struct vm_rg_struct *rgit_prev = NULL, *best_fit = NULL, *best_fit_prev = NULL; 
  /* TODO Traverse on list of free vm region to find a fit space */
  //while (...)
  // ..
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

//#endif
