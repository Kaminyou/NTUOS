#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"
#define PTE_RWX (PTE_R | PTE_W | PTE_X | PTE_U)

/* NTU OS 2022 */
/* Page fault handler */
int handle_pgfault() {
  /* Find the address that caused the fault */
  /* uint64 va = r_stval(); */

  /* TODO */
  // Get the address that caused the fault
  uint64 va = r_stval();
  // Round down to the nearest page boundary
  va = PGROUNDDOWN(va);

  // Find the process's page table entry for the virtual address
  struct proc *p = myproc();
  pte_t *pte = walk(p->pagetable, va, 1);

  if (*pte & PTE_S) {
    // Page was swapped out, so read it in from disk
    begin_op();
    uint64 blockno = PTE2BLOCKNO(*pte);
    char *pa = kalloc();
    read_page_from_disk(ROOTDEV, pa, blockno);
    *pte = PA2PTE(pa) | PTE_FLAGS(*pte);
    end_op();

    *pte &= ~PTE_S; // unset
    *pte |= PTE_V; // set
  }
  else {
    // Page was not previously allocated, so allocate a new page
    char *pa = kalloc();
    if (pa == 0) return -3;

    memset(pa, 0, PGSIZE);
    if (mappages(p->pagetable, va, PGSIZE, (uint64)pa, PTE_RWX) != 0) {
      kfree(pa);
      return -4;
    }
  }

  return 0;
}