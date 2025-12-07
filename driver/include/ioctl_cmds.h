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
#define IOCTL_PANTILT_GET_INFO   _IOR(MAGIC_VAL, 0x70, int)  // NEW
#define IOCTL_PANTILT_GET_CAPS   _IOR(MAGIC_VAL, 0x80, int)  // NEW

struct usb_request {
  uint8_t  request; // GET_CUR = 0x81, SET_CUR = 0x01, GET_MIN, GET_MAX, ...
  uint8_t  data_size; // wLength (payload size)
  uint16_t value; // wValue   (selector << 8)
  uint16_t index; //// wValue   (selector << 8)
  uint16_t timeout;  // timeout in ms
  uint8_t  *data; // pointer to user buffer
};

// Structure for proprietaire relative pan/tilt command
struct pantilt_relative {
  int16_t pan;   // signed 16-bit, little endian
  int16_t tilt;  // signed 16-bit, little endian
};

#endif 
