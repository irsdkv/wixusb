/*
 * The MIT License
 *
 * Copyright 2017 Ildar Sadykov.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/usb/ch9.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include "wixusb_driver_types.h"
#include "wixusb_ioctl.h"

#ifndef DEBUG
#define DEBUG
#endif

#ifndef WIXUSB_DRV_NAME
#define WIXUSB_DRV_NAME              "WIXUSB"
#endif
#ifndef WIXUSB_DEV_NAME
#define WIXUSB_DEV_NAME              "wixusb-dev"
#endif

#ifndef VENDOR_ID
#define PID_CODES_VID                   0x1209 // http://pid.codes/1209/
#define VENDOR_ID                       PID_CODES_VID
#endif
#define WIXUSB_PREFIX                   WIXUSB_DRV_NAME "> "

#define USB_SKEL_MINOR_BASE             0

#define WIXUSB_BUFFSIZE              4096

#define EP_INT_NUM                 (0x01)
#define EP_INT_IN_ADDR             ((unsigned int)((USB_DIR_IN) | (EP_INT_NUM)))
#define EP_INT_OUT_ADDR            ((unsigned int)((USB_DIR_OUT) | (EP_INT_NUM)))

#define PIPE_INT_IN(usbdev)        usb_rcvintpipe(usbdev, (EP_INT_IN_ADDR))
#define PIPE_INT_OUT(usbdev)       usb_sndintpipe(usbdev, (EP_INT_OUT_ADDR))

#define EP_BULK_IN_NUM             (0x02)
#define EP_BULK_OUT_NUM            (0x03)
#define EP_BULK_IN_ADDR            ((unsigned int)((USB_DIR_IN) | (EP_BULK_IN_NUM)))
#define EP_BULK_OUT_ADDR           ((unsigned int)((USB_DIR_OUT) | (EP_BULK_OUT_NUM)))

#define PIPE_BULK_IN(usbdev)       usb_rcvbulkpipe(usbdev, (EP_BULK_IN_ADDR))
#define PIPE_BULK_OUT(usbdev)      usb_sndbulkpipe(usbdev, (EP_BULK_OUT_ADDR))

#define to_wixusb_dev(d)                  container_of(d, struct usb_wixusb, kref)

#ifdef DEBUG
#define wixusb_log(fmt, ...)             printk(KERN_DEBUG WIXUSB_PREFIX pr_fmt(fmt), ##__VA_ARGS__)
#else
#define wixusb_log(fmt, ...)             pr_devel(WIXUSB_PREFIX pr_fmt(fmt),##__VA_ARGS__)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
#error "Kernel version too old :("
#endif

static struct usb_device_id wixusb_table[];

struct usb_wixusb {
    struct usb_device *usbdev; /* the usb device for this device */
    struct usb_interface *interface; /* the interface for this device */
    struct mutex io_mutex; /* synchronize I/O with disconnect */
    struct kref kref;
    unsigned int timeout;
    atomic_t open_counter;
    __u16 idProduct;
};

static struct usb_driver wixusb_driver;

static int
wixusb_open(struct inode *inode, struct file *file) {
    struct usb_wixusb *dev;
    struct usb_interface *interface;
    int subminor;
    int retval = 0;

    subminor = iminor(inode);

    interface = usb_find_interface(&wixusb_driver, subminor);
    if (!interface)
    {
        pr_err("%s - error, can't find device for minor %d\n",
            __func__, subminor);
        retval = -ENODEV;
        goto exit;
    }

    dev = usb_get_intfdata(interface);
    if (!dev)
    {
        wixusb_log("Error: wixusb_open : usb_get_intfdata");
        retval = -ENODEV;
        goto exit;
    }
    
    /* increment our usage count for the device */
    kref_get(&dev->kref);

    dev->timeout = 0;

    /* save our object in the file's private structure */
    file->private_data = dev;

exit:
    wixusb_log("wixusb_open : (%d)", retval);
    return retval;

}

