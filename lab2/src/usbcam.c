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
#include <uapi/asm-generic/int-ll64.h>
#include <linux/usb.h>
#include <linux/completion.h>
#include "usbvideo.h"
#include "dht_data.h"
#include "usbcam.h"

// Module Information
MODULE_AUTHOR("JORA Alexandru, MUKANDILA Mukandila");
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

// General data structure for driver
struct USBCam_Dev {
    struct usb_device *usbdev;
    struct usb_interface *usbcam_interface;
    struct urb *myUrb[5];
    struct semaphore sem_read;
    struct semaphore sem_grab;
    struct completion *urb_done;
    atomic_t urbCounter;
};


// Private function prototypes
static int urbInit(struct usb_interface *intf, struct USBCam_Dev *cam_dev);
static void urbCompletionCallback(struct urb *urb);

// global vars
static unsigned int myStatus = 0;
static unsigned int myLength = 42666;
static unsigned int myLengthUsed = 0;
static char myData[42666];
atomic_t open_count;


static struct usb_device_id usbcam_table[] = {
// { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
{ USB_DEVICE(0x046d, 0x08cc) },
{ USB_DEVICE(0x046d, 0x0994) },
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

#define USBCAM_MINOR 0
static struct usb_class_driver usbcam_class = {
	.name       = "usb/usbcam%d",
	.fops       = &usbcam_fops,
	.minor_base = USBCAM_MINOR,
};

static int __init usbcam_init(void) {
    int rv;
    rv = usb_register(&usbcam_driver);
    if(rv) {
        printk(KERN_ERR "===usbcam_INIT: ERROR registering USB device; code=%d\n", rv);
        return rv;
    }
    else{
        printk(KERN_WARNING "===usbcam_INIT: Device succesfully registered with USBCORE\n");
        return 0;
    }
}

static void __exit usbcam_exit(void) {
    printk(KERN_WARNING "===usbcam_EXIT: Deregistering usb device\n");
    usb_deregister(&usbcam_driver);
}

static int usbcam_probe (struct usb_interface *intf, const struct usb_device_id *devid) {
    const struct usb_host_interface *interface;
    int rv, i;
    struct usb_device *dev = interface_to_usbdev(intf);

    // flag for subclass detection
    int active_interface = 0;

	interface = &intf->altsetting[0];

    if(interface->desc.bInterfaceClass == CC_VIDEO) {
        if(interface->desc.bInterfaceSubClass == SC_VIDEOCONTROL) {
            printk(KERN_WARNING "===usbcam_PROBE: FOUND INTERFACE!\n");
            return 0;
        }
        if(interface->desc.bInterfaceSubClass == SC_VIDEOSTREAMING) {
            printk(KERN_WARNING "===usbcam_PROBE: Found proper Interface Subclass\n");
            // set flag to notify that subclass is found
            active_interface = 1;
        }
        else
            return -1;// end subclass check if
    }
    else
        return -1;// end class check if

    printk(KERN_WARNING "===usbcam_PROBE: Done detecting Interface(s)\n");

    if(active_interface) {
        printk(KERN_WARNING "===usbcam_PROBE: Active interface found\n");

        struct USBCam_Dev *cam_dev = NULL;
        cam_dev = (struct USBCam_Dev*) kmalloc(sizeof(struct USBCam_Dev), GFP_KERNEL);

        if(!cam_dev) {
            printk(KERN_WARNING "===usbcam_PROBE: Cannot allocate memory to USBCam_Dev (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            return -ENOMEM;
        }

        // increment reference count of usb device structure
        cam_dev->usbdev = usb_get_dev(dev);

        // associate cam_dev to interface selected
        usb_set_intfdata (intf, cam_dev);
        rv = usb_register_dev(intf, &usbcam_class);

        if(rv < 0) {
           printk(KERN_WARNING "===usbcam_PROBE: Error registering driver with USBCORE\n");
           return -EINVAL;
        }
        else
            printk(KERN_WARNING "===usbcam_PROBE: Registered driver with USBCORE\n");

        // initialize access protection variables and such
        sema_init(&cam_dev->sem_read, 0);
        sema_init(&cam_dev->sem_grab, 0);
        cam_dev->urbCounter = (atomic_t) ATOMIC_INIT(0);
        cam_dev->urb_done = (struct completion *) kmalloc(sizeof(struct completion), GFP_KERNEL);
        open_count = (atomic_t) ATOMIC_INIT(0);
        init_completion(cam_dev->urb_done);

        // just to be sure
        for(i=0; i<5; i++) {
            cam_dev->myUrb[i] = NULL;
        }

        // make altsetting current
        usb_set_interface(cam_dev->usbdev, 1, 4);

        printk(KERN_WARNING "===usbcam_PROBE: Done probe routine\n");
        return 0;
    }
    else{
        printk(KERN_WARNING "===usbcam_PROBE: Could not associate interface to device\n");
        return -ENODEV;
    }
}

void usbcam_disconnect(struct usb_interface *intf) {
    usb_set_intfdata(intf, NULL);
    usb_deregister_dev(intf, &usbcam_class);
}

int usbcam_open (struct inode *inode, struct file *filp) {
    struct usb_interface *intf;
    int subminor, count;

    // only allow 1 open at a time
    count = (int) atomic_read(&open_count);
    if(!count) {
        atomic_inc(&open_count);
        subminor = iminor(inode);
        intf = usb_find_interface(&usbcam_driver, subminor);

        if(!intf) {
            printk(KERN_WARNING "===usbcam_OPEN: Unable to open device driver\n");
            return -ENODEV;
        }
        printk(KERN_WARNING "===usbcam_OPEN: Opening usbcam driver in READONLY MODE!\n");

        // copy interface inside private_data
        filp->private_data = intf;
        return 0;
    }
    else
        return -EBUSY;
}

int usbcam_release (struct inode *inode, struct file *filp) {
    // reset open counter
    atomic_set(&open_count, 0);

    switch(filp->f_flags & O_ACCMODE) {
        case O_RDONLY:
            printk(KERN_WARNING "===usbcam_RELEASE: Releasing usbcam driver in from READONLY MODE!\n");
            return 0;

        default:
            printk(KERN_WARNING "===usbcam_RELEASE: UNKNOWN RELEASE MODE!\n");
            return -ENOTTY;
    }
}

ssize_t usbcam_read (struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops) {
    int i;
    struct usb_interface *intf;
    struct USBCam_Dev *cam_dev;
    unsigned int bytes_missed = 0;

    // get interface from private_data
    intf = filp->private_data;
    cam_dev = usb_get_intfdata(intf);

    // try to acquire semaphore without blocking
    if(down_trylock(&cam_dev->sem_read)) {
        printk(KERN_ERR "===usbcam_READ: sem_read not available! Exiting!\n");
        return -EBUSY;
    }

    // wait for callback to be done
    printk(KERN_WARNING "===usbcam_READ: Waiting for urb callback completion...\n");
    wait_for_completion(cam_dev->urb_done);
    printk(KERN_WARNING "===usbcam_READ: Completion done!\n");

    // copy data to user space
    bytes_missed = copy_to_user(ubuf, myData, myLengthUsed);

    // destroy all URB
    for(i=0; i<5; i++) {
        printk(KERN_WARNING "===usbcam_READ: Killing URB (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_kill_urb(cam_dev->myUrb[i]);

        printk(KERN_WARNING "===usbcam_READ: USB_FREE_COHERENT (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_free_coherent(cam_dev->usbdev, cam_dev->myUrb[i]->transfer_buffer_length, cam_dev->myUrb[i]->transfer_buffer, cam_dev->myUrb[i]->transfer_dma);

        printk(KERN_WARNING "===usbcam_READ: USB_FREE_URB (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_free_urb(cam_dev->myUrb[i]);

        // debugging some weird issue with usb_free_urb
        cam_dev->myUrb[i] = NULL;
    }

    // return actual number of bytes copied to user space
    return myLengthUsed - bytes_missed;
}

ssize_t usbcam_write (struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}

long usbcam_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
    char cam_pos[4];
    int rv;
    int pantilt = 0x03;
    struct usb_interface *intf = filp->private_data;
    struct USBCam_Dev *cam_dev = usb_get_intfdata(intf);

    switch(cmd) {
        case IOCTL_GET:
            if(arg == GET_CUR || arg == GET_MIN || arg == GET_MAX || arg == GET_RES) {
                rv = usb_control_msg(cam_dev->usbdev,
                                usb_rcvctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                                arg,
                                (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                                0,
                                0x0200,
                                NULL, // use this for now.
                                2,
                                0);

                if(rv < 0) {
                    printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_GET! code %i\n", rv);
                    return rv;
                }
                break;
            }
            else {
                printk(KERN_ERR "===usbcam_IOCTL_get: ERROR arg is invalid! %s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
                return -EFAULT;
            }

        case IOCTL_SET:
            rv = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0,
                            0x0200,
                            arg,
                            2,
                            0);
            if(rv < 0) {
                printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_SET! code %i\n", rv);
                return rv;
            }
            break;

        case IOCTL_STREAMON:
            printk(KERN_WARNING "===usbcam_IOCTL: Entering IOCTL_STREAMON\n");
            rv = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x0B,
                            (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),
                            0x0004,
                            0x0001,
                            NULL,
                            0,
                            0);
            if(rv < 0) {
                printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_STREAMON! code %i\n", rv);
                return rv;
            }
            // release sem_grab
            up(&cam_dev->sem_grab);
            break;

        case IOCTL_STREAMOFF:
            printk(KERN_WARNING "===usbcam_IOCTL: Entering IOCTL_STREAMOFF\n");
            rv = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x0B,
                            (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),
                            0x0000,
                            0x0001,
                            NULL,
                            0,
                            0);
            if(rv < 0) {
                printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_STREAMOFF! code %i\n", rv);
                return rv;
            }
            break;

        case IOCTL_GRAB:
            printk(KERN_WARNING "===usbcam_IOCTL: Entering IOCTL_GRAB\n");
            if(down_trylock(&cam_dev->sem_grab)) {
                printk(KERN_WARNING "===usbcam_IOCTL: sem_grab not available. Exiting!\n");
                return -EBUSY;
            }
            urbInit(intf, cam_dev);
            // release sem_read
            up(&cam_dev->sem_read);
            break;

        case IOCTL_PANTILT:
            // fill cam_pos array based on user provided position
            switch(arg) {
                // UP
                case 0:
                    cam_pos[0] = 0x00;
                    cam_pos[1] = 0x00;
                    cam_pos[2] = 0x80;
                    cam_pos[3] = 0xFF;
                    break;

                // DOWN
                case 1:
                    cam_pos[0] = 0x00;
                    cam_pos[1] = 0x00;
                    cam_pos[2] = 0x80;
                    cam_pos[3] = 0x00;
                    break;

                // LEFT
                case 2:
                    cam_pos[0] = 0x80;
                    cam_pos[1] = 0x00;
                    cam_pos[2] = 0x00;
                    cam_pos[3] = 0x00;
                    break;

                // RIGHT
                case 3:
                    cam_pos[0] = 0x80;
                    cam_pos[1] = 0xFF;
                    cam_pos[2] = 0x00;
                    cam_pos[3] = 0x00;
                    break;
            }
            //send actual usb_control_msgs
            rv = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0x0100,
                            0x0900,
                            &cam_pos,
                            4,
                            0);
            if(rv < 0) {
                printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_PANTILT! code %i\n", rv);
                return rv;
            }
            break;

        case IOCTL_PANTILT_RESET:
            rv = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0x0200,
                            0x0900,
                            &pantilt,//0x03,
                            1,
                            0);
            if(rv < 0) {
                printk(KERN_ERR "===usbcam_IOCTL: ERROR something went wrong during IOCTL_PANTILT_RESET! code %i\n", rv);
                return rv;
            }
            break;

        default:
            return -EINVAL;
    }
    return 0;
}


// *************************** //
// **** Private functions **** //
// *************************** //

int urbInit(struct usb_interface *intf, struct USBCam_Dev *cam_dev) {

    unsigned int i, j, rv, nbPackets, myPacketSize, size, nbUrbs;
    struct usb_host_interface *cur_altsetting;
    struct usb_endpoint_descriptor endpointDesc;
    myStatus = 0;
    myLengthUsed = 0;

    cur_altsetting = intf->cur_altsetting;
    endpointDesc = cur_altsetting->endpoint[0].desc;

    nbPackets = 40;  // The number of isochronous packets this urb should contain
    myPacketSize = le16_to_cpu(endpointDesc.wMaxPacketSize);
    size = myPacketSize * nbPackets;
    nbUrbs = 5;

    for(i = 0; i < nbUrbs; i++) {
        usb_free_urb(cam_dev->myUrb[i]);
        cam_dev->myUrb[i] = usb_alloc_urb(nbPackets, GFP_ATOMIC);
        if(cam_dev->myUrb[i] == NULL) {
            printk(KERN_ERR "===usbcam_urbInit: ERROR allocating memory for urb!\n");
            return -ENOMEM;
        }

        // NOTE: usb_buffer_alloc renamed to usb_alloc_coherent
        // see linux commit 073900a28d95c75a706bf40ebf092ea048c7b236
        cam_dev->myUrb[i]->transfer_buffer = usb_alloc_coherent(cam_dev->usbdev, size, GFP_KERNEL, &cam_dev->myUrb[i]->transfer_dma);

        if (cam_dev->myUrb[i]->transfer_buffer == NULL) {
            printk(KERN_ERR "===usbcam_urbInit: ERROR allocating memory for transfer buffer!\n");
            usb_free_urb(cam_dev->myUrb[i]);
            return -ENOMEM;
        }

        // initializing isochronous urb by hand
        printk(KERN_WARNING "===usbcam_urbInit: initializing isochronous urb: %d\n", i);
        cam_dev->myUrb[i]->dev = cam_dev->usbdev;
        cam_dev->myUrb[i]->context = cam_dev;
        cam_dev->myUrb[i]->pipe = usb_rcvisocpipe(cam_dev->usbdev, endpointDesc.bEndpointAddress);
        cam_dev->myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
        cam_dev->myUrb[i]->interval = endpointDesc.bInterval;
        cam_dev->myUrb[i]->complete = urbCompletionCallback;
        cam_dev->myUrb[i]->number_of_packets = nbPackets;
        cam_dev->myUrb[i]->transfer_buffer_length = size;

        for (j = 0; j < nbPackets; ++j) {
            cam_dev->myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
            cam_dev->myUrb[i]->iso_frame_desc[j].length = myPacketSize;
        }
    }

    for(i = 0; i < nbUrbs; i++) {
        rv = usb_submit_urb(cam_dev->myUrb[i], GFP_ATOMIC);

        if(rv < 0) {
            printk(KERN_ERR "===usbcam_URBINIT: ERROR submitting URB: %i\n", rv);
            return rv;
        }
    }
    return 0;
}


static void urbCompletionCallback(struct urb *urb) {
    int ret, urbCounterTotal;
    int i;
    unsigned char *data;
    unsigned int len;
    unsigned int maxlen;
    unsigned int nbytes;
    void * mem;
    struct USBCam_Dev *cam_dev;

    cam_dev = urb->context;
    if(urb->status == 0) {
        for (i = 0; i < urb->number_of_packets; ++i) {
            if(myStatus == 1) {
                continue;
            }
            if (urb->iso_frame_desc[i].status < 0) {
                continue;
            }

            data = urb->transfer_buffer + urb->iso_frame_desc[i].offset;
            if(data[1] & (1 << 6)) {
                continue;
            }
            len = urb->iso_frame_desc[i].actual_length;
            if (len < 2 || data[0] < 2 || data[0] > len) {
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

        if (!(myStatus == 1)) {
            if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
                printk(KERN_ERR "===usbcam_CALLBACK: ERROR submitting URB: %i\n", ret);
            }
        }

        else {
            // possibly redundant
            myStatus = 0;

            // increment urbCounter
            atomic_inc(&cam_dev->urbCounter);

            urbCounterTotal = (int) atomic_read(&cam_dev->urbCounter);
            if(urbCounterTotal == 5) {
                // reset urbCounter
                atomic_set(&cam_dev->urbCounter, 0);

                // mark urb complete
                complete(cam_dev->urb_done);
            }
        }
    }
    else
        printk(KERN_ERR "===usbcam_CALLBACK: ERROR completing urb: %i\n", urb->status);
}
