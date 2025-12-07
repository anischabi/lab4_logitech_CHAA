#ifndef USB_STRUCTS_H
#define USB_STRUCTS_H

#include <linux/kernel.h>    // printk
#include <linux/types.h>     // uint8_t, uint16_t, uint32_t
#include <linux/string.h>    // memcpy, memset
#include <linux/usb.h>       // USB control request constants

//from usbvideo.h
// Video Interface Class Codes
#define CC_VIDEO                                   0x0E

// Video Interface Subclass Codes
#define SC_UNDEFINED                               0x00
#define SC_VIDEOCONTROL                            0x01
#define SC_VIDEOSTREAMING                          0x02
#define SC_VIDEO_INTERFACE_COLLECTION              0x03

#define SET_CUR  0x01
#define GET_CUR  0x81
#define GET_MIN  0x82
#define GET_MAX  0x83
#define GET_RES  0x84
#define GET_DEF  0x87
#define GET_INFO 0x86
#define GET_DEF  0x87

#define SELECTOR 0x0100
// #define FRAME_SIZE 153600   // 320x240 YUYV

#define TIMEOUT 4000

#define GET_CONTROL                   (0x01<<8)
#define PANTILT_RELATIVE_CONTROL      (0x01<<8)
#define PANTILT_RESET_CMD             0x03
#define PANTILT_RESET_CONTROL         (0x02 << 8)
#define PANTILT_INDEX                 0x0B00

#define VS_PROBE_CONTROL_VALUE        (0x01 << 8)
#define VS_PROBE_CONTROL_WINDEX_LE    0x0001
#define VS_COMMIT_CONTROL_VALUE       (0x02 << 8)
#define VS_COMMIT_CONTROL_WINDEX_LE   0x0001

#define VS_PROBE_CONTROL_SIZE   (26u)
// #define VS_PROBE_CONTROL_SIZE   (34u)

// UVC VS Probe structure (34 bytes)
struct vs_probe_control {
    uint16_t    bmHint;
    uint8_t     bFormatIndex;
    uint8_t     bFrameIndex;
    uint32_t    dwFrameInterval;
    uint16_t    wKeyFrameRate;
    uint16_t    wPFrameRate;
    uint16_t    wCompQuality;
    uint16_t    wCompWindowSize;
    uint16_t    wDelay;
    uint32_t    dwMaxVideoFrameSize;
    uint32_t    dwMaxPayloadTransferSize;
    uint32_t    dwClockFrequency;
    uint8_t     bmFramingInfo;
    uint8_t     bPreferedVersion;
    uint8_t     bMinVersion;
    uint8_t     bMaxVersion;
};



enum VS_PROBE_CONTROL_OFFSETS {
    bmHint                   =  0,
    bFormatIndex             =  2,
    bFrameIndex              =  3,
    dwFrameInterval          =  4,
    wKeyFrameRate            =  8,
    wPFrameRate              = 10,
    wCompQuality             = 12,
    wCompWindowSize          = 14,
    wDelay                   = 16,
    dwMaxVideoFrameSize      = 18,
    dwMaxPayloadTransferSize = 22,
    dwClockFrequency         = 26,
    bmFramingInfo            = 30,
    bPreferedVersion         = 31,
    bMinVersion              = 32,
    bMaxVersion              = 33,
};

// Little-endian serialization helpers
#define PUT_U16_LE(bytes, offset, val) do { \
    (bytes)[(offset)] = (val) & 0xFF; \
    (bytes)[(offset)+1] = ((val) >> 8) & 0xFF; \
} while(0)

#define PUT_U32_LE(bytes, offset, val) do { \
    (bytes)[(offset)] = (val) & 0xFF; \
    (bytes)[(offset)+1] = ((val) >> 8) & 0xFF; \
    (bytes)[(offset)+2] = ((val) >> 16) & 0xFF; \
    (bytes)[(offset)+3] = ((val) >> 24) & 0xFF; \
} while(0)

// Little-endian deserialization
#define GET_U16_LE(bytes, offset) \
    ((bytes)[(offset)] | ((bytes)[(offset)+1] << 8))

#define GET_U32_LE(bytes, offset) \
    ((bytes)[(offset)] | ((bytes)[(offset)+1] << 8) | \
     ((bytes)[(offset)+2] << 16) | ((bytes)[(offset)+3] << 24))

