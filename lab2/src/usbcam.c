/*
 * File         : usbcam.c
 * Description  : ELE784 Lab2 source
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
#include <uapi/asm-generic/ioctl.h>
#include <linux/usb.h>
#include <linux/completion.h>
#include "usbvideo.h"
#include "dht_data.h"
#include "usbcam.h"

// Module Information
MODULE_AUTHOR("prenom nom #1, prenom nom #2");
MODULE_LICENSE("Dual BSD/GPL");

// Prototypes
static int __init usbcam_init(void);
static void __exit usbcam_exit(void);
static int usbcam_probe (struct usb_interface *intf, const struct usb_device_id *devid);
static void usbcam_disconnect(struct usb_interface *intf);
static int usbcam_open (struct inode *inode, struct file *filp);
static int usbcam_release (struct inode *inode, struct file *filp) ;
static ssize_t usbcam_read (struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops);
static ssize_t usbcam_write (struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops);
static long usbcam_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
module_init(usbcam_init);
module_exit(usbcam_exit);

// Private function prototypes
static int urbInit(struct urb *urb, struct usb_interface *intf);
static void urbCompletionCallback(struct urb *urb);


static unsigned int myStatus;
static unsigned int myLength;
static unsigned int myLengthUsed;
static char * myData;
static struct urb *myUrb[5];

struct usbcam_dev {
	struct usb_device *usbdev;
    dev_t dev;
    struct cdev cdev;
    struct class *usbcam_class;
};



static struct usb_device_id usbcam_table[] = {
// { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
{ USB_DEVICE(0x046d, 0x08cc) },
{}
};
MODULE_DEVICE_TABLE(usb, usbcam_table);

// USB Driver structure
static struct usb_driver usbcam_driver = {
	.name       = "usbcam",
	.id_table   = usbcam_table,
	.probe      = usbcam_probe,
	.disconnect = usbcam_disconnect,
};

// File operation structure
struct file_operations usbcam_fops = {
	.owner          = THIS_MODULE,
	.open           = usbcam_open,
	.release        = usbcam_release,
	.read           = usbcam_read,
	.write          = usbcam_write,
	.unlocked_ioctl = usbcam_ioctl,
};

// driver constants
#define USBCAM_MINOR 0
#define DEVICE_NAME "etsele_usbcam"

static struct usb_class_driver usbcam_class = {
	.name       = "usb/usbcam%d",
	.fops       = &usbcam_fops,
	.minor_base = USBCAM_MINOR,
};

static int __init usbcam_init(void) {
    int res;
    printk(KERN_WARNING "===usbcam_init: registering usb device with usbcore\n");
    res = usb_register(&usbcam_driver);
    if(res)
        printk(KERN_WARNING "===usbcam_init: ERROR registering USB device %d\n", res);
    else
        printk(KERN_WARNING "===usbcam_init: device registered with return value: %d\n", res);

    return 0;
}

static void __exit usbcam_exit(void) {
    printk(KERN_WARNING "===usbcam_exit: deregistering usb device\n");
    usb_deregister(&usbcam_driver);
}

static int usbcam_probe (struct usb_interface *intf, const struct usb_device_id *devid) {
    const struct usb_host_interface *interface;
    const struct usb_endpoint_descriptor *endpoint;
    struct usb_device *dev = interface_to_usbdev(intf);
    struct usb

    return -1;
}

void usbcam_disconnect(struct usb_interface *intf) {

}

int usbcam_open (struct inode *inode, struct file *filp) {
    return 0;
}

int usbcam_release (struct inode *inode, struct file *filp) {
    return 0;
}

ssize_t usbcam_read (struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}

ssize_t usbcam_write (struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}

long usbcam_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
    return 0;
}


// *************************** //
// **** Private functions **** //
// *************************** //

/* FIXME: REMOVE THIS LINE

int urbInit(struct urb *urb, struct usb_interface *intf) {
    int i, j, ret, nbPackets, myPacketSize, size, nbUrbs;
    struct usb_host_interface *cur_altsetting = intf->cur_altsetting;
    struct usb_endpoint_descriptor endpointDesc = cur_altsetting->endpoint[0].desc;

    nbPackets = 40;  // The number of isochronous packets this urb should contain
    myPacketSize = le16_to_cpu(endpointDesc.wMaxPacketSize);
    size = myPacketSize * nbPackets;
    nbUrbs = 5;

    for (i = 0; i < nbUrbs; ++i) {
        // TODO: usb_free_urb(...);
        // TODO: myUrb[i] = usb_alloc_urb(...);
        if (myUrb[i] == NULL) {
            // TODO: printk(KERN_WARNING "");
            return -ENOMEM;
        }

        // TODO: myUrb[i]->transfer_buffer = usb_buffer_alloc(...);

        if (myUrb[i]->transfer_buffer == NULL) {
            // printk(KERN_WARNING "");
            usb_free_urb(myUrb[i]);
            return -ENOMEM;
        }

        // TODO: myUrb[i]->dev = ...
        // TODO: myUrb[i]->context = *dev*;
        // TODO: myUrb[i]->pipe = usb_rcvisocpipe(*dev*, endpointDesc.bEndpointAddress);
        myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
        myUrb[i]->interval = endpointDesc.bInterval;
        // TODO: myUrb[i]->complete = ...
        // TODO: myUrb[i]->number_of_packets = ...
        // TODO: myUrb[i]->transfer_buffer_length = ...

        for (j = 0; j < nbPackets; ++j) {
            myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
            myUrb[i]->iso_frame_desc[j].length = myPacketSize;
        }
    }

    for(i = 0; i < nbUrbs; i++){
        // TODO: if ((ret = usb_submit_urb(...)) < 0) {
            // TODO: printk(KERN_WARNING "");
            return ret;
        }
    }
    return 0;
}


static void urbCompletionCallback(struct urb *urb) {
    int ret;
    int i;
    unsigned char * data;
    unsigned int len;
    unsigned int maxlen;
    unsigned int nbytes;
    void * mem;

    if(urb->status == 0){

        for (i = 0; i < urb->number_of_packets; ++i) {
            if(myStatus == 1){
                continue;
            }
            if (urb->iso_frame_desc[i].status < 0) {
                continue;
            }

            data = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
            if(data[1] & (1 << 6)){
                continue;
            }
            len = urb->iso_frame_desc[i].actual_length;
            if (len < 2 || data[0] < 2 || data[0] > len){
                continue;
            }

            len -= data[0];
            maxlen = myLength - myLengthUsed ;
            mem = myData + myLengthUsed;
            nbytes = min(len, maxlen);
            memcpy(mem, data + data[0], nbytes);
            myLengthUsed += nbytes;

            if (len > maxlen) {
                myStatus = 1; // DONE
            }

            // Mark the buffer as done if the EOF marker is set.
            if ((data[1] & (1 << 1)) && (myLengthUsed != 0)) {
                myStatus = 1; // DONE
            }
        }

        if (!(myStatus == 1)){
            if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
                // TODO: printk(KERN_WARNING "");
            }
        } else {
            ///////////////////////////////////////////////////////////////////////
            //  Synchronisation
            ///////////////////////////////////////////////////////////////////////
            //TODO
        }
    } else {
        // TODO: printk(KERN_WARNING "");
    }
}

FIXME: REMOVE THIS LINE*/
