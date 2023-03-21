#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/kprobes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/fs_struct.h>
#include <asm/tlbflush.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/hugetlb.h>
#include <linux/vmalloc.h>

#include "btplus.h"

static int major;
atomic_t device_opened;
static struct class *demo_class;
struct device *demo_device;

struct kobject *cs614_kobject;
unsigned promote = 0;

static ssize_t sysfs_show(struct kobject *kobj,
						  struct kobj_attribute *attr, char *buf);
static ssize_t sysfs_store(struct kobject *kobj,
						   struct kobj_attribute *attr, const char *buf, size_t count);

struct kobj_attribute sysfs_attr;

struct address
{
	unsigned long from_addr;
	unsigned long to_addr;
};

struct input
{
	unsigned long addr;
	unsigned length;
	struct address *buff;
};

unsigned long (*magic)(const char *name);


unsigned long lookup_kallsyms_lookup_name(void) {
    struct kprobe kp;
    unsigned long addr;
    
    memset(&kp, 0, sizeof(struct kprobe));
    kp.symbol_name = "kallsyms_lookup_name";
    if (register_kprobe(&kp) < 0) {
        return 0;
    }
    addr = (unsigned long)kp.addr;
    unregister_kprobe(&kp);
    return addr;
}

static int device_open(struct inode *inode, struct file *file)
{
	atomic_inc(&device_opened);
	try_module_get(THIS_MODULE);
	printk(KERN_INFO "Device opened successfully\n");
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	atomic_dec(&device_opened);
	module_put(THIS_MODULE);
	printk(KERN_INFO "Device closed successfully\n");

	return 0;
}

static ssize_t device_read(struct file *filp,
						   char *buffer,
						   size_t length,
						   loff_t *offset)
{
	printk("read called\n");
	return 0;
}

static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{

	printk("write called\n");
	return 8;
}

unsigned long (*my_move_page_tables) (struct vm_area_struct * ovma, unsigned long old_addr,  struct vm_area_struct * nvma, unsigned long to_addr, unsigned long length, bool val);

int (*my_do_munmap) (struct mm_struct *, unsigned long, size_t, struct list_head *uf);

struct vm_area_struct*  (*my_copy_vma) (struct vm_area_struct **vmap, unsigned long addr, unsigned long len, pgoff_t pgoff, bool* need_rmap_locks);

int my_mremap(struct vm_area_struct * ovma, unsigned long to_addr){
	int ret = 0;
	unsigned long length = 0, moved_len;
	struct vm_area_struct * nvma;
	bool need_rmap_locks;
	LIST_HEAD(uf_unmap);

	length = (ovma->vm_end - ovma->vm_start);
	nvma = (*my_copy_vma)(&ovma, to_addr, length, ovma->vm_pgoff, &need_rmap_locks);
	if (!nvma) {
		return -1;
	}
	moved_len = (*my_move_page_tables)(ovma, ovma->vm_start, nvma, to_addr, length, false);
	printk(KERN_INFO "Moved page tables with moved_len = %ld and length = %ld \n", moved_len, length);
	if(moved_len != length){
		return -1;
	}
	else if(ovma->vm_ops && ovma->vm_ops->mremap){
		ovma->vm_ops->mremap(nvma);
	}
	(*my_do_munmap)(ovma->vm_mm, ovma->vm_start, length, &uf_unmap);
	return ret;
}

static struct task_struct * kthread_task;

unsigned long HUGE_PAGE_SIZE = (1ULL<<21);
unsigned long HUGE_PAGE_SHIFT = 21;

