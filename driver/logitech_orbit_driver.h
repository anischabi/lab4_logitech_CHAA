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
#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/completion.h>
#include <linux/uaccess.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "callback.h"
#include "ioctl_cmds.h"
#include "usbvideo.h"

MODULE_LICENSE("GPL");

//URB_COUNT   : Number of USB Request Blocks (URBs) allocated for streaming.
//MAX_PACKETS : Maximum number of packets per URB.
#define URB_COUNT   5
#define MAX_PACKETS 32

#define PANTILT_UP    0x00
#define PANTILT_DOWN  0x01
#define PANTILT_LEFT  0x02
#define PANTILT_RIGHT 0x03
//IOCTL_PANTILT_RELATIVE command
#define PANTILT_RELATIVE_CONTROL (0x01<<8)
#define PANTILT_RELATIVE_INDEX 0x0B00
#define PANTILT_RELATIVE_TIMEOUT 400
// Payload: commande reset propriétaire
#define PANTILT_RESET_CMD     0x03
#define PANTILT_RESET_VALUE   (0x02 << 8)  // Sélecteur propriétaire
#define PANTILT_RESET_INDEX   0x0B00       // Interface vidéo + classe
#define PANTILT_RESET_TIMEOUT 400          // Timeout en ms
// used for IOCTL_STREAMON command
#define PROBE_LENGTH  26

#define DEV_MINOR       0x00
#define DEV_MINORS      0x01


#define GET_CUR  0x81
#define GET_MIN  0x82
#define GET_MAX  0x83
#define GET_RES  0x84
#define GET_DEF  0x87
#define GET_INFO 0x86

//tableau ou chaque element est une struct usb_device_id.
// static symbol cannot be accessed from other .c files : it is private to this compilation unit.
//Because it’s static:
// -it is allocated once
// -it stays in memory for the whole life of the module
// -not on the stack
//Without static, if it were inside a function, it would be temporary and disappear — kernel would not have access to it.
static struct usb_device_id usb_device_id [] = {
    {USB_DEVICE(0x046d, 0x0837)},
    {USB_DEVICE(0x046d, 0x046d)},
    {USB_DEVICE(0x046d, 0x08c2)},
    {USB_DEVICE(0x046d, 0x08cc)},
    {USB_DEVICE(0x046d, 0x0994)},
    {},
};
//MODULE_DEVICE_TABLE lets modprobe auto-load this module if a supported device is plugged in.
MODULE_DEVICE_TABLE(usb, usb_device_id);

static int ele784_open(struct inode *inode, struct file *file);
static long ele784_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t ele784_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos);
static int ele784_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void ele784_disconnect (struct usb_interface *intf);

// Registers the USB driver with the kernel.
// Kernel uses this struct to match devices and call probe or disconnect.
static struct usb_driver udriver = {
  .name = "logitech_orbit_driver",
  .probe = ele784_probe,
  .disconnect = ele784_disconnect,
  .id_table = usb_device_id,
};

// Provides standard character device operations for /dev/camera_control or /dev/camera_stream. (open,read,ioctl)
// .unlocked_ioctl : new version of ioctl that doesn't require the Big Kernel Lock.
static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = ele784_read,
  .open = ele784_open,
  .unlocked_ioctl = ele784_ioctl,
};


// Used for user-space device nodes.
// These create /dev/camera_streamX and /dev/camera_controlX nodes automatically.
// Each USB interface (stream + control) gets a minor number and a /dev node.
// One for video streaming (camera_stream) and one for control commands (camera_control).
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

// Structure personnelle du Pilote : Your private per-device data.
// This is what gets stored in file->private_data.
struct orbit_driver {
  struct usb_device		  *device;
  struct usb_interface	  *interface;
  struct usb_class_driver *class_driver;
  struct urb			  *isoc_in_urb[URB_COUNT];
  struct driver_buffer     frame_buf;
};

enum {USB_CONTROL_INTF, USB_VIDEO_INTF, NUM_INTF};

//The line module_usb_driver(udriver); auto-generates the module init/exit.
module_usb_driver(udriver);
