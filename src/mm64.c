/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */
#include "os-cfg.h"
#include "os-mm.h"

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <string.h>

#if defined(MM64)

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  if (pgd == NULL || p4d == NULL || pud == NULL || pmd == NULL || pt == NULL) {
        return -1; // Tránh lỗi ghi vào con trỏ NULL
  }
	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
//struct krnl_t *krnl = caller->krnl;

  uint64_t *pte;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;
	
  // dummy pte alloc to avoid runtime error
  // pte = malloc(sizeof(addr_t));
#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;
  // 1. Kiểm tra và cấp phát PGD gốc nếu chưa tồn tại
  if (!caller->mm.pgd) {
    caller->mm.pgd = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!caller->mm.pgd) return -1;
    memset(caller->mm.pgd, 0, 512 * sizeof(uint64_t));
  }

  // 2. Ép kiểu mảng thành mảng con trỏ để lấy địa chỉ bảng P4D an toàn
  uint64_t *pgd_table = (uint64_t *)caller->mm.pgd;
  uint64_t *p4d_table = (uint64_t *)(uintptr_t)pgd_table[pgd];
  if (!p4d_table) {
    p4d_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!p4d_table) return -1;
    memset(p4d_table, 0, 512 * sizeof(uint64_t));
    pgd_table[pgd] = (uint64_t)(uintptr_t)p4d_table; // Lưu con trỏ trực tiếp, không ép kiểu số nguyên
  }

  // 3. Xử lý tương tự cho tầng PUD
  uint64_t *pud_table = (uint64_t *)(uintptr_t)p4d_table[p4d];
  if (!pud_table) {
    pud_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pud_table) return -1;
    memset(pud_table, 0, 512 * sizeof(uint64_t));
    p4d_table[p4d] = (uint64_t)(uintptr_t)pud_table;
  }

  // 4. Xử lý tương tự cho tầng PMD
  uint64_t *pmd_table = (uint64_t *)(uintptr_t)pud_table[pud];
  if (!pmd_table) {
    pmd_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pmd_table) return -1;
    memset(pmd_table, 0, 512 * sizeof(uint64_t));
    pud_table[pud] = (uint64_t)(uintptr_t)pmd_table;
  }

  // 5. Xử lý tương tự cho tầng PT
  uint64_t *pt_table = (uint64_t *)(uintptr_t)pmd_table[pmd];
  if (!pt_table) {
    pt_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pt_table) return -1;
    memset(pt_table, 0, 512 * sizeof(uint64_t));
    pmd_table[pmd] = (uint64_t)(uintptr_t)pt_table;
  }

  // Điểm đích PTE cuối cùng (Trỏ chính xác vào phần tử uint64_t của bảng PT)
  pte = &pt_table[pt];

#else
  struct krnl_t *krnl = caller->krnl;
  pte = &krnl->mm->pgd[pgn];
#endif
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
//struct krnl_t *krnl = caller->krnl;

  uint64_t *pte;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;
	
  // dummy pte alloc to avoid runtime error
  // pte = malloc(sizeof(addr_t));
#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;
  // 1. Kiểm tra và cấp phát PGD gốc nếu chưa tồn tại
  if (!caller->mm.pgd) {
    caller->mm.pgd = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!caller->mm.pgd) return -1;
    memset(caller->mm.pgd, 0, 512 * sizeof(uint64_t));
  }

  // 2. Ép kiểu mảng thành mảng con trỏ để lấy địa chỉ bảng P4D an toàn
  uint64_t *pgd_table = (uint64_t *)caller->mm.pgd;
  uint64_t *p4d_table = (uint64_t *)(uintptr_t)pgd_table[pgd];
  if (!p4d_table) {
    p4d_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!p4d_table) return -1;
    memset(p4d_table, 0, 512 * sizeof(uint64_t));
    pgd_table[pgd] = (uint64_t)(uintptr_t)p4d_table; // Lưu con trỏ trực tiếp, không ép kiểu số nguyên
  }

  // 3. Xử lý tương tự cho tầng PUD
  uint64_t *pud_table = (uint64_t *)(uintptr_t)p4d_table[p4d];
  if (!pud_table) {
    pud_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pud_table) return -1;
    memset(pud_table, 0, 512 * sizeof(uint64_t));
    p4d_table[p4d] = (uint64_t)(uintptr_t)pud_table;
  }

  // 4. Xử lý tương tự cho tầng PMD
  uint64_t *pmd_table = (uint64_t *)(uintptr_t)pud_table[pud];
  if (!pmd_table) {
    pmd_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pmd_table) return -1;
    memset(pmd_table, 0, 512 * sizeof(uint64_t));
    pud_table[pud] = (uint64_t)(uintptr_t)pmd_table;
  }

  // 5. Xử lý tương tự cho tầng PT
  uint64_t *pt_table = (uint64_t *)(uintptr_t)pmd_table[pmd];
  if (!pt_table) {
    pt_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pt_table) return -1;
    memset(pt_table, 0, 512 * sizeof(uint64_t));
    pmd_table[pmd] = (uint64_t)(uintptr_t)pt_table;
  }

  // Điểm đích PTE cuối cùng (Trỏ chính xác vào phần tử uint64_t của bảng PT)
  pte = &pt_table[pt];