static void do_promote(struct mm_struct *mm){
	struct vm_area_struct * vma;
	unsigned long sz, addr;
	VMA_ITERATOR(vmi, mm, 0);

	mmap_write_lock(mm);
	for_each_vma(vmi, vma){
		sz = vma->vm_end - vma->vm_start;
		printk(KERN_INFO "START: %lx, END: %lx DIFF:%lx\n", vma->vm_start, vma->vm_end, sz);
		if(sz < HUGE_PAGE_SIZE){
			continue;
		}
		if (vma->vm_flags & VM_HUGEPAGE) {
            printk("Huge page already exists for this vma. Skipping this vma.\n");
			continue;
		}
		for(addr = vma->vm_start; addr < vma->vm_end; addr += HUGE_PAGE_SIZE){

			struct page *page = NULL;
            unsigned long pfn;
            struct page *new_page = NULL;
            unsigned long new_pfn;

			if(addr + HUGE_PAGE_SIZE > vma->vm_end){
				break;
			}

            // Allocate a 2MB page
            page = alloc_pages(GFP_KERNEL | __GFP_ZERO, HPAGE_SHIFT - PAGE_SHIFT);
            if (!page) {
                printk(KERN_ERR "Failed to allocate a 2MB page\n");
                continue;
            }

            // Get the physical page frame number (pfn) of the allocated page
            pfn = page_to_pfn(page);

            // Copy contents from original page frame to the new page frame
            new_page = pfn_to_page(pfn);
            memcpy((void *)page_address(new_page), (void *)addr, HUGE_PAGE_SIZE);

            // Remap current vma address to new physical page range
            new_pfn = pfn + ((addr - vma->vm_start) >> HUGE_PAGE_SHIFT);
            remap_pfn_range(vma, addr, new_pfn, HUGE_PAGE_SIZE, vma->vm_page_prot);
			vma->vm_flags |= VM_HUGEPAGE;
		}
	}
	flush_tlb_range(vma, vma->vm_start, vma->vm_end);
	mmap_write_unlock(mm);
}

static int promote_pages(void * data){
	struct task_struct * tsk = current;
	
	while(!kthread_should_stop()){
		if(promote){
			printk(KERN_INFO "Promoting pages\n");
			do_promote(tsk->mm);
			promote = 0;
		}
		msleep(1000);
	}
	return 0;
}

long device_ioctl(struct file *file,
				  unsigned int ioctl_num,
				  unsigned long ioctl_param)
{
	int ret = 0; // on failure return -1
	struct address *buff = NULL;
	unsigned long vma_addr = 0;
	unsigned long to_addr = 0;
	unsigned length = 0, delta = 0;
	struct input *ip;
	unsigned index = 0;
	struct address temp;

	struct vm_area_struct *vma, *ovma = NULL;
	struct mm_struct *mm = current->mm;
	VMA_ITERATOR(vmi, mm, 0);

	/*
	 * Switch according to the ioctl called
	 */
	switch (ioctl_num){

	case IOCTL_MVE_VMA_TO:
		buff = (struct address *)vmalloc(sizeof(struct address));
		printk("move VMA at a given address");
		if (copy_from_user(buff, (char *)ioctl_param, sizeof(struct address)))
		{
			pr_err("MVE_VMA address write error\n");
			return -1;
		}
		vma_addr = buff->from_addr;
		to_addr = buff->to_addr;
		vfree(buff);
		// printk("address from :%lx, to:%lx \n", vma_addr, to_addr);
		mmap_read_lock(mm);
		for_each_vma(vmi, vma)
		{
			// printk(KERN_INFO "START: %lx, END: %lx\n", vma->vm_start, vma->vm_end);
			if (vma->vm_start <= vma_addr && vma_addr < vma->vm_end)
			{
				ovma = vma;
				to_addr -= vma_addr - ovma->vm_start;
				// printk("Found here\n");
			}
		}
		mmap_read_unlock(mm);
		if(ovma == NULL){
			return -1;
		}
		mmap_write_lock(mm);
		ret = my_mremap(ovma, to_addr);
		mmap_write_unlock(mm);
		return ret;

	case IOCTL_MVE_VMA:
		buff = (struct address *)vmalloc(sizeof(struct address));
		printk("move VMA to available hole address");
		if (copy_from_user(buff, (char *)ioctl_param, sizeof(struct address)))
		{
			pr_err("MVE_VMA address write error\n");
			return -1;
		}
		vma_addr = buff->from_addr;
		printk("VMA address :%lx \n", vma_addr);

		// find old VMA
		mmap_read_lock(mm);
		for_each_vma(vmi, vma)
		{
			printk( "START: %lx, END: %lx\n", vma->vm_start, vma->vm_end);
			if (vma->vm_start <= vma_addr && vma_addr < vma->vm_end) {
				ovma = vma;
			}
		}
		mmap_read_unlock(mm);
		if(ovma == NULL){
			vfree(buff);
			return -1;
		}

		// Find to_Addr

		to_addr =  get_unmapped_area(ovma->vm_file, 0, (ovma->vm_end - ovma->vm_start), ovma->vm_pgoff, 0);
		if(IS_ERR_VALUE(to_addr)){
			vfree(buff);
			return -1;
		}
		else{
			delta = vma_addr - ovma->vm_start;
			buff->to_addr = to_addr + delta;
			if (copy_to_user((char *)ioctl_param, buff, sizeof(struct address)))
			{
				pr_err("MVE_VMA new address write error\n");
				return -1;
			}
			vfree(buff);
		}
		printk(KERN_INFO "Got new addr as %lx\n", to_addr);

		// Remap VMA
		mmap_write_lock(mm);
		ret = my_mremap(ovma, to_addr);
		mmap_write_unlock(mm);
		return ret;
		
	case IOCTL_PROMOTE_VMA:
		// Create kthread
		kthread_task = kthread_create(promote_pages, NULL, "promote-pages");
		if(IS_ERR(kthread_task)){
			printk(KERN_ALERT "Couldn't create kernel thread!\n");
			return -EINVAL;
		}

		wake_up_process(kthread_task);
		printk("promoting 4KB pages to 2mb\n");
		return ret;
	case IOCTL_COMPACT_VMA:
		printk("compact VMA\n");
		ip = (struct input *)vmalloc(sizeof(struct input));
		if (copy_from_user(ip, (char *)ioctl_param, sizeof(struct input)))
		{
			pr_err("MVE_MERG_VMA address write error\n");
			return ret;
		}
		vma_addr = ip->addr;
		length = ip->length;
		buff = ip->buff;
		temp.from_addr = vma_addr;
		temp.to_addr = vma_addr;
		printk("vma address:%lx, length:%u, buff:%lx\n", vma_addr, length, (unsigned long)buff);
		// populate old to new address mapping in user buffer.
		// number of entries in this buffer is equal to the number of
		// virtual pages in vma address range
		// index of moved addr in mapping table is , index = (addr-vma_address)>>12
		index = (vma_addr - vma_addr) >> 12;
		if (copy_to_user((struct address *)buff + index, &temp, sizeof(struct address)))
		{
			pr_err("COMPACT VMA read error\n");
			return ret;
		}
		vfree(ip);
		return ret;
	}
	return ret;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

static char *demo_devnode(struct device *dev, umode_t *mode)
{
	if (mode && dev->devt == MKDEV(major, 0))
		*mode = 0666;
	return NULL;
}


// Implement required logic
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
						  char *buf)
{
	pr_info("sysfs read\n");
	return sprintf(buf, "%d", promote);
}

