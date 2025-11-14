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
#include <linux/usb.h>
#include <linux/usb/video.h>
#include <linux/completion.h>
#include <linux/uaccess.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "callback.h"
#include "ioctl_cmds.h"
#include "usbvideo.h"

MODULE_LICENSE("GPL");

#define URB_COUNT   5
#define MAX_PACKETS 32

#define PANTILT_UP    0x00
#define PANTILT_DOWN  0x01
#define PANTILT_LEFT  0x02
#define PANTILT_RIGHT 0x03

#define DEV_MINOR       0x00
#define DEV_MINORS      0x01

static struct usb_device_id usb_device_id [] = {
    {USB_DEVICE(0x046d, 0x0837)},
    {USB_DEVICE(0x046d, 0x046d)},
    {USB_DEVICE(0x046d, 0x08c2)},
    {USB_DEVICE(0x046d, 0x08cc)},
    {USB_DEVICE(0x046d, 0x0994)},
    {},
};
MODULE_DEVICE_TABLE(usb, usb_device_id);

static int ele784_open(struct inode *inode, struct file *file);
static long ele784_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t ele784_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos);
static int ele784_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void ele784_disconnect (struct usb_interface *intf);

static struct usb_driver udriver = {
  .name = "logitech_orbit_driver",
  .probe = ele784_probe,
  .disconnect = ele784_disconnect,
  .id_table = usb_device_id,
};

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = ele784_read,
  .open = ele784_open,
  .unlocked_ioctl = ele784_ioctl,
};

static struct usb_class_driver class_stream_driver = {
  .name = "camera_stream",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};

static struct usb_class_driver class_control_driver = {
  .name = "camera_control",
  .fops = &fops,
  .minor_base = DEV_MINOR,
};

// Structure personnelle du Pilote
struct orbit_driver {
  struct usb_device		  *device;
  struct usb_interface	  *interface;
  struct usb_class_driver *class_driver;
  struct urb			  *isoc_in_urb[URB_COUNT];
  struct driver_buffer     frame_buf;
};

enum {USB_CONTROL_INTF, USB_VIDEO_INTF, NUM_INTF};

module_usb_driver(udriver);



