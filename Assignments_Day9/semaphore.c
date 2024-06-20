#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/semaphore.h>

static int dchar_open(struct inode *, struct file *);
static int dchar_close(struct inode *, struct file *);
static ssize_t dchar_read(struct file *, char *, size_t, loff_t *);
static ssize_t dchar_write(struct file *, const char *, size_t, loff_t *);

#define MAX 32
static struct kfifo buf;
static int major;
static dev_t devno;
static struct class *pclass;
static struct cdev cdev;
static struct semaphore s;

static struct file_operations dchar_fops = {
    .owner = THIS_MODULE,
    .open = dchar_open,
    .release = dchar_close,
    .read = dchar_read,
    .write = dchar_write
};

static __init int dchar_init(void) {
    int ret, minor;
    struct device *pdevice;
    
    printk(KERN_INFO "%s: dchar_init() called.\n", THIS_MODULE->name);
    ret = kfifo_alloc(&buf, MAX, GFP_KERNEL);
    if(ret != 0) {
        printk(KERN_ERR "%s: kfifo_alloc() failed.\n", THIS_MODULE->name);
        goto kfifo_alloc_failed;
    }
    printk(KERN_ERR "%s: kfifo_alloc() successfully created device.\n", THIS_MODULE->name);

    ret = alloc_chrdev_region(&devno, 0, 1, "dchar");
    if(ret != 0) {
        printk(KERN_ERR "%s: alloc_chrdev_region() failed.\n", THIS_MODULE->name);
        goto alloc_chrdev_region_failed;
    }
    major = MAJOR(devno);
    minor = MINOR(devno);
    printk(KERN_INFO "%s: alloc_chrdev_region() allocated device number %d/%d.\n", THIS_MODULE->name, major, minor);

    pclass = class_create("dchar_class");
    if(IS_ERR(pclass)) {
        printk(KERN_ERR "%s: class_create() failed.\n", THIS_MODULE->name);
        ret = -1;
        goto class_create_failed;
    }
    printk(KERN_INFO "%s: class_create() created device class.\n", THIS_MODULE->name);

    pdevice = device_create(pclass, NULL, devno, NULL, "dchar%d", 0);
    if(IS_ERR(pdevice)) {
        printk(KERN_ERR "%s: device_create() failed.\n", THIS_MODULE->name);
        ret = -1;
        goto device_create_failed;
    }
    printk(KERN_INFO "%s: device_create() created device file.\n", THIS_MODULE->name);

    cdev_init(&cdev, &dchar_fops);
    ret = cdev_add(&cdev, devno, 1);  
    if(ret != 0) {

        printk(KERN_ERR "%s: cdev_add() failed to add cdev in kernel db.\n", THIS_MODULE->name);
        goto cdev_add_failed;
    }
    printk(KERN_INFO "%s: cdev_add() added device in kernel db.\n", THIS_MODULE->name);

    sema_init(&s,1);
    printk(KERN_INFO "%s: sema_init() is done.\n", THIS_MODULE->name);

    return 0;
cdev_add_failed:
    device_destroy(pclass, devno);
device_create_failed:
    class_destroy(pclass);
class_create_failed:
    unregister_chrdev_region(devno, 1);
alloc_chrdev_region_failed:
    kfifo_free(&buf);
kfifo_alloc_failed:
    return ret;
}

static __exit void dchar_exit(void) {
    printk(KERN_INFO "%s: dchar_exit() called.\n", THIS_MODULE->name);
    cdev_del(&cdev);
    printk(KERN_INFO "%s: cdev_del() removed device from kernel db.\n", THIS_MODULE->name);
    device_destroy(pclass, devno);
    printk(KERN_INFO "%s: device_destroy() destroyed device file.\n", THIS_MODULE->name);
    class_destroy(pclass);
    printk(KERN_INFO "%s: class_destroy() destroyed device class.\n", THIS_MODULE->name);
    unregister_chrdev_region(devno, 1);
    printk(KERN_INFO "%s: unregister_chrdev_region() released device number.\n", THIS_MODULE->name);
    kfifo_free(&buf);
    printk(KERN_INFO "%s: kfifo_free() destroyed device.\n", THIS_MODULE->name);
}


static int dchar_open(struct inode *pinode, struct file *pfile) {
    printk(KERN_INFO "%s: dchar_open() called.\n", THIS_MODULE->name);
    down(&s);
    return 0;
}

static int dchar_close(struct inode *pinode, struct file *pfile) {
    printk(KERN_INFO "%s: dchar_close() called.\n", THIS_MODULE->name);
    up(&s);
    return 0;
}

static ssize_t dchar_read(struct file *pfile, char *ubuf, size_t size, loff_t *poffset) {
    int nbytes, ret;
    printk(KERN_INFO "%s: dchar_read() called.\n", THIS_MODULE->name);
    ret = kfifo_to_user(&buf, ubuf, size, &nbytes);
    if(ret < 0) {
        printk(KERN_ERR "%s: dchar_read() failed to copy data from kernel space using kfifo_to_user().\n", THIS_MODULE->name);
        return ret;     
    }
    printk(KERN_INFO "%s: dchar_read() copied %d bytes to user space.\n", THIS_MODULE->name, nbytes);
    return nbytes;
}

static ssize_t dchar_write(struct file *pfile, const char *ubuf, size_t size, loff_t *poffset) {
    int nbytes, ret;
    printk(KERN_INFO "%s: dchar_write() called.\n", THIS_MODULE->name);
    

    ret = kfifo_from_user(&buf, ubuf, size, &nbytes);
    if(ret < 0) {
        printk(KERN_ERR "%s: dchar_write() failed to copy data in kernel space using kfifo_from_user().\n", THIS_MODULE->name);
        return ret;     
    }
    printk(KERN_INFO "%s: dchar_write() copied %d bytes from user space.\n", THIS_MODULE->name, nbytes);
    return nbytes;
}

module_init(dchar_init);
module_exit(dchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nilesh Ghule <nilesh@sunbeaminfo.com>");
MODULE_DESCRIPTION("Simple dchar driver with kfifo as device.");