// Implement required logic
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
						   const char *buf, size_t count)
{
	int z;
	printk("sysfs write\n");
	sscanf(buf, "%d", &z);
	promote = z;
	return count;
}

static void (*orig_munmap)(struct vm_area_struct *vma, unsigned long addr, size_t len);

static int break_at(struct vm_area_struct * vma, unsigned long addr){
	unsigned long page_size = PAGE_SIZE;
	void * new_page_addr;
	int err;
	struct page* new_page;
	unsigned long page_start, page_end;

	for(int i = 0; i<512; i++){
		page_start = addr + i * page_size;
        page_end = page_start + page_size;

        // Allocate a new 4KB page
        *new_page = alloc_page_vma(GFP_HIGHUSER, vma, page_start);
        if (!new_page) {
			return -1;
            // Failed to allocate page
            break;
        }

        // Copy data from the huge page to the new page
        new_page_addr = page_address(new_page);
        memcpy(new_page_addr, (void * ) addr, page_size);

        // Remap the original VMA to the new page
        err = remap_pfn_range(vma, page_start, page_to_pfn(new_page), page_size, vma->vm_page_prot);
        if (err) {
			printk(KERN_ALERT "Failed to remap page\n");
            // Failed to remap page
			return -1;
        }

        // Flush the TLB cache for the remapped page range
        flush_tlb_range(vma, page_start, page_end);
	}
	flush_tlb_range(vma, addr, addr + page_size);
	
	return 0;
}

static void my_munmap(struct vm_area_struct *vma, unsigned long addr, size_t len)
{
    /* Do your own processing here */
	if(vma->vm_flags & VM_HUGEPAGE){
		printk("Huge page unmapping function called\n");
		if((addr - vma->vm_start) % HUGE_PAGE_SIZE){
			if(break_at(vma, addr - (addr - vma->vm_start) % HUGE_PAGE_SIZE)){
				printk(KERN_ALERT "Failed to break at %lx\n", addr - (addr - vma->vm_start) % HUGE_PAGE_SIZE);
			}
		}
		if((addr + len - vma->vm_start) % HUGE_PAGE_SIZE){
			if(break_at(vma, addr + len - (addr + len - vma->vm_start) % HUGE_PAGE_SIZE)){
				printk(KERN_ALERT "Failed to break at %lx\n", addr + len - (addr + len - vma->vm_start) % HUGE_PAGE_SIZE);
			}
		}
	}

    orig_munmap(vma, addr, len);
}


