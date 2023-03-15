#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/mm.h>
#include<linux/mm_types.h>
#include<linux/file.h>
#include<linux/fs.h>
#include<linux/path.h>
#include<linux/slab.h>
#include<linux/dcache.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs_struct.h>
#include <asm/tlbflush.h>
#include<linux/uaccess.h>
#include<linux/device.h>

//values to read
#define PID 		0
#define	STATIC_PRIO 	1
#define	COMM 		2
#define PPID		3
#define NVCSW		4



static int curid;

// static struct kobject *cs614_kobj;

static int resolve(char* buf){
        int id;
        struct task_struct *tsk;
        id = curid;
        tsk = current;
        if(id < 0 || id > 4){
                return sprintf(buf, "%d", -EINVAL);
        }
        if(id == PID){
                printk(KERN_INFO "Requested for PID: %d\n", tsk->pid);
                return sprintf(buf, "%d", tsk->pid);
        }
        else if(id == STATIC_PRIO){
                printk(KERN_INFO "Requested for PRIORITY: %d\n", tsk->static_prio);
                return sprintf(buf, "%d", tsk->static_prio);
        }
        else if(id == COMM){
                printk(KERN_INFO "Requested for COMM: %s\n", tsk->comm);
                return sprintf(buf, "%s", tsk->comm);
        }
        else if(id == PPID){
                printk(KERN_INFO "Requested for PPID: %d\n", tsk->parent->pid);
                return sprintf(buf, "%d", tsk->parent->pid);
        }
        else if(id == NVCSW){
                printk(KERN_INFO "Requested for NVCSW: %lu\n", tsk->nvcsw);
                return sprintf(buf, "%lu", tsk->nvcsw);
        }
        return 0;
}

static ssize_t cs614sys_status(struct kobject *kobj,
                                  struct kobj_attribute *attr, char *buf)
{
        printk(KERN_INFO "In status func(): Can't read from status\n");
        return 0;
}

static ssize_t cs614sys_set(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
        int newval, err;
        printk(KERN_INFO "In set func()\n");
        err = kstrtoint(buf, 10, &newval);
        if (err || newval < 0 || newval > 4 )
                return -EINVAL;
        curid = newval;
        return count;
}

static struct kobj_attribute cs614sys_attribute = __ATTR(cs614_value, 0660, cs614sys_status, cs614sys_set);
static struct attribute *cs614sys_attrs[] = {
        &cs614sys_attribute.attr,
        NULL,
};
static struct attribute_group cs614sys_attr_group = {
        .attrs = cs614sys_attrs,
        .name = "cs614_sysfs",
};


#define DEVNAME "cs614_device"

static int major;
atomic_t  device_opened;
static struct class *demo_class;
struct device *demo_device;

static char *d_buf = NULL;


static int demo_open(struct inode *inode, struct file *file)
{
        atomic_inc(&device_opened);
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Device opened successfully\n");
        return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
        atomic_dec(&device_opened);
        module_put(THIS_MODULE);
        printk(KERN_INFO "Device closed successfully\n");
        return 0;
}

static ssize_t demo_read(struct file *filp,
                           char *ubuf,
                           size_t length,
                           loff_t * offset)
{ 
        int ret;
        printk(KERN_INFO "In read\n");
	if(length > 16)
		return -EINVAL;
        ret = resolve(d_buf);
	if(copy_to_user(ubuf, d_buf, ret))
		return -EINVAL;
        return ret;
}

static ssize_t
demo_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
            printk(KERN_INFO "In write: Not Allowed\n");
            return 0;
}

static struct file_operations fops = {
        .read = demo_read,
        .write = demo_write,
        .open = demo_open,
        .release = demo_release,
};

static char *demo_devnode(struct device *dev, umode_t *mode)
{
        if (mode && dev->devt == MKDEV(major, 0))
                *mode = 0666;
        return NULL;
}

int init_sysfs(void){

        int ret;

        // cs614_kobj = kobject_create_and_add("cs614_sysfs", kernel_kobj);
	// if (!cs614_kobj)
	// 	return -ENOMEM;

        ret = sysfs_create_group (kernel_kobj, &cs614sys_attr_group);
        if(unlikely(ret)){
                printk(KERN_INFO "cs614: can't create sysfs\n");
                return -1;
        }

	printk(KERN_INFO "All set to play\n");
	return 0;
}

int init_module(void)
{
        int err;
	printk(KERN_INFO "Hello kernel\n");

        if(init_sysfs() < 0){
                return -1;
        }
            
        major = register_chrdev(0, DEVNAME, &fops);
        err = major;
        if (err < 0) {      
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
 
        d_buf = kzalloc(4096, GFP_KERNEL);
        printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);                                                              
        atomic_set(&device_opened, 0);
       

	return 0;

error_device:
         class_destroy(demo_class);
error_class:
        unregister_chrdev(major, DEVNAME);
error_regdev:
        return  err;
}

void cleanup_sysfs(void){
        sysfs_remove_group (kernel_kobj, &cs614sys_attr_group);
        printk(KERN_INFO "Removed the sysfs group\n");   
}

void cleanup_module(void)
{
        cleanup_sysfs();
	kfree(d_buf);
        device_destroy(demo_class, MKDEV(major, 0));
        class_destroy(demo_class);
        unregister_chrdev(major, DEVNAME);
	printk(KERN_INFO "Goodbye kernel\n");
}
 
MODULE_LICENSE("GPL");
