/*
 * File         : ele784-lab1.c
 * Description  : ELE784 Lab1 source
 *
 * Etudiants:  XXXX00000000 (prenom nom #1)
 *             XXXX00000000 (prenom nom #2)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include "circularBuffer.h"

// Configuration / Defines
#define READWRITE_BUFSIZE 16
#define CIRCULAR_BUFFER_SIZE 256

// Driver internal management struct
typedef struct {
    char *ReadBuf;
    char *WriteBuf;
    struct semaphore SemBuf;
    unsigned short numWriter;
    unsigned short numReader;
    dev_t dev;
    struct cdev cdev;
} charDriverDev;

// Module Information
MODULE_AUTHOR("JORA Alexandru, MUKANDILA, Mukandila");
MODULE_LICENSE("Dual BSD/GPL");

// Prototypes
static int __init charDriver_init(void);
static void __exit charDriver_exit(void);
static int charDriver_open(struct inode *inode, struct file *flip);
static int charDriver_release(struct inode *inode, struct file *flip);
static ssize_t charDriver_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops);
static ssize_t charDriver_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops);
static long charDriver_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

// Driver handled file operations
static struct file_operations charDriver_fops = {
    .owner = THIS_MODULE,
    .open = charDriver_open,
    .release = charDriver_release,
    .read = charDriver_read,
    .write = charDriver_write,
    .unlocked_ioctl = charDriver_ioctl,
};

// Init and Exit functions
module_init(charDriver_init);
module_exit(charDriver_exit);

struct class *charDriver_class;
// shouldn't we use struct cdev within struct charStruct?
//struct cdev charDriver_cdev;
charDriverDev *charStruct;

static int __init charDriver_init(void) {

	// 1) init dev_t: must be declared outside the init
	// 2) class_create
	// 3) device_create
	// 4) cdev_init
	// 5) cdev_add
    // 6) init all sempahores
    // 7) init r/w buffers

	charStruct = kmalloc(sizeof(charDriverDev),GFP_KERNEL);
	if(!charStruct)
		printk(KERN_WARNING"===charDriver ERROR in kmalloc (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

	int result = alloc_chrdev_region(&charStruct->dev, 0, 1, "charDriver");
	if(result < 0)
		printk(KERN_WARNING"===charDriver ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
	else
		printk(KERN_WARNING"===charDriver : MAJOR = %u MINOR = %u\n", MAJOR(charStruct->dev), MINOR(charStruct->dev));

	printk(KERN_WARNING"===charDriver: Creating charDriver Class\n");
	charDriver_class = class_create(THIS_MODULE, "charDriverClass");

	printk(KERN_WARNING"===charDriver: Creating charDriver_Node\n");
	device_create(charDriver_class, NULL, charStruct->dev, NULL, "charDriver_Node");

	printk(KERN_WARNING"===charDriver: cdev INIT\n");
	cdev_init(&charStruct->cdev, &charDriver_fops);

	charStruct->cdev.owner = THIS_MODULE;

	if (cdev_add(&charStruct->cdev, charStruct->dev, 1) < 0)
		printk(KERN_WARNING"charDriver ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    printk(KERN_WARNING "===charDriver initializing semaphores and R/W buffers\n");
    // initialize mutex type semaphore for buffer
    sema_init(&charStruct->SemBuf, 1);

    // init read/write buffers
    charStruct->ReadBuf = kmalloc(READWRITE_BUFSIZE, GFP_KERNEL);
    charStruct->WriteBuf = kmalloc(READWRITE_BUFSIZE, GFP_KERNEL);

	return 0;

}


static void __exit charDriver_exit(void) {

	printk(KERN_WARNING"===charDriver: cdev DELETE\n");
	cdev_del(&charStruct->cdev);
	printk(KERN_WARNING"==charDriver: charDriver_class DEVICE DELETE\n");
	device_destroy(charDriver_class, charStruct->dev);
	printk(KERN_WARNING"===charDriver: charDriver_class DELETE\n");
	class_destroy(charDriver_class);
	printk(KERN_WARNING"===charDriver: charStruct devno DELETE\n");
	unregister_chrdev_region(charStruct->dev, 1);
	printk(KERN_WARNING"===charDriver: charStruct kfree()\n");
	kfree(charStruct);
}


static int charDriver_open(struct inode *inode, struct file *flip) {
    printk(KERN_WARNING "===charDriver: entering OPEN function\n");

    // check opening mode
    switch(flip->f_flags){

        case O_RDONLY:
            // read only

        case O_WRONLY:
            // write only

            // capture semaphore
            if (down_interruptible(&charStruct->SemBuf)){
                return -ENOTTY;
            }

            // critical region


            // release semaphore
            up(&charStruct->SemBuf);


        case O_RDWR:
            // read/write

            // capture semaphore
            if (down_interruptible(&charStruct->SemBuf)){
                return -ENOTTY;
            }

            // critical region


            // release semaphore
            up(&charStruct->SemBuf);
    }
    return 0;
}


static int charDriver_release(struct inode *inode, struct file *flip) {
    printk(KERN_WARNING "===charDriver: entering RELEASE function\n");
    return 0;
}


static ssize_t charDriver_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops) {
    printk(KERN_WARNING "===charDriver: entering READ function\n");
    return 0;
}


static ssize_t charDriver_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops) {
    printk(KERN_WARNING "===charDriver: entering WRITE function\n");
    return 0;
}


static long charDriver_ioctl(struct file *flip, unsigned int cmd, unsigned long arg) {
    printk(KERN_WARNING "===charDriver: entering IOCTL function\n");
    return 0;
}
