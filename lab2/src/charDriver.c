/*
 * File         : ele784-lab1.c
 * Description  : ELE784 Lab1 source
 *
 * Etudiants:  JORA09019100 (Alexandru Jora)
 *             MUKM28098503 (Mukandila Mukandila)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/fcntl.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include "circularBuffer.h"
#include "charDriver.h"

// Driver constants
#define MINOR_NUMBER 0
#define NUMBER_OF_DEVICES 1
#define DEVICE_NAME "etsele_cdev"  // Use with 'alloc_chrdev_region()' and 'device_create()'

// Configuration / Defines
#define READWRITE_BUFSIZE 16
#define CIRCULAR_BUFFER_SIZE 256

// Driver internal management struct
typedef struct {
    char ReadBuf[READWRITE_BUFSIZE];
    char WriteBuf[READWRITE_BUFSIZE];
    struct semaphore SemBuf;
    unsigned short numWriter;
    unsigned short numReader;
    unsigned int circularBufferSize;
    dev_t dev;
    struct cdev cdev;
    wait_queue_head_t inq;
    wait_queue_head_t outq;
    struct class *charDriver_class;
    BufferHandle_t Buffer;
} charDriverDev;

// Module Information
MODULE_AUTHOR("JORA Alexandru, MUKANDILA Mukandila");
MODULE_LICENSE("Dual BSD/GPL");

// Prototypes
static int __init charDriver_init(void);
static void __exit charDriver_exit(void);
static int charDriver_open(struct inode *inode, struct file *filp);
static int charDriver_release(struct inode *inode, struct file *filp);
static ssize_t charDriver_read(struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops);
static ssize_t charDriver_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops);
static long charDriver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

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


charDriverDev *charStruct;

struct semaphore SemReadBuf;
struct semaphore SemWriteBuf;

static int __init charDriver_init(void) {

    // 1) init dev_t: must be declared outside the init
    // 2) class_create
    // 3) device_create
    // 4) cdev_init
    // 5) cdev_add
    // 6) init all sempahores
    // 7) init & flush r/w buffers
    // 8) init circular buffer

    int result;
    int i;

    charStruct = kmalloc(sizeof(charDriverDev),GFP_KERNEL);
    if(!charStruct)
        printk(KERN_WARNING"===charDriver_init: ERROR in kmalloc (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    result = alloc_chrdev_region(&charStruct->dev, MINOR_NUMBER, NUMBER_OF_DEVICES, DEVICE_NAME);
    if(result < 0)
        printk(KERN_WARNING"===charDriver_init: ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
    else
        printk(KERN_WARNING"===charDriver_init: MAJOR = %u MINOR = %u\n", MAJOR(charStruct->dev), MINOR(charStruct->dev));

    printk(KERN_WARNING"===charDriver_init: Creating charDriver Class\n");
    charStruct->charDriver_class = class_create(THIS_MODULE, "charDriverClass");

    printk(KERN_WARNING"===charDriver_init: Creating charDriver_Node\n");
    device_create(charStruct->charDriver_class, NULL, charStruct->dev, NULL, "charDriver_Node");

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

    printk(KERN_WARNING "===charDriver_init: initializing wait queue items\n");
    init_waitqueue_head(&charStruct->inq);
    init_waitqueue_head(&charStruct->outq);

    // flush read/write buffers
    for(i=0; i<READWRITE_BUFSIZE; i++){
        charStruct->ReadBuf[i] = '\0';
        charStruct->WriteBuf[i] = '\0';
    }
    charStruct->numWriter = 0;
    charStruct->numReader = 0;

    // init circular buffer
    charStruct->Buffer = circularBufferInit(CIRCULAR_BUFFER_SIZE);
    return 0;
}


static void __exit charDriver_exit(void) {

    printk(KERN_WARNING"===charDriver_exit: cdev DELETE\n");
    cdev_del(&charStruct->cdev);
    printk(KERN_WARNING"===charDriver_exit: charDriver_class DEVICE DELETE\n");
    device_destroy(charStruct->charDriver_class, charStruct->dev);
    printk(KERN_WARNING"===charDriver_exit: charDriver_class DELETE\n");
    class_destroy(charStruct->charDriver_class);
    printk(KERN_WARNING"===charDriver_exit: charStruct devno DELETE\n");
    unregister_chrdev_region(charStruct->dev, 1);
    printk(KERN_WARNING"===charDriver_exit: charStruct kfree()\n");
    kfree(charStruct);

    if(circularBufferDelete(charStruct->Buffer))
        printk(KERN_WARNING "===charDriver_exit: Unable to delete circular buffer\n");
    else
        printk(KERN_WARNING "===charDriver_exit: deleting circular buffer\n");
}


static int charDriver_open(struct inode *inode, struct file *filp) {
    printk(KERN_WARNING "===charDriver_open: entering OPEN function\n");
    printk(KERN_WARNING "===charDriver_open: f_flags=%u\n", filp->f_flags);

    // check opening mode
    switch(filp->f_flags & O_ACCMODE){

        case O_RDONLY:
            // read only
            printk(KERN_WARNING "===charDriver_open: opened in O_RDONLY\n");
            charStruct->numReader++;
            break;

        case O_WRONLY:
            // write only
            printk(KERN_WARNING "===charDriver_open: opened in O_WRONLY\n");

            // only open in O_WRONLY if there are no writers already
            if (!charStruct->numWriter)
                charStruct->numWriter++;
            else
                return -ENOTTY;

            break;

        case O_RDWR:
            // read/write
            printk(KERN_WARNING "===charDriver_open: opened in O_RDWR\n");

            // only open in O_RDWR if there are no writers already
            if (!charStruct->numWriter){
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


static int charDriver_release(struct inode *inode, struct file *filp) {
    int i;
    printk(KERN_WARNING "===charDriver_release: entering RELEASE function\n");

    // empty read/write buffers
    for(i=0; i<READWRITE_BUFSIZE; i++){
        charStruct->ReadBuf[i] = '\0';
        charStruct->WriteBuf[i] = '\0';
    }

    // check opening mode
    switch(filp->f_flags & O_ACCMODE){

        case O_RDONLY:
            // read only
            charStruct->numReader--;
            break;

        case O_WRONLY:
            // write only
            // release semaphore
            //up(&charStruct->SemBuf);
            charStruct->numWriter--;
            break;

        case O_RDWR:
            // read/write
            // release semaphore
            //up(&charStruct->SemBuf);
            charStruct->numReader--;
            charStruct->numWriter--;
            break;

        default:
            printk(KERN_WARNING "===charDriver_release:RELEASE unknown access mode!!\n");
            break;
    }
    return 0;
}


static ssize_t charDriver_read(struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops) {
    int i = 0;
    int x = 0;
    int buf_retcode = 0;
    int ReadBuf_size;
    printk(KERN_WARNING "===charDriver_read: entering READ function\n");
    printk(KERN_WARNING "===charDriver_read: bytes requested by user=%i\n", (int) count);

    // lock semaphore
    if(down_interruptible(&charStruct->SemBuf))
    	return -ERESTARTSYS;

    // if circular buffer is empty we exit
    while(!circularBufferDataCount(charStruct->Buffer)){ // While there's nothing to read in the circular bufer
        printk(KERN_WARNING "===charDriver_read: circular buffer empty!\n");

        // unlock semaphore
        up(&charStruct->SemBuf);

        if(filp->f_flags & O_NONBLOCK) // Open in non-blocking
        	return -EAGAIN;

        printk(KERN_WARNING "===charDriver_read: putting reader to sleep\n");
        if(wait_event_interruptible(charStruct->inq, (circularBufferDataCount(charStruct->Buffer) > 0)))
        	return -ERESTARTSYS;

        if(down_interruptible(&charStruct->SemBuf))
        	return -ERESTARTSYS;

        //return 0;
    } // Data in the buffer, handle it

    // if user requests too many bytes we return multiple chunks of READWRITE_BUFSIZE
    if(count >= READWRITE_BUFSIZE){
        if(down_interruptible(&SemReadBuf))
            return -ERESTARTSYS;
        while(i<READWRITE_BUFSIZE && !buf_retcode){
            buf_retcode = circularBufferOut(charStruct->Buffer, &charStruct->ReadBuf[i]);
            printk(KERN_WARNING "===charDriver_read: circularBufferOut=%i\n", buf_retcode);
            i++;
        }
        printk(KERN_WARNING "===charDriver_read: contents of ReadBuf:%s\n", charStruct->ReadBuf);
        printk(KERN_WARNING "===charDriver_read: returning %i available bytes\n", (int) READWRITE_BUFSIZE);
        if (copy_to_user(ubuf, charStruct->ReadBuf, READWRITE_BUFSIZE)){
            printk(KERN_WARNING "===charDriver_read: error while copying data from kernel space\n");
            return -EFAULT;
        }
        // flush buffer
        for(x=0; x<READWRITE_BUFSIZE; x++){
            charStruct->ReadBuf[x] = '\0';
            charStruct->WriteBuf[x] = '\0';
        }
        up(&SemReadBuf);
        up(&charStruct->SemBuf);
        wake_up_interruptible(&charStruct->outq); // Wakeup the writers
        return READWRITE_BUFSIZE;
    }

    // else we return in one chunk
    else{
        if(down_interruptible(&SemReadBuf))
            return -ERESTARTSYS;
        while(i<count && !buf_retcode){
            buf_retcode = circularBufferOut(charStruct->Buffer, &charStruct->ReadBuf[i]);
            printk(KERN_WARNING "===charDriver_read: circularBufferOut=%i\n", buf_retcode);
            i++;
        }
        printk(KERN_WARNING "===charDriver_read: contents of ReadBuf:%s\n", charStruct->ReadBuf);
        ReadBuf_size = (int) strlen(charStruct->ReadBuf);
        printk(KERN_WARNING "===charDriver_read: returning %i available bytes\n", ReadBuf_size);
        if (copy_to_user(ubuf, charStruct->ReadBuf, ReadBuf_size)){
            printk(KERN_WARNING "===charDriver_read: error while copying data from kernel space\n");
            return -EFAULT;
        }
        // flush buffer
        for(x=0; x<READWRITE_BUFSIZE; x++){
            charStruct->ReadBuf[x] = '\0';
            charStruct->WriteBuf[x] = '\0';
        }
        up(&SemReadBuf);
        up(&charStruct->SemBuf);
        wake_up_interruptible(&charStruct->outq); // Wakeup the writers
        return count;
    }
}


static ssize_t charDriver_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops) {
    int i = 0;
    int x = 0;
    int buf_retcode = 0;
    printk(KERN_WARNING "===charDriver_write: entering WRITE function\n");

    if(down_interruptible(&charStruct->SemBuf)){
        printk(KERN_WARNING "===charDriver_write: unable to acquire SemBuf\n");
    	return -ERESTARTSYS;
    }

    while(!circularBufferDataRemaining(charStruct->Buffer)){ // while the circular buffer is full
    	printk(KERN_WARNING "===charDriver_write: circular buffer is full\n");

    	up(&charStruct->SemBuf); // unlock semaphore

    	if(filp->f_flags & O_NONBLOCK) // Open in non-blocking
    		return -EAGAIN;

    	printk(KERN_WARNING "===charDriver_write: putting writer to sleep\n");
    	if(wait_event_interruptible(charStruct->outq, (circularBufferDataRemaining(charStruct->Buffer))))
    		return -ERESTARTSYS;

    	if(down_interruptible(&charStruct->SemBuf)) // lock semaphore to read
    	    return -ERESTARTSYS;
    } // space in the buffer

    // in case user sends more bytes than size of READWRITE_BUFSIZE we write in chunks
    if (count > READWRITE_BUFSIZE){
        if(down_interruptible(&SemWriteBuf))
            return -ERESTARTSYS;
        if(copy_from_user(charStruct->WriteBuf, ubuf, READWRITE_BUFSIZE)){
            printk(KERN_WARNING "===charDriver_write: error while copying data from user space\n");
            return -EFAULT;
        }
        // push data in circular buffer
        while(i<READWRITE_BUFSIZE && buf_retcode != -1){
            buf_retcode = circularBufferIn(charStruct->Buffer, charStruct->WriteBuf[i]);
            printk(KERN_WARNING "===charDriver_write: circularBufferIn=%i\n", buf_retcode);
            i++;
        }
        printk(KERN_WARNING "===charDriver_write: contents of WriteBuf:%s\n", charStruct->WriteBuf);

        // we return only the number of bytes pushed to circular buffer or zero if full
        if(buf_retcode == -1){
            printk(KERN_WARNING "===charDriver_write: circularBuffer FULL!");
            return 0;
        }
        else{
            up(&SemWriteBuf);
            up(&charStruct->SemBuf);
            wake_up_interruptible(&charStruct->inq); // Wakeup the readers
            return i;
        }
    }

    // else we write it in one chunk
    else{
        if(down_interruptible(&SemWriteBuf))
            return -ERESTARTSYS;
        if(copy_from_user(charStruct->WriteBuf, ubuf, count)){
            printk(KERN_WARNING "===charDriver_write: error while copying data from user space\n");
            return -EFAULT;
        }
        while(i<count && buf_retcode != -1){
            buf_retcode = circularBufferIn(charStruct->Buffer, charStruct->WriteBuf[i]);
            printk(KERN_WARNING "===charDriver_write: circularBufferIn=%i\n", buf_retcode);
            i++;
        }
        printk(KERN_WARNING "===charDriver_write: contents of WriteBuf:%s\n", charStruct->WriteBuf);

        if(buf_retcode == -1){
            printk(KERN_WARNING "===charDriver_write: circularBuffer FULL!");
            return 0;
        }
        else{
            up(&SemWriteBuf);
            up(&charStruct->SemBuf);
            wake_up_interruptible(&charStruct->inq); // Wakeup the readers
            return count;
        }
    }
}


static long charDriver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    printk(KERN_WARNING "===charDriver_ioctl: entering IOCTL function\n");

    if (_IOC_TYPE(cmd) != CHARDRIVER_IOC_MAGIC) {
        printk(KERN_WARNING "===charDriver_ioctl: invalid MAGIC NUMBER\n");
        return -ENOTTY;
    }

    if (_IOC_NR(cmd) > CHARDRIVER_IOC_MAXNR) {
        printk(KERN_WARNING "===charDriver_ioctl: invalid IOCTL command\n");
        return -ENOTTY;
    }

    switch(cmd) {

        case CHARDRIVER_GETNUMDATA:

            put_user(circularBufferDataCount(charStruct->Buffer), (int __user *)arg);
            printk(KERN_WARNING "===charDriver_ioctl: data in buffer is: %i \n", circularBufferDataCount(charStruct->Buffer));

            break;

        case CHARDRIVER_GETNUMREADER:

            printk(KERN_WARNING "===charDriver_ioctl: number of readers is: %i \n", charStruct->numReader);
            put_user(charStruct->numReader, (int __user *)arg);
            break;

        case CHARDRIVER_GETBUFSIZE:

            printk(KERN_WARNING "===charDriver_ioctl: size of buffer is: %i \n", circularBufferDataSize(charStruct->Buffer));
            put_user(circularBufferDataSize(charStruct->Buffer), (int __user *)arg);

            break;

        case CHARDRIVER_SETBUFSIZE:

            if(!capable(CAP_SYS_ADMIN))
                return -EPERM;

            printk(KERN_WARNING "===charDriver_ioctl: resizing buffer to %i \n", (int)arg);
            if(down_interruptible(&charStruct->SemBuf))
                return -ERESTARTSYS;
            if(circularBufferResize(charStruct->Buffer, (unsigned int)arg))
                return -EINVAL;
            up(&charStruct->SemBuf);
            break;

        case CHARDRIVER_GETMAGICNUMBER:

            put_user(CHARDRIVER_IOC_MAGIC, (char __user *)arg);
            printk(KERN_WARNING "===charDriver_ioctl: the magic number is: %c \n", CHARDRIVER_IOC_MAGIC);

            break;

        default:
            return -EINVAL;
    }
    return 0;
}
