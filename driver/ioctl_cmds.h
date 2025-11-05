#ifndef IOCTL_H
#define IOCTL_H

#include <linux/ioctl.h>

#define MAGIC_VAL 'R'

#define IOCTL_GET                _IOWR(MAGIC_VAL, 0x10, int)
#define IOCTL_SET                _IOW(MAGIC_VAL, 0x20, int)
#define IOCTL_STREAMON           _IOW(MAGIC_VAL, 0x30, int)
#define IOCTL_STREAMOFF          _IOW(MAGIC_VAL, 0x40, int)
#define IOCTL_PANTILT_RELATIVE   _IOW(MAGIC_VAL, 0x50, int)
#define IOCTL_PANTILT_RESET      _IOW(MAGIC_VAL, 0x60, int)

struct usb_request {
  uint8_t  request;
  uint8_t  data_size;
  uint16_t value;
  uint16_t index;
  uint16_t timeout;
  uint8_t  *data;
};


#endif 
