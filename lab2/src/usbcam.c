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

//#define THIS_MODULE 	"usbcam"
//#define KBUILD_MODNAME	"ele784-lab2"

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

// Private function prototypes
static int urbInit(struct usb_interface *intf, struct usb_device *dev);
static void urbCompletionCallback(struct urb *urb);

// global vars
static unsigned int myStatus = 0;
static unsigned int myLength = 42666;
static unsigned int myLengthUsed = 0;
static char myData[42666];
static int flag_done = 0;

// General data structure for driver
struct USBCam_Dev {
    struct usb_device *usbdev;
    unsigned int number_interfaces;
    int active_interface;
    struct usb_interface *usbcam_interface;
    struct urb *myUrb[5];
    struct semaphore SemURB;
    atomic_t urbCounter;
};

struct class *my_class;

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
    struct usb_device *dev = interface_to_usbdev(intf);
    int retnum;//, altSetNum;
    struct USBCam_Dev *cam_dev = NULL;

	// Allocate memory to local driver structure
    cam_dev = kmalloc(sizeof(struct USBCam_Dev), GFP_KERNEL);

    if(!cam_dev)
        printk(KERN_WARNING "===usbcam_probe: Cannot allocate memory to USBCam_Dev (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

    cam_dev->usbdev = usb_get_dev(dev);
    cam_dev->active_interface = -1;
    interface = &intf->altsetting[0];

    if(interface->desc.bInterfaceClass == CC_VIDEO) {
        if(interface->desc.bInterfaceSubClass == SC_VIDEOCONTROL) {
            printk(KERN_WARNING "===usbcam_probe: FOUND INTERFACE!\n");
                        return 0;
        }
        if(interface->desc.bInterfaceSubClass == SC_VIDEOSTREAMING) {
            printk(KERN_WARNING "===usbcam_probe: Found proper Interface Subclass\n");
            cam_dev->active_interface = 1;
        }
        else
            return -1;// end subclass check if
    }
    else
        return -1;// end class check if

    printk(KERN_WARNING "===usbcam_probe: Done detecting Interface(s)\n");

    if(cam_dev->active_interface != -1) {
        usb_set_intfdata (intf, cam_dev); // This is where the cam_dev structure is associated with the interface selected
        retnum = usb_register_dev(intf, &usbcam_class);
        if(retnum < 0)
            printk(KERN_WARNING "===usbcam_probe: Error registering driver with USBCORE\n");
        else
            printk(KERN_WARNING "===usbcam_probe: Registered driver with USBCORE\n");
        usb_set_interface(cam_dev->usbdev, 1, 4);
        printk(KERN_WARNING "===usbcam_probe: all good\n");

        printk(KERN_WARNING "===usbcam_init: Initializing URB status Semaphore\n");
        sema_init(&cam_dev->SemURB, 1);

        printk(KERN_WARNING "===usbcam_init: Initializing URB counter to 0\n");
        cam_dev->urbCounter = (atomic_t) ATOMIC_INIT(0);

        return 0;
    }
    else{
        printk(KERN_WARNING "===usbcam_probe: could not associate interface to device\n");
        return -1;
    }

}

void usbcam_disconnect(struct usb_interface *intf) {
    usb_set_intfdata(intf, NULL);
    usb_deregister_dev(intf, &usbcam_class);
}

int usbcam_open (struct inode *inode, struct file *filp) {

    struct usb_interface *intf;
    int subminor;

    printk(KERN_WARNING "===usbcam_open: Opening usbcam driver in READONLY MODE!\n");

    subminor = iminor(inode);

    intf = usb_find_interface(&usbcam_driver, subminor);

    if(!intf) {
    	printk(KERN_WARNING "===usbcam_open: Unable to open device driver\n");
    	return -ENODEV;
    }

    filp->private_data = intf;
    return 0;
}

int usbcam_release (struct inode *inode, struct file *filp) {

    switch(filp->f_flags & O_ACCMODE) {
        case O_RDONLY:
            printk(KERN_WARNING "===usbcam_release: Releasing usbcam driver in from READONLY MODE!\n");
        break;
        default:
            printk(KERN_WARNING "===usbcam_release: UNKNOWN RELEASE MODE!\n");
            return -ENOTTY;
        break;
    }
    return 0;
}

