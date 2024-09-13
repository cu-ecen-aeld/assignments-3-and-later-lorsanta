/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>      // file_operations
#include <linux/slab.h>    // kzalloc
#include <linux/uaccess.h> // copy_from_user
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("lorsanta"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;
char *remaining_buf = NULL;
size_t remaining_buf_len = 0;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;

    PDEBUG("open");
    /**
     * TODO: handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;

    PDEBUG("release");
    /**
     * TODO: handle release
     */
    dev = filp->private_data;
    return 0;
}


ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev;
    struct aesd_buffer_entry *entry;
    ssize_t retval = 0;
    size_t entry_offset_byte_rtn;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    dev = filp->private_data;
    while(retval < count) {
        entry = aesd_circular_buffer_find_entry_offset_for_fpos(dev->cbuffer, retval + *f_pos, &entry_offset_byte_rtn);
        if(entry) {
            int size = entry->size > count ? entry->size - count : entry->size;
            copy_to_user(buf + retval, entry->buffptr, size);
            retval += size;
        }
        else break;
    }
    *f_pos = retval;
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev;
    struct aesd_buffer_entry entry;
    ssize_t retval = -ENOMEM;
    int start, end;
    char *tmp;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */

    dev = filp->private_data;
    mutex_lock(&dev->mutex);
    tmp = kzalloc(sizeof(char)*(count), GFP_KERNEL);
    copy_from_user(tmp, buf, sizeof(char)*(count));

    for(start = 0, end = 0; end < count; end++)
    {
        if(tmp[end] == '\n')
        {
            int n = remaining_buf_len + (end - start) + 1;
            entry.buffptr = kzalloc(sizeof(char)*n, GFP_KERNEL);
            entry.size = n;
            if(remaining_buf != NULL)
            {
                memcpy(entry.buffptr, remaining_buf, remaining_buf_len);
                kfree(remaining_buf);
                remaining_buf = NULL;
            }
            memcpy(entry.buffptr + remaining_buf_len, tmp + start, (end - start) + 1);
            remaining_buf_len = 0;

            if(dev->cbuffer->entry[dev->cbuffer->in_offs].buffptr != NULL)
                kfree(dev->cbuffer->entry[dev->cbuffer->in_offs].buffptr);

            aesd_circular_buffer_add_entry(dev->cbuffer, &entry);

            start = end + 1;
        }
    }

    if(end - start > 0)
    {
        remaining_buf = krealloc(remaining_buf, sizeof(char)*(remaining_buf_len + end - start), GFP_KERNEL);
        memcpy(remaining_buf + remaining_buf_len, tmp + start, end - start);
        remaining_buf_len += end - start;
    }

    kfree(tmp);
    mutex_unlock(&dev->mutex);

    retval = count;

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_device.cbuffer = kzalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);

    mutex_init(&aesd_device.mutex);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    int i;
    struct aesd_buffer_entry *entry;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    AESD_CIRCULAR_BUFFER_FOREACH(entry, aesd_device.cbuffer, i) {
        if(entry && entry->buffptr)
            kfree(entry->buffptr);
    }
    kfree(aesd_device.cbuffer);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);