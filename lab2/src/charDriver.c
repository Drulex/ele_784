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
MODULE_AUTHOR("prenom nom #1, prenom nom #2");
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

//dev_t devno;
struct class *charDriver_class;
struct cdev charDriver_cdev;



static int __init charDriver_init(void) {

	// 1) init dev_t: must be declared outside the init
	// 2) class_create
	// 3) device_create
	// 4) cdev_init
	// 5) cdev_add

	charDriverDev *charStruct = kmalloc(sizeof(charDriverDev),GFP_KERNEL);
	//DEFINE_SEMAPHORE(&charStruct->SemBuf);

	int result;

	result = alloc_chrdev_region(&charStruct->dev, 0, 1, "charDriver");
	if(result < 0)
		printk(KERN_WARNING"charDriver ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
	else
		printk(KERN_WARNING"charDriver : MAJOR = %u MINOR = %u\n", MAJOR(charStruct->dev), MINOR(charStruct->dev));

	charDriver_class = class_create(THIS_MODULE, "charDriverClass");

	device_create(charDriver_class, NULL, charStruct->dev, NULL, "HelloDev_Node");
	cdev_init(&charDriver_cdev, &charDriver_fops);

	charDriver_cdev.owner = THIS_MODULE;

	if (cdev_add(&charDriver_cdev, charStruct->dev, 1) < 0)
		printk(KERN_WARNING"charDriver ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

	return 0;


    printk(KERN_ERR     "This is a kernel error message...\n");
    printk(KERN_WARNING "This is a kernel warning message...\n");
    printk(KERN_NOTICE  "This is a kernel notice message...\n");
    printk(KERN_INFO    "This is a kernel info message...\n");
    printk(KERN_DEBUG   "This is a kernel debug message...\n");

    return 0;
}


static void __exit charDriver_exit(void) {

}


static int charDriver_open(struct inode *inode, struct file *flip) {
    return 0;
}


static int charDriver_release(struct inode *inode, struct file *flip) {
    return 0;
}


static ssize_t charDriver_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}


static ssize_t charDriver_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}


static long charDriver_ioctl(struct file *flip, unsigned int cmd, unsigned long arg) {
    return 0;
}