static ssize_t
wixusb_read(struct file *file, char *buffer, size_t count,
    loff_t *ppos) {
    struct usb_wixusb *dev;
    int retval = 0;
    int actual_length = count;
    char *buf = NULL;

    /* verify that we actually have some data to read */
    if (count == 0)
        goto exit;

    dev = file->private_data;

    /* no concurrent readers */
    retval = mutex_lock_interruptible(&dev->io_mutex);
    if (retval < 0)
        return retval;

    if (!dev->interface)
    {
        retval = -ENODEV;
        goto error;
    }

    buf = kmalloc(count, GFP_KERNEL);
    if (!buf)
    {
        retval = -ENOMEM;
        goto error;
    }

    retval = usb_bulk_msg(dev->usbdev,
        PIPE_BULK_IN(dev->usbdev), buf, count,
        &actual_length, dev->timeout);

    if (retval)
    {
        goto error_free;
    }

    if (copy_to_user(buffer, buf, actual_length))
    {
        retval = -EFAULT;
        goto error_free;
    }

    mutex_unlock(&dev->io_mutex);
    kfree(buf);

exit:
    wixusb_log("wixusb_read : success (%d), received %d bytes", retval, actual_length);
    return actual_length;

error_free:
    kfree(buf);
error:
    mutex_unlock(&dev->io_mutex);
    wixusb_log("wixusb_read : fail (%d)", retval);
    return retval;
}

static ssize_t
wixusb_write(struct file *file, const char *user_buffer,
    size_t count, loff_t *ppos) {
    int retval = 0;
    struct usb_wixusb *dev;
    char *buf = NULL;
    int actual_length;
    ssize_t writed_size = 0;

    /* verify that we actually have some data to write */
    if (count == 0)
        goto exit;

    dev = file->private_data;

    /* this lock makes sure we don't submit URBs to gone devices */
    mutex_lock(&dev->io_mutex);
    if (!dev->interface) /* disconnect() was called */
    {
        retval = -ENODEV;
        goto error;
    }

    if (count > WIXUSB_BUFFSIZE)
    {
        retval = -ENOMEM;
        goto error;
    }

    buf = kmalloc(count, GFP_KERNEL);
    if (!buf)
    {
        retval = -ENOMEM;
        goto error;
    }

    if (copy_from_user(buf, user_buffer, count))
    {
        retval = -EFAULT;
        goto error_free;
    }

    retval = usb_bulk_msg(dev->usbdev,
        PIPE_BULK_OUT(dev->usbdev), buf, count,
        &actual_length, dev->timeout);


    if (retval)
    {
        goto error_free;
    }

    writed_size = actual_length;

    if (!(count % EP_SIZE))
    {
        retval = usb_bulk_msg(dev->usbdev,
            PIPE_BULK_OUT(dev->usbdev), buf, 0,
            &actual_length, dev->timeout);

        if (retval)
        {
            goto error_free;
        }
    }

    mutex_unlock(&dev->io_mutex);
    kfree(buf);
    wixusb_log("wixusb_write : success (%zd)", writed_size);
    return writed_size;

error_free:
    kfree(buf);
error:
    mutex_unlock(&dev->io_mutex);
exit:
    wixusb_log("wixusb_write : nothing to read (%d)", retval);
    return retval;
}

static void
wixusb_delete(struct kref *kref) {
    struct usb_wixusb *dev = to_wixusb_dev(kref);

    usb_put_dev(dev->usbdev);

    kfree(dev);
}

static int
wixusb_release(struct inode *inode, struct file *file) {
    int retval = 0;
    struct usb_wixusb *dev;

    dev = file->private_data;
    if (dev == NULL)
        return -ENODEV;

    kref_put(&dev->kref, wixusb_delete);

    wixusb_log("wixusb_release : (%d)", retval);
    return retval;
}

static int
wixusb_flush(struct file *file, fl_owner_t id) {
    return 0;
}