ssize_t usbcam_read (struct file *filp, char __user *ubuf, size_t count, loff_t *f_ops) {
    int i;
    int bytes_copied = 0;
    struct usb_interface *intf;
    struct USBCam_Dev *cam_dev;
    intf = filp->private_data;
    cam_dev = usb_get_intfdata(intf);

    // wait for callback to be done
    while(!flag_done) {
        printk(KERN_WARNING "===usbcam_read: Waiting for URB callback completion\n");
    }

    // perhaps perform some access protection as well?

    // copy data to user space
    printk(KERN_WARNING "===usbcam_read: COPY TO USER\n");
    bytes_copied = copy_to_user(ubuf, myData, myLengthUsed);
    printk(KERN_WARNING "===usbcam_read: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

    // in case something went wrong
    if(bytes_copied < 0) {
        printk(KERN_WARNING "===usbcam_read: error while copying data from kernel space(%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
        return -EFAULT;
    }

    // destroy all URB
    for(i=0; i<5; i++) {
        printk(KERN_WARNING "===usbcam_read:=====================\n");
        printk(KERN_WARNING "===usbcam_read:INFO ON URB ABOUT TO BE DESTROYED\n");
        printk(KERN_WARNING "===usbcam_read:transfer_buffer_length: %u\n", cam_dev->myUrb[i]->transfer_buffer_length);
        printk(KERN_WARNING "===usbcam_read:transfer_dma: %lu\n", cam_dev->myUrb[i]->transfer_dma);
        if(cam_dev->myUrb[i]->transfer_buffer == NULL)
            printk(KERN_WARNING "===usbcam_read:transfer_buffer is NULL!\n");
        else
            printk(KERN_WARNING "===usbcam_read:transfer_buffer is NOT NULL!\n");
        printk(KERN_WARNING "===usbcam_read:=====================\n");

        printk(KERN_WARNING "===usbcam_read: Killing URB (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        usb_kill_urb(cam_dev->myUrb[i]);

        printk(KERN_WARNING "===usbcam_read: USB_FREE_COHERENT (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        // free urb buffer, really unsure about this..
        usb_free_coherent(cam_dev->usbdev, cam_dev->myUrb[i]->transfer_buffer_length, cam_dev->myUrb[i]->transfer_buffer, cam_dev->myUrb[i]->transfer_dma);

        printk(KERN_WARNING "===usbcam_read: USB_FREE_URB (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        // free urb struct
        usb_free_urb(cam_dev->myUrb[i]);
        if(cam_dev->myUrb[i] != NULL)
            printk(KERN_WARNING "===usbcam_read: URB not freed properly!\n");
        else
            printk(KERN_WARNING "===usbcam_read: URB freed!\n");
    }
    return bytes_copied;
}

ssize_t usbcam_write (struct file *filp, const char __user *ubuf, size_t count, loff_t *f_ops) {
    return 0;
}

long usbcam_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {
    printk(KERN_WARNING "===usbcam_ioctl: entering IOCTL function\n");
    char cam_pos[4];
    int retcode;
    int pantilt = 0x03;
    struct usb_interface *intf = filp->private_data;
    struct USBCam_Dev *cam_dev = usb_get_intfdata(intf);

    //NOTE let's not forget to add access protection in this function!

    // prototype of usb_control_msg
    /*
    int usb_control_msg (struct usb_device * dev,
    unsigned int pipe,
    __u8 request,
    __u8 requesttype,
    __u16 value,
    __u16 index,
    void * data,
    __u16 size,
    int timeout);
    */

    switch(cmd) {

        case IOCTL_GET:
            if(arg == GET_CUR || arg == GET_MIN || arg == GET_MAX || arg == GET_RES) {
                retcode = usb_control_msg(cam_dev->usbdev,
                                usb_rcvctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                                arg,
                                (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                                0,
                                0x0200,
                                //&data,
                                NULL, // use this for now.
                                2,
                                0);
                if(retcode < 0) {
                    printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_GET! code %i\n", retcode);
                    return retcode;
                }
                break;
            }
            else{
                printk(KERN_WARNING "===usbcam_ioctl_get: ERROR arg is invalid! %s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
                return -EFAULT;
            }


        case IOCTL_SET:
            retcode = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0,
                            0x0200,
                            arg,
                            2,
                            0);
            if(retcode < 0) {
                printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_SET! code %i\n", retcode);
                return retcode;
                }
            break;

        case IOCTL_STREAMON:
            printk(KERN_WARNING "===usbcam_ioctl: Entering IOCTL_STREAMON\n");
            printk(KERN_WARNING "===usbcam_ioctl: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            retcode = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x0B,
                            (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),
                            0x0004,
                            0x0001,
                            NULL,
                            0,
                            0);
            if(retcode < 0) {
                printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_STREAMON! code %i\n", retcode);
                return retcode;
            }
            break;

        case IOCTL_STREAMOFF:
            printk(KERN_WARNING "===usbcam_ioctl: Entering IOCTL_STREAMOFF\n");
            retcode = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x0B,
                            (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE),
                            0x0000,
                            0x0001,
                            NULL,
                            0,
                            0);
            if(retcode < 0) {
                printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_STREAMOFF! code %i\n", retcode);
                return retcode;
            }
            break;

        case IOCTL_GRAB:
            printk(KERN_WARNING "===usbcam_ioctl: Entering IOCTL_GRAB\n");
            urbInit(intf, cam_dev->usbdev);
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
            retcode = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0x0100,
                            0x0900,
                            &cam_pos,
                            4,
                            0);
            if(retcode < 0) {
                printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_PANTILT! code %i\n", retcode);
                return retcode;
            }
            break;

        case IOCTL_PANTILT_RESET:
            retcode = usb_control_msg(cam_dev->usbdev,
                            usb_sndctrlpipe(cam_dev->usbdev, cam_dev->usbdev->ep0.desc.bEndpointAddress),
                            0x01,
                            (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
                            0x0200,
                            0x0900,
                            &pantilt,//0x03,
                            1,
                            0);
            if(retcode < 0) {
                printk(KERN_WARNING "===usbcam_ioctl: ERROR something went wrong during IOCTL_PANTILT_RESET! code %i\n", retcode);
                return retcode;
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

int urbInit(struct usb_interface *intf, struct usb_device *dev) {

    unsigned int i, j, ret, nbPackets, myPacketSize, size, nbUrbs;
    //struct usb_device *dev = usb_get_intfdata(intf);

    struct usb_host_interface *cur_altsetting;
    struct usb_endpoint_descriptor endpointDesc;
    struct USBCam_Dev *cam_dev;
    myStatus = 0;
    myLengthUsed = 0;
    cur_altsetting = intf->cur_altsetting;
    endpointDesc = cur_altsetting->endpoint[0].desc;

    cam_dev = usb_get_intfdata(intf);

    //cur_altsetting = intf->cur_altsetting;
    //endpointDesc = cur_altsetting->endpoint[0].desc;

    nbPackets = 40;  // The number of isochronous packets this urb should contain
    myPacketSize = le16_to_cpu(endpointDesc.wMaxPacketSize);
    size = myPacketSize * nbPackets;
    nbUrbs = 5;
    printk(KERN_WARNING "===usbcam_urbinit_datacheck: Endpoint address %d\n", endpointDesc.bEndpointAddress);
    printk(KERN_WARNING "===usbcam_urbinit_datacheck: Endpoint packet size %d\n", myPacketSize);
    printk(KERN_WARNING "===usbcam_urbinit_datacheck: Endpoint interval %d\n", endpointDesc.bInterval);
    printk(KERN_WARNING "===usbcam_urbinit_datacheck: URB Pipe value is %d\n", usb_rcvisocpipe(cam_dev->usbdev, endpointDesc.bEndpointAddress));
    printk(KERN_WARNING "===usbcam_urbinit_datacheck: URB transfer flag is %d\n", URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP);
    printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

    // reset flag_done
    flag_done = 0;

    for (i = 0; i < nbUrbs; i++) {

        //if(cam_dev->myUrb[i] != NULL)
        usb_free_urb(cam_dev->myUrb[i]);
        printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        cam_dev->myUrb[i] = usb_alloc_urb(nbPackets, GFP_ATOMIC);
        printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        if (cam_dev->myUrb[i] == NULL) {
            printk(KERN_WARNING "===usbcam_urbInit: ERROR allocating memory for urb!\n");
            return -ENOMEM;
        }

        // NOTE: usb_buffer_alloc renamed to usb_alloc_coherent
        // see linux commit 073900a28d95c75a706bf40ebf092ea048c7b236
        printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        cam_dev->myUrb[i]->transfer_buffer = usb_alloc_coherent(cam_dev->usbdev, size, GFP_KERNEL, &cam_dev->myUrb[i]->transfer_dma);
        printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

        if (cam_dev->myUrb[i]->transfer_buffer == NULL) {
            printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            printk(KERN_WARNING "===usbcam_urbInit: ERROR allocating memory for transfer buffer!\n");
            usb_free_urb(cam_dev->myUrb[i]);
            printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            return -ENOMEM;
        }

        // initializing isochronous urb by hand
        printk(KERN_WARNING "===usbcam_urbInit: initializing isochronous urb: %d\n", i);
        cam_dev->myUrb[i]->dev = cam_dev->usbdev;
        cam_dev->myUrb[i]->context = cam_dev; // *dev* ?? // check struct void*?
        cam_dev->myUrb[i]->pipe = usb_rcvisocpipe(cam_dev->usbdev, endpointDesc.bEndpointAddress);
        cam_dev->myUrb[i]->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
        cam_dev->myUrb[i]->interval = endpointDesc.bInterval;
        cam_dev->myUrb[i]->complete = urbCompletionCallback;
        cam_dev->myUrb[i]->number_of_packets = nbPackets;
        cam_dev->myUrb[i]->transfer_buffer_length = size;

        for (j = 0; j < nbPackets; ++j) {
            printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            cam_dev->myUrb[i]->iso_frame_desc[j].offset = j * myPacketSize;
            cam_dev->myUrb[i]->iso_frame_desc[j].length = myPacketSize;
        }
        printk(KERN_WARNING "=================\n");
        printk(KERN_WARNING "===usbcam_urbinit: URB# %d pipe value is %d\n", i, cam_dev->myUrb[i]->pipe);
        printk(KERN_WARNING "===usbcam_urbinit: URB# %d transfer flags are %d\n", i, cam_dev->myUrb[i]->transfer_flags);
        printk(KERN_WARNING "===usbcam_urbinit: URB# %d number of packets is %d\n", i, cam_dev->myUrb[i]->number_of_packets);
        printk(KERN_WARNING "===usbcam_urbinit: URB# %d transfer_buffer_length is %d\n", i, cam_dev->myUrb[i]->transfer_buffer_length);
        printk(KERN_WARNING "===usbcam_urbinit: URB# %d interval is %d\n", i, cam_dev->myUrb[i]->interval);

        ret = usb_submit_urb(cam_dev->myUrb[i], GFP_ATOMIC);

        printk(KERN_WARNING "===usbcam_urbinit: SUBMIT URB VALUE is %d\n", ret);

        if (ret < 0) {
            printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            printk(KERN_WARNING "===usbcam_urbInit: ERROR submitting URB: %i\n", ret);
            return ret;
        }
        printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

    }
/*
    printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
    for(i = 0; i < nbUrbs; i++) {
        printk(KERN_WARNING "===usbcam_urbinit: usb_submit_urb is after this (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
        //ret = usb_submit_urb(cam_dev->myUrb[i], GFP_ATOMIC);

        //printk(KERN_WARNING "===usbcam_urbinit: SUBMIT URB VALUE is %d\n", ret);

        if (ret < 0) {
            printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            printk(KERN_WARNING "===usbcam_urbInit: ERROR submitting URB: %i\n", ret);
            return ret;
        }
        //printk(KERN_WARNING "===usbcam_urbinit: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
    }
    */
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
    printk(KERN_WARNING "===usbcam_CALLBACK: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);

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
            printk(KERN_WARNING "===usbcam_CALLBACK: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            if ((ret = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
                printk(KERN_WARNING "===usbcam_urbCompletionCallback: ERROR submitting URB: %i\n", ret);
            }
        }

        else {
            printk(KERN_WARNING "===usbcam_CALLBACK: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
            atomic_inc(&cam_dev->urbCounter); // increment counter +1
            urbCounterTotal = (int) atomic_read(&cam_dev->urbCounter);
            if(urbCounterTotal == 5) {
                printk(KERN_WARNING "===usbcam_CALLBACK: (%s,%s,%u)\n",__FILE__,__FUNCTION__,__LINE__);
                // unlock semaphore
            //    up(&cam_dev->SemURB);

                // reset urbCounter
                atomic_set(&cam_dev->urbCounter, 0);

                // set flag_done
                flag_done = 1;
            }
        }
    }
    else {
        printk(KERN_WARNING "===usbcam_urbCompletionCallback: ERROR completing urb: %i\n", urb->status);
    }
}
