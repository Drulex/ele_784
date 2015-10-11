/*
 * File         : ele784-lab1.c
 * Description  : ELE784 Lab1 source
 *
 * Etudiants:  JORA09019100 (Alexandru Jora)
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
#include <uapi/asm-generic/fcntl.h>
#include <uapi/asm-generic/errno-base.h>
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
charDriverDev *charStruct;

BufferHandle_t Buffer;

struct semaphore SemReadBuf;
struct semaphore SemWriteBuf;

static int __init charDriver_init(void) {

	// 1) init dev_t: must be declared outside the init
	// 2) class_create
	// 3) device_create
	// 4) cdev_init
	// 5) cdev_add
	// 6) init all sempahores
	// 7) init r/w buffers
	// 8) init circular buffer

	int result;
	charStruct = kmalloc(sizeof(charDriverDev),GFP_KERNEL);
	if(!charStruct)
		printk(KERN_WARNING"===charDriver_init: ERROR in kmalloc (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    result = alloc_chrdev_region(&charStruct->dev, 0, 1, "charDriver");
	if(result < 0)
		printk(KERN_WARNING"===charDriver_init: ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
	else
		printk(KERN_WARNING"===charDriver_init: MAJOR = %u MINOR = %u\n", MAJOR(charStruct->dev), MINOR(charStruct->dev));

	printk(KERN_WARNING"===charDriver_init: Creating charDriver Class\n");
	charDriver_class = class_create(THIS_MODULE, "charDriverClass");

	printk(KERN_WARNING"===charDriver_init: Creating charDriver_Node\n");
	device_create(charDriver_class, NULL, charStruct->dev, NULL, "charDriver_Node");

	printk(KERN_WARNING"===charDriver_init: cdev INIT\n");
	cdev_init(&charStruct->cdev, &charDriver_fops);

	charStruct->cdev.owner = THIS_MODULE;

	if (cdev_add(&charStruct->cdev, charStruct->dev, 1) < 0)
		printk(KERN_WARNING"charDriver_init: ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

	printk(KERN_WARNING "===charDriver_init: initializing semaphores and R/W buffers\n");
	// initialize mutex type semaphore for buffer
	sema_init(&charStruct->SemBuf, 1);
	sema_init(&SemReadBuf, 1);
	sema_init(&SemWriteBuf, 1);

	// init read/write buffers
	charStruct->ReadBuf = kmalloc(READWRITE_BUFSIZE * sizeof(char), GFP_KERNEL);
	charStruct->WriteBuf = kmalloc(READWRITE_BUFSIZE * sizeof(char), GFP_KERNEL);

	// init circular buffer
	Buffer = circularBufferInit(CIRCULAR_BUFFER_SIZE);

	return 0;

}


static void __exit charDriver_exit(void) {

	printk(KERN_WARNING"===charDriver_exit: cdev DELETE\n");
	cdev_del(&charStruct->cdev);
	printk(KERN_WARNING"===charDriver_exit: charDriver_class DEVICE DELETE\n");
	device_destroy(charDriver_class, charStruct->dev);
	printk(KERN_WARNING"===charDriver_exit: charDriver_class DELETE\n");
	class_destroy(charDriver_class);
	printk(KERN_WARNING"===charDriver_exit: charStruct devno DELETE\n");
	unregister_chrdev_region(charStruct->dev, 1);
	printk(KERN_WARNING"===charDriver_exit: charStruct kfree()\n");
	kfree(charStruct);

	if(circularBufferDelete(Buffer))
        printk(KERN_WARNING "===charDriver_exit: Unable to delete circular buffer\n");
	else
        printk(KERN_WARNING "===charDriver_exit: deleting circular buffer\n");
}


static int charDriver_open(struct inode *inode, struct file *flip) {
    printk(KERN_WARNING "===charDriver_open: entering OPEN function\n");
    printk(KERN_WARNING "===charDriver_open: f_flags=%u\n", flip->f_flags);

    // check opening mode
    switch(flip->f_flags & O_ACCMODE){

        case O_RDONLY:
            // read only
            printk(KERN_WARNING "===charDriver_open: opened in O_RDONLY\n");
            charStruct->numReader++;
            break;

        case O_WRONLY:
            // write only
            printk(KERN_WARNING "===charDriver_open: opened in O_WRONLY\n");

            // only open in O_WRONLY if there are no writers already
            if (!charStruct->numWriter){
                down_interruptible(&charStruct->SemBuf);
                charStruct->numWriter++;
            }
            else
                return -ENOTTY;

            break;

        case O_RDWR:
            // read/write
            printk(KERN_WARNING "===charDriver_open: opened in O_RDWR\n");

            // only open in O_RDWR if there are no writers already
            if (!charStruct->numWriter){
                down_interruptible(&charStruct->SemBuf);
                charStruct->numWriter++;
                charStruct->numReader++;
            }
            else
                return -ENOTTY;

            break;

        default:
            printk(KERN_WARNING "===charDriver_open: unknown access mode!!\n");
            break;
    }
    return 0;
}


static int charDriver_release(struct inode *inode, struct file *flip) {
    printk(KERN_WARNING "===charDriver_release: entering RELEASE function\n");

    // check opening mode
    switch(flip->f_flags & O_ACCMODE){

        case O_RDONLY:
            // read only
            charStruct->numReader--;
            break;

        case O_WRONLY:
            // write only
            // release semaphore
            up(&charStruct->SemBuf);
            charStruct->numWriter--;
            break;

        case O_RDWR:
            // read/write
            // release semaphore
            up(&charStruct->SemBuf);
            charStruct->numReader--;
            charStruct->numWriter--;
            break;

        default:
            printk(KERN_WARNING "===charDriver_release:RELEASE unknown access mode!!\n");
            break;
    }
    return 0;
}


static ssize_t charDriver_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops) {
    printk(KERN_WARNING "===charDriver: entering READ function\n");
    return 0;
}


static ssize_t charDriver_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops) {
    printk(KERN_WARNING "===charDriver_write: entering WRITE function\n");
    return 0;
}


static long charDriver_ioctl(struct file *flip, unsigned int cmd, unsigned long arg) {
    printk(KERN_WARNING "===charDriver_ioctl: entering IOCTL function\n");
    return 0;
}