#else
  struct krnl_t *krnl = caller->krnl;
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
//struct krnl_t *krnl = caller->krnl;
  uint32_t pte = 0;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t	pt=0;
	
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  //pte = &krnl->mm->pt;	
  #ifdef MM64

  if (!caller->mm.pgd) return 0;

  uint64_t *pgd_table = (uint64_t *)caller->mm.pgd;
  uint64_t *p4d_table = (uint64_t *)(uintptr_t)pgd_table[pgd];
  if (!p4d_table) return 0;

  uint64_t *pud_table = (uint64_t *)(uintptr_t)p4d_table[p4d];
  if (!pud_table) return 0;

  uint64_t *pmd_table = (uint64_t *)(uintptr_t)pud_table[pud];
  if (!pmd_table) return 0;

  uint64_t *pt_table = (uint64_t *)(uintptr_t)pmd_table[pmd];
  if (!pt_table) return 0;

  pte = (uint32_t)pt_table[pt];
  #else
  // 32bit
  if (!caller->mm.pgd) return 0;
  return ((uint32_t *)caller->mm.pgd)[pgn];
  
  #endif

  return pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	krnl->mm->pgd[pgn]=pte_val;
	
	return 0;
}

#ifdef MM64

int is_canonical(uint64_t vaddr) {
  uint64_t top_bits = vaddr >> 57;
  // 7 bit dau 0 la user, 1 la kernel
  if(top_bits == 0x00 || top_bits == 0x7F) return 1;
  return 0;
}

#endif

