/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (pvma != NULL && vmait < vmaid)
  {

    pvma = pvma->vm_next;
    if (pvma == NULL)
      return NULL;

    vmait = pvma->vm_id;
  }
  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->kernl->mm, vmaid);

  //newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
  if(newrg == NULL) return NULL;
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;
  newrg->rg_next = NULL;
  /* END TODO */

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  //struct vm_area_struct *vma = caller->krnl->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  struct vm_area_struct *vma = caller->krnl->mm->mmap;
  if (vma == NULL)
  {
    return -1;
  }

  /* TODO validate the planned memory area is not overlapped */

  struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_area == NULL)
  {
    return -1;
  }

  while (vma != NULL)
  {
    if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end))
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  /* End TODO*/

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  // Cấp phát vùng nhớ mới
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  if(newrg == NULL) return -1;

  // Lấy cấu trúc VMA hiện tại của tiến trình để thao tác ranh giới sbrk
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  if(cur_vma == NULL) {
    free(newrg);
    return -1;
  }

  // Lưu lại điểm kết thúc hiện tại trước khi tăng giới hạn bộ nhớ ảo
  addr_t old_end = cur_vma->sbrk;

  /* TOTO with new address scheme, the size need tobe aligned 
   *      the raw inc_sz maybe not fit pagesize
   */ 

  // Biến kích thước tăng thực tế sau khi làm tròn theo trang
  addr_t inc_amt = inc_sz;
  if (inc_amt % PAGING_PAGESZ != 0) {
      inc_amt = ((inc_amt / PAGING_PAGESZ) + 1) * PAGING_PAGESZ;
  }

  // Biến tính toán số lượng trang cần nạp
  int incnumpage =  inc_amt / PAGING_PAGESZ;

  // Định hình ranh giới vùng nhớ ảo mới dự kiến cho Region
  newrg->rg_start = old_end;
  newrg->rg_end = old_end + inc_sz; // Kích thước thực tế mà user mong muốn sử dụng
  newrg->rg_next = NULL;

  /* TODO Validate overlap of obtained region */
  // Kiểm tra chồng lấp vùng nhớ
  if (validate_overlap_vm_area(caller, vmaid, newrg->rg_start, newrg->rg_start + inc_amt) < 0) {
      free(newrg);
      return -1; /*Overlap and failed allocation */
  }
 
  /* TODO: Obtain the new vm area based on vmaid */
  //cur_vma->vm_end... 
  // inc_limit_ret...
  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->sbrk = old_end + inc_amt; 
  if (cur_vma->vm_end < cur_vma->sbrk) {
      cur_vma->vm_end = cur_vma->sbrk;
  }
  
  // Ánh xạ page table
  if (vm_map_ram(caller, cur_vma->vm_start, cur_vma->vm_end, 
                    old_end, incnumpage , newrg) < 0) {
        free(newrg);
        return -1; /* Map the memory to MEMRAM */
    }
    enlist_vm_rg_node(&cur_vma->vm_freerg_list, newrg);
    return 0;
}


// #endif