void munmap_hook(void){
	unsigned long munmap_addr = magic("sys_munmap");
    int ret;

    /* Initialize the ftrace infrastructure */
    ret = register_ftrace_function(&global_ftrace_ops, NULL, 0);
    if (ret) {
        pr_err("Failed to register ftrace function (%d)\n", ret);
        return ret;
    }

    /* Set the filter on the munmap function address */
    ftrace_set_filter_ip(&global_ftrace, munmap_addr, 0, 0);

    /* Set the ftrace filter to my_munmap */
    ret = ftrace_set_ftrace_filter(&global_ftrace, my_munmap, NULL);
    if (ret) {
        pr_err("Failed to set ftrace filter (%d)\n", ret);
        return ret;
    }

    /* Get the original munmap function address */
    orig_munmap = (void (*)(struct vm_area_struct *, unsigned long, size_t)) magic("sys_munmap");
}

int init_module(void)
{
	int err;
	magic = (unsigned long (*)(const char *))lookup_kallsyms_lookup_name();
	if(!magic){
		printk(KERN_ALERT "Couldn't get address for kallsyms_lookup_name!\n");
		return -EINVAL;
	}
	my_copy_vma = (struct vm_area_struct* (*)(struct vm_area_struct **, unsigned long , unsigned long , pgoff_t , bool *))magic("copy_vma");
	my_move_page_tables = (unsigned long (*) (struct vm_area_struct * , unsigned long ,  struct vm_area_struct * , unsigned long , unsigned long , bool))magic("move_page_tables");
	my_do_munmap = (int (*) (struct mm_struct *, unsigned long, size_t, struct list_head *uf))magic("do_munmap");
	
	if(!my_copy_vma || !my_move_page_tables || !my_do_munmap){
		printk(KERN_ALERT "Couldn't get the addresses for functions\n");
		return -EINVAL;
	}
	kthread_task = NULL;
	munmap_hook();
	printk(KERN_INFO "Hello kernel\n");
	major = register_chrdev(0, DEVNAME, &fops);
	err = major;
	if (err < 0)
	{
		printk(KERN_ALERT "Registering char device failed with %d\n", major);
		goto error_regdev;
	}

	demo_class = class_create(THIS_MODULE, DEVNAME);
	err = PTR_ERR(demo_class);
	if (IS_ERR(demo_class))
		goto error_class;

	demo_class->devnode = demo_devnode;

	demo_device = device_create(demo_class, NULL,
								MKDEV(major, 0),
								NULL, DEVNAME);
	err = PTR_ERR(demo_device);
	if (IS_ERR(demo_device))
		goto error_device;

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
	atomic_set(&device_opened, 0);

	cs614_kobject = kobject_create_and_add("kobject_cs614", kernel_kobj);

	if (!cs614_kobject)
		return -ENOMEM;

	sysfs_attr.attr.name = "promote";
	sysfs_attr.attr.mode = 0666;
	sysfs_attr.show = sysfs_show;
	sysfs_attr.store = sysfs_store;

	err = sysfs_create_file(cs614_kobject, &(sysfs_attr.attr));
	if (err)
	{
		pr_info("sysfs exists:");
		goto r_sysfs;
	}
	return 0;
r_sysfs:
	kobject_put(cs614_kobject);
	sysfs_remove_file(kernel_kobj, &sysfs_attr.attr);
error_device:
	class_destroy(demo_class);
error_class:
	unregister_chrdev(major, DEVNAME);
error_regdev:
	return err;
}

void cleanup_module(void)
{
	if(kthread_task != NULL){
		kthread_stop(kthread_task);
	}
    unregister_ftrace_function(&global_ftrace_ops, NULL);
	device_destroy(demo_class, MKDEV(major, 0));
	class_destroy(demo_class);
	unregister_chrdev(major, DEVNAME);
	kobject_put(cs614_kobject);
	sysfs_remove_file(kernel_kobj, &sysfs_attr.attr);
	printk(KERN_INFO "Goodbye kernel\n");
}

MODULE_AUTHOR("cs614");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("assignment2");