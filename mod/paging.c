#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/memory.h>
#include <linux/mm.h>

#include <paging.h>

/**
 * Lab 03: Memory Management and Paging
 * 		paging.c - kernel module
 *      creates /dev/paging device file and registers
 *      mmap() callback that installs page fault handler
 *      when processes try to allocate memory via device
 *      file
 */

struct State { // data structure to track what physical mem has been allocated for a process
  atomic_t counter; // atomic reference counter
  atomic_t * page_alloc; // page allocated for process (so it can be freed)
} state;

atomic_t alloc_page; // increment every time allocate new struct page
atomic_t free_page; // increment every time free struct page

/*
 * do_fault()
 *    allocs new page of physical mem and updates
 *    process' page tables
 */
static int do_fault(struct vm_area_struct * vma, unsigned long fault_address) {
  struct page * page;

  printk(KERN_INFO "paging_vma_fault() invoked: took a page fault at VA 0x%lx\n", fault_address);

  // alloc page of physical memory
  page = alloc_page(GFP_KERNEL);

  if (!page) { // still uninitialized
    printk(KERN_ERR "Memory allocation fail\n");
    return VM_FAULT_OOM;
  }
  atomic_inc(&alloc_page);

  // update process' page tables to map faulting virtual address to new physical address (page)
  state.page_alloc = (void*) (long) remap_pfn_range(vma, PAGE_ALIGN(fault_address), page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
  if (state.page_alloc == 0) { // success page table update
    return VM_FAULT_NOPAGE; // success message
  }
  printk(KERN_ERR "Failure in updating process' page tables\n");
  return VM_FAULT_SIGBUS;
}

/*
 * paging_vma_fault()
 *    initializes params for and calls do_fault()
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int paging_vma_fault(struct vm_area_struct * vma, struct vm_fault * vmf) {
  unsigned long fault_address = (unsigned long)vmf->virtual_address
#else
static int paging_vma_fault(struct vm_fault * vmf) {
  struct vm_area_struct * vma = vmf->vma;
  unsigned long fault_address = (unsigned long)vmf->address;
#endif

  return do_fault(vma, fault_address);
}

/*
 * paging_vma_open()
 *    open callback
 *    retreive ptr to data struct and increment
 *    reference counter
 */
static void paging_vma_open(struct vm_area_struct * vma) {
  printk(KERN_INFO "paging_vma_open() invoked\n");
  // TODO retreive ptr to data struct state
  atomic_inc(&state.counter);
}

/*
 * paging_vma_close()
 *    close callback
 *    retreive data struct, decrement reference
 *    counter (if 0, free dynamically alloc mem)
 */
static void paging_vma_close(struct vm_area_struct * vma) {
  printk(KERN_INFO "paging_vma_close() invoked\n");
  if (atomic_dec_and_test(&state.counter)) { // counter now is 0
    // TODO free dynamically allocated mem (__free_page and kfree)
    // if successful: atomic_inc(&free_page);

  }
}

static struct vm_operations_struct paging_vma_ops = {
  .fault = paging_vma_fault,
  .open  = paging_vma_open,
  .close = paging_vma_close
};

/* vma is the new virtual address segment for the process */
static int paging_mmap(struct file * filp, struct vm_area_struct * vma) {
  /* prevent Linux from mucking with our VMA (expanding it, merging it
   * with other VMAs, etc.)
   */
  vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE
    | VM_DONTDUMP | VM_PFNMAP;

  /* setup the vma->vm_ops, so we can catch page faults */
  vma->vm_ops = &paging_vma_ops;

  printk(KERN_INFO "paging_mmap() invoked: new VMA for pid %d from VA 0x%lx to 0x%lx\n",
    current->pid, vma->vm_start, vma->vm_end);

  return 0;
}

static struct file_operations dev_ops = {
  .mmap = paging_mmap,
};

static struct miscdevice dev_handle = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = PAGING_MODULE_NAME,
  .fops = &dev_ops,
};
/*** END device I/O **/

/*** Kernel module initialization and teardown ***/
static int kmod_paging_init(void) {
  int status;
  atomic_set(&state.counter, 0); // init reference counter

  /* Create a character device to communicate with user-space via file I/O operations */
  status = misc_register(&dev_handle);
  if (status != 0) {
    printk(KERN_ERR "Failed to register misc. device for module\n");
    return status;
  }

  printk(KERN_INFO "Loaded kmod_paging module\n");

  return 0;
}

static void kmod_paging_exit(void) {
  /* Deregister our device file */
  misc_deregister(&dev_handle);

  printk(KERN_INFO "Unloaded kmod_paging module\n");
}

module_init(kmod_paging_init);
module_exit(kmod_paging_exit);

/* Misc module info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elaine Cole, Jesse Huang, Zhengliang Liu");
MODULE_DESCRIPTION("Paging Module");