// Unpack structure from 34-byte buffer
static inline void unpack_probe_control(uint8_t *bytes, struct vs_probe_control *data) {
    data->bmHint = GET_U16_LE(bytes, bmHint);
    data->bFormatIndex = bytes[bFormatIndex];
    data->bFrameIndex = bytes[bFrameIndex];
    data->dwFrameInterval = GET_U32_LE(bytes, dwFrameInterval);
    data->wKeyFrameRate = GET_U16_LE(bytes, wKeyFrameRate);
    data->wPFrameRate = GET_U16_LE(bytes, wPFrameRate);
    data->wCompQuality = GET_U16_LE(bytes, wCompQuality);
    data->wCompWindowSize = GET_U16_LE(bytes, wCompWindowSize);
    data->wDelay = GET_U16_LE(bytes, wDelay);
    data->dwMaxVideoFrameSize = GET_U32_LE(bytes, dwMaxVideoFrameSize);
    data->dwMaxPayloadTransferSize = GET_U32_LE(bytes, dwMaxPayloadTransferSize);
    data->dwClockFrequency = GET_U32_LE(bytes, dwClockFrequency);
    data->bmFramingInfo = bytes[bmFramingInfo];
    data->bPreferedVersion = bytes[bPreferedVersion];
    data->bMinVersion = bytes[bMinVersion];
    data->bMaxVersion = bytes[bMaxVersion];
}

// Pack structure into 34-byte buffer
static inline void pack_probe_control(struct vs_probe_control *data, uint8_t *bytes) {
    PUT_U16_LE(bytes, bmHint, data->bmHint);
    bytes[bFormatIndex] = data->bFormatIndex;
    bytes[bFrameIndex] = data->bFrameIndex;
    PUT_U32_LE(bytes, dwFrameInterval, data->dwFrameInterval);
    PUT_U16_LE(bytes, wKeyFrameRate, data->wKeyFrameRate);
    PUT_U16_LE(bytes, wPFrameRate, data->wPFrameRate);
    PUT_U16_LE(bytes, wCompQuality, data->wCompQuality);
    PUT_U16_LE(bytes, wCompWindowSize, data->wCompWindowSize);
    PUT_U16_LE(bytes, wDelay, data->wDelay);
    PUT_U32_LE(bytes, dwMaxVideoFrameSize, data->dwMaxVideoFrameSize);
    PUT_U32_LE(bytes, dwMaxPayloadTransferSize, data->dwMaxPayloadTransferSize);
    PUT_U32_LE(bytes, dwClockFrequency, data->dwClockFrequency);
    bytes[bmFramingInfo] = data->bmFramingInfo;
    bytes[bPreferedVersion] = data->bPreferedVersion;
    bytes[bMinVersion] = data->bMinVersion;
    bytes[bMaxVersion] = data->bMaxVersion;
}

// Kernel-print version of your debug function
static inline void print_probe_control_struct(uint8_t *bytes) {

    printk(KERN_INFO "VS PROBE CONTROL content\n");
    printk(KERN_INFO "raw data indexes:\n");

    for (int i = 0; i < VS_PROBE_CONTROL_SIZE; i++)
        printk(KERN_CONT "%02x ", i);

    printk(KERN_INFO "\n---------------------------------------\n");

    for (int i = 0; i < VS_PROBE_CONTROL_SIZE; i++)
        printk(KERN_CONT "%02x ", bytes[i]);

    struct vs_probe_control data;
    unpack_probe_control(bytes, &data);

    printk(KERN_INFO "\nFields decoded:\n");
    printk(KERN_INFO "(00) bmHint = %u\n", data.bmHint);
    printk(KERN_INFO "(02) bFormatIndex = %u\n", data.bFormatIndex);
    printk(KERN_INFO "(03) bFrameIndex = %u\n", data.bFrameIndex);
    printk(KERN_INFO "(04) dwFrameInterval = %u\n", data.dwFrameInterval);
    printk(KERN_INFO "(08) wKeyFrameRate = %u\n", data.wKeyFrameRate);
    printk(KERN_INFO "(10) wPFrameRate = %u\n", data.wPFrameRate);
    printk(KERN_INFO "(12) wCompQuality = %u\n", data.wCompQuality);
    printk(KERN_INFO "(14) wCompWindowSize = %u\n", data.wCompWindowSize);
    printk(KERN_INFO "(16) wDelay = %u\n", data.wDelay);
    printk(KERN_INFO "(18) dwMaxVideoFrameSize = %u\n", data.dwMaxVideoFrameSize);
    printk(KERN_INFO "(22) dwMaxPayloadTransferSize = %u\n", data.dwMaxPayloadTransferSize);
    printk(KERN_INFO "(26) dwClockFrequency = %u\n", data.dwClockFrequency);
    printk(KERN_INFO "(30) bmFramingInfo = %u\n", data.bmFramingInfo);
    printk(KERN_INFO "(31) bPreferedVersion = %u\n", data.bPreferedVersion);
    printk(KERN_INFO "(32) bMinVersion = %u\n", data.bMinVersion);
    printk(KERN_INFO "(33) bMaxVersion = %u\n", data.bMaxVersion);
}

#endif