long
wixusb_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    long retval = 0;
    struct usb_wixusb *dev;
    char * buff = NULL;

    dev = file->private_data;

    mutex_lock(&dev->io_mutex);
    
    if (!dev->interface)
    {
        retval = -ENODEV;
        goto error_no_dev;
    }

    if ((_IOC_TYPE(cmd) != WIXUSB_IOC_MAGIC))
    {
        wixusb_log("wixusb_ioctl : wrong MAGIC (%u)", _IOC_NR(cmd));
        mutex_unlock(&dev->io_mutex);
        return -ENOTTY;
    }
    wixusb_log("wixusb_ioctl : enter with %u", _IOC_NR(cmd));
    switch (cmd)
    {
        case IOCTL_SEND_CTRL:
        {
            wixusb_ctrl_packet_t * ctrl_packet;

            buff = kmalloc(sizeof (wixusb_ctrl_packet_t), GFP_KERNEL);
            if (!buff)
            {
                retval = -ENOMEM;
                break;
            }

            if (copy_from_user(buff, (void*) arg, sizeof (wixusb_ctrl_packet_t)))
            {
                retval = -EFAULT;
                break;
            }

            ctrl_packet = (wixusb_ctrl_packet_t *) buff;
            retval = usb_control_msg(dev->usbdev, usb_sndctrlpipe(dev->usbdev, 0),
                ctrl_packet->winusb_packet.Request, ctrl_packet->winusb_packet.RequestType,
                ctrl_packet->winusb_packet.Value, ctrl_packet->winusb_packet.Index,
                ctrl_packet->data, ctrl_packet->winusb_packet.Length, dev->timeout);
            break;
        }
        case IOCTL_RECV_CTRL:
        {
            wixusb_ctrl_packet_t * ctrl_packet;

            buff = kmalloc(sizeof (wixusb_ctrl_packet_t), GFP_KERNEL);
            if (!buff)
            {
                retval = -ENOMEM;
                break;
            }

            if (copy_from_user(buff, (void*) arg, sizeof (wixusb_ctrl_packet_t)))
            {
                retval = -EFAULT;
                break;
            }

            ctrl_packet = (wixusb_ctrl_packet_t *) buff;
            retval = usb_control_msg(dev->usbdev, usb_sndctrlpipe(dev->usbdev, 0),
                ctrl_packet->winusb_packet.Request, ctrl_packet->winusb_packet.RequestType,
                ctrl_packet->winusb_packet.Value, ctrl_packet->winusb_packet.Index,
                ctrl_packet->data, ctrl_packet->winusb_packet.Length, dev->timeout);
            if (retval < 0)
                break;

            if (copy_to_user(((wixusb_ctrl_packet_t *) arg)->data, ctrl_packet->data, retval))
            {
                retval = -EFAULT;
                break;
            }
            break;
        }
        case IOCTL_GET_DESC:
        {
            wixusb_get_desc_t * desc;

            buff = kmalloc(sizeof (wixusb_get_desc_t), GFP_KERNEL);
            if (!buff)
            {
                retval = -ENOMEM;
                break;
            }

            if (copy_from_user(buff, (void*) arg, sizeof (wixusb_get_desc_t)))
            {
                retval = -EFAULT;
                break;
            }

            desc = (wixusb_get_desc_t *) buff;
            retval = usb_get_descriptor(dev->usbdev, desc->desc_type, desc->desc_idx, desc->data, DESC_BUFF_LENGTH);
            if (retval < 0)
                break;
            if (copy_to_user(((wixusb_get_desc_t *) arg)->data, desc->data, retval))
            {
                retval = -EFAULT;
                break;
            }
            break;
        }
        case IOCTL_SET_PIPE_POL:
        {
            wixusb_set_pipe_policy_t * policy;

            buff = kmalloc(sizeof (wixusb_set_pipe_policy_t), GFP_KERNEL);
            if (!buff)
            {
                retval = -ENOMEM;
                break;
            }

            if (copy_from_user(buff, (void*) arg, sizeof (wixusb_set_pipe_policy_t)))
            {
                retval = -EFAULT;
                break;
            }

            policy = (wixusb_set_pipe_policy_t*) buff;
            switch (policy->policy_type)
            {
                case SHORT_PACKET_TERMINATE:
                    retval = 0;
                    break;
                case PIPE_TRANSFER_TIMEOUT:
                    dev->timeout = policy->policy_value;
                    retval = 0;
                    break;
                default:
                    retval = -EINVAL;
                    break;
            }
            break;
        }
        case IOCTL_GET_VID_PID:
        {
            wixusb_vid_pid_t vidpid = {
                .pid = dev->idProduct,
                .vid = VENDOR_ID
            };

            if (copy_to_user(((void *) arg), &vidpid, sizeof (vidpid)))
            {
                retval = -EFAULT;
                break;
            }
            break;
        }
        case IOCTL_IS_CONNECTED:
        {
            retval = 0;
            break;
        }
        case IOCTL_WRITE_INT:
        {
            int actual_length;
            wixusb_intrpt_packet * intrpt_packet;

            buff = kmalloc(sizeof (wixusb_intrpt_packet), GFP_KERNEL);
            if (!buff)
            {
                retval = -ENOMEM;
                break;
            }

            if (copy_from_user(buff, (void*) arg, sizeof (wixusb_intrpt_packet)))
            {
                retval = -EFAULT;
                break;
            }
            intrpt_packet = (wixusb_intrpt_packet*) buff;
            retval = usb_interrupt_msg(dev->usbdev,
                PIPE_INT_OUT(dev->usbdev), intrpt_packet->data, intrpt_packet->length,
                &actual_length, dev->timeout);
            break;
        }
        default:
            retval = -ENOTTY;
            break;
    }

    mutex_unlock(&dev->io_mutex);

    if (buff != NULL)
        kfree(buff);
    wixusb_log("wixusb_ioctl : (%ld)", retval);
    if (retval < 0)
        wixusb_log("wixusb_ioctl : failed ioctl %d", _IOC_NR(cmd));
    return retval;
