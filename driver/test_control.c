#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "usbvideo.h"
#include "ioctl_cmds.h"

#define SELECTOR 0x0100
#define ZOOM_SELECTOR 0x0F00
#define INDEX 0x0B00


int get(int fd, uint8_t request, uint16_t value,
        uint16_t index, uint8_t *buf, uint8_t size)
{
    struct usb_request req;
    req.request   = request;      // GET_CUR / GET_MIN / GET_MAX / ...
    req.data_size = size;
    req.value     = value;        // full 16-bit wValue
    req.index     = index;        // full 16-bit wIndex
    req.timeout   = 300;          // ms
    req.data      = buf;          // pointer where the kernel writes the result

    if (ioctl(fd, IOCTL_GET, &req) < 0) {
        perror("IOCTL_GET failed");
        return -1;
    }
    return 0;
}

void set(int fd, uint8_t request, uint16_t value, uint16_t index,
         uint8_t *payload, uint8_t size)
{
    struct usb_request rq;
    rq.request   = request;
    rq.value     = value;
    rq.index     = index;
    rq.timeout   = 300;
    rq.data_size = size;
    rq.data      = payload;

    if (ioctl(fd, IOCTL_SET, &rq) < 0)
        perror("IOCTL_SET failed");
}

static void decode_pantilt(uint8_t *b)
{
    int16_t pan  = b[0] | (b[1] << 8);
    int16_t tilt = b[2] | (b[3] << 8);

    printf(" pan=%d  tilt=%d\n", pan, tilt);
}


int main() {
    int fd = open("/dev/camera_control", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /*********************************************
     * 1) PERFORM RESET
     *********************************************/
    printf("Sending RESET command...\n");

    if (ioctl(fd, IOCTL_PANTILT_RESET, 0) < 0) {
        perror("IOCTL_PANTILT_RESET failed");
    } else {
        printf("RESET sent successfully!\n");
    }
    sleep(2);
    
    // /*********************************************
    //  * 2) SEND PAN_RELATIVE = +4000 (tilt = 0)
    //  *********************************************/
    // struct pantilt_relative rel;
    // printf("\nSending PAN_RELATIVE +4000...\n");
    // rel.pan = 4000;
    // rel.tilt = 0;

    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan = +4000) failed");
    // } else {
    //     printf("Pan +4000 command sent!\n");
    // }
    // sleep(2);

    // // /*********************************************
    // //  * 3) SEND PAN_RELATIVE = -4000 (tilt = 0)
    // //  *********************************************/
    // printf("\nSending PAN_RELATIVE -4000...\n");
    // rel.pan = -4000;
    // rel.tilt = 0;

    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan = -4000) failed");
    // } else {
    //     printf("Pan -4000 command sent!\n");
    // }
    // sleep(2);


    // // /*********************************************
    // //  * 4) SEND tilt_RELATIVE = 1000 (PAN= 0)
    // //  *********************************************/
    // printf("\nSending TILT_RELATIVE = 1000...\n");
    // rel.pan = 0;
    // rel.tilt = 1000;

    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (tilt = 1000) failed");
    // } else {
    //     printf("tilt 1000 command sent!\n");
    // }
    // sleep(2);

    // // /*********************************************
    // //  * 5) SEND tilt_RELATIVE = -1000 (PAN= 0)
    // //  *********************************************/
    // printf("\nSending TILT_RELATIVE = -1000...\n");
    // rel.pan = 0;
    // rel.tilt = -1000;

    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (tilt = -1000) failed");
    // } else {
    //     printf("tilt -1000 command sent!\n");
    // }
    // sleep(1);

    // // /*********************************************
    // //  * 6) SEND Pan = 4000 and tilt =1000
    // //  *********************************************/
    // printf("\nSending PAN =4000 and TILT = 1000...\n");
    // rel.pan = 4000;
    // rel.tilt = 1000;

    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan =4000 and tilt = 1000) failed");
    // } else {
    //     printf("pan =4000 and tilt = 1000 command sent!\n");
    // }
    // sleep(1);

    // // /*********************************************
    // //  * 7) SEND Pan = -4000 and tilt =-1000
    // //  *********************************************/
    // printf("\nSending PAN =-4000 and TILT =-1000 #1...\n");
    // rel.pan = -4000;
    // rel.tilt = -1000;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan =-4000 and tilt =-1000) #1 failed");
    // } else {
    //     printf("pan = -4000 and tilt = -1000 command #1 sent!\n");
    // }
    // sleep(1);

    // /*********************************************
    //  * 8) SEND Pan = -4000 and tilt =-1000
    //  *********************************************/
    // printf("\nSending PAN =-4000 and TILT =-1000 #2 ...\n");
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan = -4000 and tilt = -1000) #2 failed");
    // } else {
    //     printf("pan =4000 and tilt = 1000 command #2 sent!\n");
    // }

    
    uint8_t gbuf[2]= {0,0};
    uint8_t buf[4]= {0,0,0,0};

    printf("\n--- GAIN GET_CUR ---\n");
    get(fd, GET_CUR,  0x0400, 0x0300, gbuf, 2);
    uint16_t gain = gbuf[0] | (gbuf[1] << 8);
    printf("gain = %u\n", gain);
    sleep(1);
    
    printf("\n--- SET GAIN = 50 ---\n");
    gain = 50;
    gbuf[0] = gain & 0xFF;
    gbuf[1] = (gain >> 8) & 0xFF;
    set(fd, SET_CUR,  0x0400, 0x0300, gbuf, 2);
    sleep(1);

    printf("\n--- GAIN GET_CUR ---\n");
    get(fd, GET_CUR,  0x0400, 0x0300, gbuf, 2);
    gain = gbuf[0] | (gbuf[1] << 8);
    printf("gain = %u\n", gain);
    sleep(1);
    
    printf("\n--- GET_CUR ---\n");
    get(fd, GET_CUR, SELECTOR, INDEX, buf, 4);
    decode_pantilt(buf);

    printf("\n--- GET_MIN ---\n");
    get(fd, GET_MIN, SELECTOR, INDEX, buf, 4);
    decode_pantilt(buf);

    printf("\n--- GET_MAX ---\n");
    get(fd, GET_MAX, SELECTOR, INDEX, buf, 4);
    decode_pantilt(buf);

    printf("\n--- GET_RES ---\n");
    get(fd, GET_RES, SELECTOR, INDEX, buf, 4);
    decode_pantilt(buf);

    printf("\n--- GET_DEF ---\n");
    get(fd, GET_DEF, SELECTOR, INDEX, buf, 4);
    decode_pantilt(buf);
        
    close(fd);
    return 0;
}
