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

static int do_fault(struct vm_area_struct * vma, unsigned long fault_address) {
  printk(KERN_INFO "paging_vma_fault() invoked: took a page fault at VA 0x%lx\n", fault_address);
  return VM_FAULT_SIGBUS;
}

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

static void paging_vma_open(struct vm_area_struct * vma) {
  printk(KERN_INFO "paging_vma_open() invoked\n");
}

static void paging_vma_close(struct vm_area_struct * vma) {
  printk(KERN_INFO "paging_vma_close() invoked\n");
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