/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  int pgit = 0;
  uint64_t pattern = 0xdeadbeef;

  /* TODO memset the page table with given pattern
   */
  for(pgit = 0; pgit < pgnum; pgit++) {
    addr_t vaddr = addr + (pgit * PAGING_PAGESZ);

    #ifdef MM64

    if(!is_canonical(vaddr)) return -1;
    addr_t pgd, p4d, pud, pmd, pt;
    get_pd_from_address(vaddr, &pgd, &p4d, &pud, &pmd, &pt);

    if (!caller->mm.pgd) {
    caller->mm.pgd = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!caller->mm.pgd) return -1;
    memset(caller->mm.pgd, 0, 512 * sizeof(uint64_t));
  }

  // 2. Ép kiểu mảng thành mảng con trỏ để lấy địa chỉ bảng P4D an toàn
  uint64_t *pgd_table = (uint64_t *)caller->mm.pgd;
  uint64_t *p4d_table = (uint64_t *)(uintptr_t)pgd_table[pgd];
  if (!p4d_table) {
    p4d_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!p4d_table) return -1;
    memset(p4d_table, 0, 512 * sizeof(uint64_t));
    pgd_table[pgd] = (uint64_t)(uintptr_t)p4d_table; // Lưu con trỏ trực tiếp, không ép kiểu số nguyên
  }

  // 3. Xử lý tương tự cho tầng PUD
  uint64_t *pud_table = (uint64_t *)(uintptr_t)p4d_table[p4d];
  if (!pud_table) {
    pud_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pud_table) return -1;
    memset(pud_table, 0, 512 * sizeof(uint64_t));
    p4d_table[p4d] = (uint64_t)(uintptr_t)pud_table;
  }

  // 4. Xử lý tương tự cho tầng PMD
  uint64_t *pmd_table = (uint64_t *)(uintptr_t)pud_table[pud];
  if (!pmd_table) {
    pmd_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pmd_table) return -1;
    memset(pmd_table, 0, 512 * sizeof(uint64_t));
    pud_table[pud] = (uint64_t)(uintptr_t)pmd_table;
  }

  // 5. Xử lý tương tự cho tầng PT
  uint64_t *pt_table = (uint64_t *)(uintptr_t)pmd_table[pmd];
  if (!pt_table) {
    pt_table = (uint64_t *)malloc(512 * sizeof(uint64_t));
    if (!pt_table) return -1;
    memset(pt_table, 0, 512 * sizeof(uint64_t));
    pmd_table[pmd] = (uint64_t)(uintptr_t)pt_table;
  }

  pt_table[pt] = (uint64_t) pattern;

    #else
    // 32 bit

    #endif
  }
  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit;
  int pgit = 0;
  addr_t pgn;

  /* TODO: update the rg_end and rg_start of ret_rg 
  //ret_rg->rg_end =  ....
  //ret_rg->rg_start = ...
  //ret_rg->vmaid = ...
  */
  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;
  ret_rg->vmaid = 0;

  /* TODO map range of frame to address space
   *      [addr to addr + pgnum*PAGING_PAGESZ
   *      in page table caller->krnl->mm->pgd,
   *                    caller->krnl->mm->pud...
   *                    ...
   */

  fpit = frames;
  for(pgit = 0; pgit < pgnum; pgit++) {
    if(fpit == NULL) break;

    // Tính địa chỉ ảo hiện tại và trích xuất số hiệu trang (pgn)
    addr_t vaddr = addr + pgit * PAGING_PAGESZ;
    pgn = vaddr >> PAGING64_ADDR_PT_SHIFT;

    // Gọi hàm dựng cấu trúc bảng trang 5 cấp
    pte_set_fpn(caller, pgn, fpit->fpn);
    /* Tracking for later page replacement activities (if needed)
     * Enqueue new usage page */
    enlist_pgn_node(&caller->mm.fifo_pgn, pgn);
  
    // Chuyển sang trang tiếp theo
    fpit = fpit->fp_next;
  }


  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str = NULL;

  /* TODO: allocate the page 
  //caller-> ...
  //frm_lst-> ...
  */
  if (frm_lst == NULL) return -1;
  *frm_lst = NULL;

  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    // TODO: allocate the page 
    if (MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
      newfp_str = malloc(sizeof(struct framephy_struct));
      if(newfp_str == NULL) return -1;

      newfp_str->fpn = fpn;

      newfp_str->fp_next = *frm_lst;
      *frm_lst = newfp_str;
    }
    else
    { // TODO: ERROR CODE of obtaining somes but not enough frames
      return -3000;
    }
  }


  /* End TODO */

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;
//int pgnum = incpgnum;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL) return -1;

  /* TODO init page table directory */
  mm->pgd = (uint64_t*)malloc(512 * sizeof(uint64_t));
  if(mm->pgd == NULL) {
    free(vma0);
    return -1;
  }
  // printf("[DEBUG] PID %d - PGD Address: %p\n", caller->pid, (void*)mm->pgd);
  // for (int i = 0; i < 5; i++) {
  //     printf("   -> pgd[%d] value: 0x%llx\n", i, (unsigned long long)mm->pgd[i]);
  // }

  memset(mm->pgd, 0, 512 * sizeof(uint64_t));

  mm->p4d = NULL;
  mm->pud = NULL;
  mm->pmd = NULL;
  mm->pt = NULL;


  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
  vma0->vm_next = NULL;

  /* Point vma owner backward */
  vma0->vm_mm = mm; 

  /* TODO: update mmap */
  mm->mmap = vma0;
  memset(mm->symrgtbl, 0, PAGING_MAX_SYMTBL_SZ * sizeof(struct vm_rg_struct));
  mm->fifo_pgn = NULL;
  // mm->symrgtbl = 
  // mm->kcpooltbl

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
//addr_t pgn_start;//, pgn_end;
//addr_t pgit;
//struct krnl_t *krnl = caller->krnl;

  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

  get_pd_from_address(start, &pgd, &p4d, &pud, &pmd, &pt);

  /* TODO traverse the page map and dump the page directory entries */

  // 1. Kiểm tra cầu chì an toàn tránh crash hệ thống nếu tiến trình chưa có bộ nhớ
  if (caller == NULL || caller->mm.pgd == NULL) {
    printf("print_pgtbl: Page table is empty or invalid process.\n");
    return -1;
  }

  // 2. Khai báo các biến lưu trữ chỉ mục kết thúc được phân rã từ địa chỉ 'end'
  addr_t end_pgd = 0, end_p4d = 0, end_pud = 0, end_pmd = 0, end_pt = 0;
  get_pd_from_address(end, &end_pgd, &end_p4d, &end_pud, &end_pmd, &end_pt);

  uint64_t *pgd_table = caller->mm.pgd;

  // 3. Trích xuất địa chỉ thực tế của các tầng để in log debug mẫu theo đúng định dạng
  uint64_t *p4d_table = (uint64_t *)(uintptr_t)pgd_table[pgd];
  uint64_t *pud_table = p4d_table ? (uint64_t *)(uintptr_t)p4d_table[p4d] : NULL;
  uint64_t *pmd_table = pud_table ? (uint64_t *)(uintptr_t)pud_table[pud] : NULL;

  printf("print_pgtbl:\n");
  printf(" PDG=%llx P4g=%llx PUD=%llx PMD=%llx\n",
       (unsigned long long)(uintptr_t)pgd_table,  // Địa chỉ thực của bảng PGD
       (unsigned long long)(uintptr_t)p4d_table,  // Địa chỉ thực của bảng P4D
       (unsigned long long)(uintptr_t)pud_table,  // Địa chỉ thực của bảng PUD
       (unsigned long long)(uintptr_t)pmd_table); // Địa chỉ thực của bảng PMD

  // 4. TIẾN HÀNH DUYỆT CÂY 5 TẦNG ĐỂ IN CHI TIẾT CÁC PTE (Nếu cần dump chi tiết)
  // TẦNG 5: Duyệt qua Page Global Directory (PGD) từ vị trí 'pgd' của địa chỉ start
  for (addr_t i_pgd = pgd; i_pgd <= end_pgd; i_pgd++) {
    if (pgd_table[i_pgd] == 0) continue; // Nhánh trống, bỏ qua

    uint64_t *cur_p4d = (uint64_t *)pgd_table[i_pgd];
    
    // TẦNG 4: Duyệt qua Page Level 4 Directory (P4D) kèm theo ranh giới biên động
    addr_t lim_start_p4d = (i_pgd == pgd) ? p4d : 0; //
    addr_t lim_end_p4d = (i_pgd == end_pgd) ? end_p4d : 511; //

    for (addr_t i_p4d = lim_start_p4d; i_p4d <= lim_end_p4d; i_p4d++) {
      if (cur_p4d[i_p4d] == 0) continue; //

      uint64_t *cur_pud = (uint64_t *)cur_p4d[i_p4d];

      // TẦNG 3: Duyệt qua Page Upper Directory (PUD)
      addr_t lim_start_pud = (i_pgd == pgd && i_p4d == p4d) ? pud : 0; //
      addr_t lim_end_pud = (i_pgd == end_pgd && i_p4d == end_p4d) ? end_pud : 511; //

      for (addr_t i_pud = lim_start_pud; i_pud <= lim_end_pud; i_pud++) {
        if (cur_pud[i_pud] == 0) continue; //

        uint64_t *cur_pmd = (uint64_t *)cur_pud[i_pud];

        // TẦNG 2: Duyệt qua Page Middle Directory (PMD)
        addr_t lim_start_pmd = (i_pgd == pgd && i_p4d == p4d && i_pud == pud) ? pmd : 0; //
        addr_t lim_end_pmd = (i_pgd == end_pgd && i_p4d == end_p4d && i_pud == end_pud) ? end_pmd : 511; //

        for (addr_t i_pmd = lim_start_pmd; i_pmd <= lim_end_pmd; i_pmd++) {
          if (cur_pmd[i_pmd] == 0) continue; //

          uint64_t *pt_table = (uint64_t *)cur_pmd[i_pmd];

          // TẦNG 1: Duyệt qua Page Table (PT) cuối cùng để lấy giá trị PTE thực tế
          addr_t lim_start_pt = (i_pgd == pgd && i_p4d == p4d && i_pud == pud && i_pmd == pmd) ? pt : 0; //
          addr_t lim_end_pt = (i_pgd == end_pgd && i_p4d == end_p4d && i_pud == end_pud && i_pmd == end_pmd) ? end_pt : 511; //

          for (addr_t i_pt = lim_start_pt; i_pt <= lim_end_pt; i_pt++) {
            uint64_t pte_val = pt_table[i_pt];
            if (pte_val == 0) continue; // Mục nhập trống, trang chưa ánh xạ

            // Tái cấu trúc địa chỉ ảo để in log chi tiết nếu cần đối chiếu kịch bản kĩ hơn
            addr_t vaddr = (i_pgd << PAGING64_ADDR_PGD_LOBIT) | 
                           (i_p4d << PAGING64_ADDR_P4D_LOBIT) | 
                           (i_pud << PAGING64_ADDR_PUD_LOBIT) | 
                           (i_pmd << PAGING64_ADDR_PMD_LOBIT) | 
                           (i_pt << PAGING64_ADDR_PT_LOBIT);

            int is_present = (pte_val >> 31) & 1; // Bit 31: Present
            int is_swapped = (pte_val >> 30) & 1; // Bit 30: Swapped

          }
        }
      }
    }
  }

  return 0;
}

#endif  //def MM64