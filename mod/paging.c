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

 static unsigned int demand_paging = 1; // module param default: default 1 (demand paging)
 module_param(demand_paging, uint, 0644); // module param: enable pre-paging by val 0, else demand

 typedef struct { // linked list wrapper to handle physical mem
     struct page * ptr; // pointer to page
     struct list_head node; // list head
 } wrapper;

typedef struct { // data structure to track what physical mem has been allocated for a process
  atomic_t counter; // atomic reference counter
  struct list_head starter; // head of linked list of pages
} state;

struct page * page; // physical mem page ptr to allocate

state * temp_state_ptr;
wrapper * temp_wrapper_ptr;

atomic_t alloc_page; // increment every time allocate new struct page
atomic_t free_page; // increment every time free struct page

/*
 * my_get_order()
 *    taken from studio 12
 */
static unsigned int my_get_order(unsigned int value) {
    unsigned int shifts = 0;

    if (!value)
        return 0;

    if (!(value & (value - 1)))
        value--;

    while (value > 0) {
        value >>= 1;
        shifts++;
    }

    return shifts;
}

/*
 * do_fault()
 *    allocs new page of physical mem and updates
 *    process' page tables
 */
static int do_fault(struct vm_area_struct * vma, unsigned long fault_address) {
  if (demand_paging == 0) { // pre-paging
    printk(KERN_INFO "paging_vma_fault() invoked: with prepaging");
    return VM_FAULT_SIGBUS;
  } else { // demand paging
    int i;
    state * ptr;
    printk(KERN_INFO "paging_vma_fault() invoked: took a page fault at VA 0x%lx\n", fault_address);

    // alloc page of physical memory
    page = alloc_page(GFP_KERNEL);

    if (!page) { // still uninitialized
      printk(KERN_ERR "do_fault: memory allocation fail\n");
      return VM_FAULT_OOM;
    }

    // update process' page tables to map faulting virtual address to new physical address (page)
    i = remap_pfn_range(vma, PAGE_ALIGN(fault_address), page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
    if (i == 0) { // success page table update
      temp_wrapper_ptr = kmalloc(sizeof(wrapper), GFP_KERNEL);
      //assign temp_wrapper_ptr->ptr to the pointer that refereces page allocated
      temp_wrapper_ptr->ptr = page;
      ptr = (state *) vma->vm_private_data; // retreive ptr to data struct state
      INIT_LIST_HEAD(&(temp_wrapper_ptr->node));
      list_add(&(temp_wrapper_ptr->node), &(ptr->starter)); // add to linked list
      atomic_inc(&alloc_page);
      return VM_FAULT_NOPAGE; // success message
    }
    printk(KERN_ERR "do_fault: failure in updating process' page tables\n");
    return VM_FAULT_SIGBUS;
  }
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
  state * ptr;
  printk(KERN_INFO "paging_vma_open() invoked\n");
  ptr = (state *) vma->vm_private_data; // retreive ptr to data struct state
  atomic_inc(&(ptr->counter)); // inc reference counter
}

/*
 * paging_vma_close()
 *    close callback
 *    retreive data struct, decrement reference
 *    counter (if 0, free dynamically alloc mem)
 */
static void paging_vma_close(struct vm_area_struct * vma) {
  unsigned int order;
  state * ptr;
  wrapper * cursor;
  wrapper * t;
  struct page * pre_ptr;
  printk(KERN_INFO "paging_vma_close() invoked\n");
  if (demand_paging == 0) { // pre-paging
    order = vma->vm_end - vma->vm_start;
    pre_ptr = vma->vm_private_data; // retreive ptr to data struct state
    __free_pages(pre_ptr, my_get_order(order));
  } else { // demand paging
    ptr = (state *) vma->vm_private_data; // retreive ptr to data struct state
    if (atomic_dec_and_test(&(ptr->counter))) { // counter now is 0
      list_for_each_entry_safe(cursor, t, &(ptr->starter), node) {
        // free dynamically allocated mem (__free_page and kfree) for each list entry
         __free_page(cursor->ptr); // free physical mem
         atomic_inc(&free_page);
         list_del(&(cursor->node)); // delete according list node
         kfree(cursor); // free wrapper
       }
    }
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
  unsigned int order;
  int i;
  vma->vm_flags |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_NORESERVE
    | VM_DONTDUMP | VM_PFNMAP;

  /* setup the vma->vm_ops, so we can catch page faults */
  vma->vm_ops = &paging_vma_ops;

  if (demand_paging == 0) { // pre-paging enabled
    // alloc page of physical memory
    order = (vma->vm_end - vma->vm_start) / PAGE_SIZE;
    page = alloc_pages(GFP_KERNEL, my_get_order(order));

	printk("foooo order:%u", order);
	printk("mygetorder: %u vm_end: %lu vm_start: %lu", my_get_order(order), vma->vm_end, vma->vm_start);
    if (!page) { // still uninitialized
      printk(KERN_ERR "paging_mmap(): memory allocation fail in pre-paging\n");
      return -ENOMEM;
    }
    vma->vm_private_data = page; // pass in pointer

    i = remap_pfn_range(vma, PAGE_ALIGN(vma->vm_start), page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
    if (i == 0) { // success page table update
    } else {
      return -EFAULT;
    }
  } else { // demand paging
    // init state struct:
    temp_state_ptr = kmalloc(sizeof(state), GFP_KERNEL);
    atomic_set(&(temp_state_ptr->counter), 1); // init reference counter
    INIT_LIST_HEAD(&(temp_state_ptr->starter)); // init list
    vma->vm_private_data = temp_state_ptr; // store state
  }

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
  printk(KERN_INFO "Pages allocated %d times, freed %d times\n", atomic_read(&alloc_page), atomic_read(&free_page));

  printk(KERN_INFO "Unloaded kmod_paging module\n");
}

module_init(kmod_paging_init);
module_exit(kmod_paging_exit);

/* Misc module info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elaine Cole, Jesse Huang, Zhengliang Liu");
MODULE_DESCRIPTION("Paging Module");