error_no_dev:
    mutex_unlock(&dev->io_mutex);
    wixusb_log("wixusb_ioctl : no dev ioctl %d", _IOC_NR(cmd));
    return retval;
}

static const struct file_operations wixusb_fops = {
    .owner = THIS_MODULE,
    .read = wixusb_read,
    .write = wixusb_write,
    .open = wixusb_open,
    .release = wixusb_release,
    .flush = wixusb_flush,
    .llseek = noop_llseek,
    .unlocked_ioctl = wixusb_ioctl,
    .compat_ioctl = wixusb_ioctl,
};

static struct usb_class_driver wixusb_class_driver = {
    .name = WIXUSB_DEV_NAME "%d",
    .fops = &wixusb_fops,
    .minor_base = USB_SKEL_MINOR_BASE,
};

static int
wixusb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_wixusb *dev;
    struct usb_host_interface *iface_desc;
    int retval = -ENOMEM;

    /* allocate memory for our device state and initialize it */
    dev = kzalloc(sizeof (*dev), GFP_KERNEL);
    if (!dev)
        goto error;

    kref_init(&dev->kref);
    mutex_init(&dev->io_mutex);
    dev->usbdev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;

    /* set up the endpoint information */
    /* use interrupt-in and interrupt-out endpoints */
    iface_desc = interface->cur_altsetting;

    /* save our data pointer in this interface device */
    usb_set_intfdata(interface, dev);

    /* we can register the device now, as it is ready */
    retval = usb_register_dev(interface, &wixusb_class_driver);
    if (retval)
    {
        /* something prevented us from registering this driver */
        dev_err(&interface->dev,
            "Not able to get a minor for this device.\n");
        usb_set_intfdata(interface, NULL);
        goto error;
    }
    atomic_set(&dev->open_counter, 0);
    dev->idProduct = id->idProduct;
    /* let the user know what node this device is now attached to */
    dev_info(&interface->dev,
        "WixUSB (%04X:%04X) device now attached to " WIXUSB_DEV_NAME "%d",
        id->idVendor, id->idProduct, interface->minor);
    return 0;

error:
    if (dev)
        /* this frees allocated memory */
        kref_put(&dev->kref, wixusb_delete);
    return retval;
}


static void
wixusb_disconnect(struct usb_interface *interface) {
    struct usb_wixusb *dev;
    int minor = interface->minor;

    dev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    /* give back our minor */
    usb_deregister_dev(interface, &wixusb_class_driver);

    /* prevent more I/O from starting */
    mutex_lock(&dev->io_mutex);
    dev->interface = NULL;
    mutex_unlock(&dev->io_mutex);

    kref_put(&dev->kref, wixusb_delete);
    wixusb_log("WIXUSB #%d now disconnected", minor);
}

static struct usb_driver wixusb_driver = {
    .name = WIXUSB_DRV_NAME,
    .probe = wixusb_probe,
    .disconnect = wixusb_disconnect,
    .id_table = wixusb_table,
};

#ifdef DEBUG

static int __init
wixusb_module_init(void) {
    int err;

    err = usb_register(&wixusb_driver);
    if (err) goto fail;

    wixusb_log("WIXUSB driver registered.\n");
    return 0;

fail:
    wixusb_log("WIXUSB driver register error. Exit code = %d.\n", err);
    return err;
}

static void __exit
wixusb_module_exit(void) {
    usb_deregister(&wixusb_driver);
    wixusb_log("WIXUSB driver deregistered.\n");
}

module_init(wixusb_module_init);
module_exit(wixusb_module_exit);

#else

module_usb_driver(wixusb_driver);

#endif

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Ildar Sadykov <irsdkv@gmail.com>");
MODULE_DESCRIPTION("WixUSB USB Driver");
MODULE_ALIAS("WixUSB driver");
MODULE_VERSION("0.9999");

static struct usb_device_id wixusb_table[] = {
    { USB_DEVICE(VENDOR_ID, 0x0001) },
    {}
};

MODULE_DEVICE_TABLE(usb, wixusb_table);
