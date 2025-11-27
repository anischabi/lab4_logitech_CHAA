#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "../driver/ioctl_cmds.h"
#define TIMEOUT 400
#define FRAME_MAX_SIZE   200000   // big enough for 320x240 or 640x480 YUYV
#define  FRAME_SIZE_160x120          38400


int get(int fd, uint8_t request, uint16_t value,
        uint16_t index, uint8_t *buf, uint8_t size)
{
    struct usb_request req;
    req.request   = request;      // GET_CUR / GET_MIN / GET_MAX / ...
    req.data_size = size;
    req.value     = value;        // full 16-bit wValue
    req.index     = index;        // full 16-bit wIndex
    req.timeout   = TIMEOUT;          // ms
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
    rq.timeout   = TIMEOUT;
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
    int fd1 = open("/dev/camera_stream", O_RDWR);
    if (fd1 < 0) {
        perror("open");
        return 1;
    }

    /*********************************************
    * 1) PERFORM RESET
    *********************************************/
    // printf("Sending RESET command #1 ...\n");
    // if (ioctl(fd, IOCTL_PANTILT_RESET, 0) < 0) {
    //     perror("IOCTL_PANTILT_RESET failed");
    // } else {
    //     printf("RESET sent successfully!\n");
    // }
    // sleep(5);
    
    

    /*********************************************
    * PAN / TILT movement tests
    *********************************************/
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

    // printf("\nSending PAN_RELATIVE -4000...\n");
    // rel.pan = -4000;
    // rel.tilt = 0;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan = -4000) failed");
    // } else {
    //     printf("Pan -4000 command sent!\n");
    // }
    // sleep(2);

    // printf("\nSending TILT_RELATIVE = 1000...\n");
    // rel.pan = 0;
    // rel.tilt = 1000;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (tilt = 1000) failed");
    // } else {
    //     printf("tilt 1000 command sent!\n");
    // }
    // sleep(2);

    // printf("\nSending TILT_RELATIVE = -1000...\n");
    // rel.pan = 0;
    // rel.tilt = -1000;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (tilt = -1000) failed");
    // } else {
    //     printf("tilt -1000 command sent!\n");
    // }
    // sleep(2);

    // printf("\nSending PAN =4000 and TILT = 1000...\n");
    // rel.pan = 4000;
    // rel.tilt = 1000;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan =4000 and tilt = 1000) failed");
    // } else {
    //     printf("pan =4000 and tilt = 1000 command sent!\n");
    // }
    // sleep(2);

    // printf("\nSending PAN =-4000 and TILT =-1000 #1...\n");
    // rel.pan = -4000;
    // rel.tilt = -1000;
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan =-4000 and tilt =-1000) #1 failed");
    // } else {
    //     printf("pan = -4000 and tilt = -1000 command #1 sent!\n");
    // }
    // sleep(2);

    // printf("\nSending PAN =-4000 and TILT =-1000 #2 ...\n");
    // if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
    //     perror("IOCTL_PANTILT_RELATIVE (pan = -4000 and tilt = -1000) #2 failed");
    // } else {
    //     printf("pan =4000 and tilt = 1000 command #2 sent!\n");
    // }


    // /*********************************************
    // * 1) PERFORM RESET
    // *********************************************/
    // printf("Sending RESET command #4 ...\n");
    // if (ioctl(fd, IOCTL_PANTILT_RESET, 0) < 0) {
    //     perror("IOCTL_PANTILT_RESET failed");
    // } else {
    //     printf("RESET sent successfully!\n");
    // }
    // sleep(5);
    


    // TO BE REWRITED 
    // /*********************************************
    // * GET/SET gain + pantilt limits
    // *********************************************/    
    // uint8_t gbuf[2]= {0,0};
    // uint8_t buf[4]= {0,0,0,0};
    // uint16_t gain = 0; 

    //to be tested later why value and index are switching from 0x0400 and 0x0300
    // to SELECTOR and INDEX
    // INDEX IS PANTILT_INDEX 

    // // printf("\n--- GAIN GET_CUR ---\n");
    // // get(fd, GET_CUR,  0x0400, 0x0300, gbuf, 2);
    // // uint16_t gain = gbuf[0] | (gbuf[1] << 8);
    // // printf("gain = %u\n", gain);
    // // sleep(1);
    
    // // printf("\n--- SET GAIN = 50 ---\n");
    // // gain = 50;
    // // gbuf[0] = gain & 0xFF;
    // // gbuf[1] = (gain >> 8) & 0xFF;
    // // set(fd, SET_CUR,  0x0400, 0x0300, gbuf, 2);
    // // sleep(1);

    // printf("\n--- GAIN GET_CUR ---\n");
    // get(fd, GET_CUR,  0x0400, 0x0300, gbuf, 2);
    // gain = gbuf[0] | (gbuf[1] << 8);
    // printf("gain = %u\n", gain);
    // sleep(1);
    
    // printf("\n--- GET_CUR ---\n");
    // get(fd, GET_CUR, SELECTOR, INDEX, buf, 4);
    // decode_pantilt(buf);

    // printf("\n--- GET_MIN ---\n");
    // get(fd, GET_MIN, SELECTOR, INDEX, buf, 4);
    // decode_pantilt(buf);

    // printf("\n--- GET_MAX ---\n");
    // get(fd, GET_MAX, SELECTOR, INDEX, buf, 4);
    // decode_pantilt(buf);

    // printf("\n--- GET_RES ---\n");
    // get(fd, GET_RES, SELECTOR, INDEX, buf, 4);
    // decode_pantilt(buf);

    // printf("\n--- GET_DEF ---\n");
    // get(fd, GET_DEF, SELECTOR, INDEX, buf, 4);
    // decode_pantilt(buf);

    /*********************************************
     * STREAMON + READ FRAMES + STREAMOFF
     *********************************************/
    printf("\n==============================\n");
    printf("      IOCTL_STREAMON\n");
    printf("==============================\n");

    if (ioctl(fd1, IOCTL_STREAMON, NULL) < 0) {
        perror("IOCTL_STREAMON failed");
    } else {
        printf("STREAMON successfully submitted .\n");
    }

    // printf("Waiting 3 seconds while URBs run...\n");
    // sleep(3);
  /*********************************************
     * READ N FRAMES
     *********************************************/
    const int NUM_FRAMES = 10;
    uint8_t frame_buffer[FRAME_MAX_SIZE];
    int frame_sizes[NUM_FRAMES];
    int i, ret;

    // Read all frames as fast as possible - NO PRINTS
    for (i = 0; i < NUM_FRAMES; i++) {
        ret = read(fd1, frame_buffer, FRAME_SIZE_160x120);
        frame_sizes[i] = ret;
        
        // Save complete frames immediately
        if (ret == FRAME_SIZE_160x120) {
            char filename[64];
            sprintf(filename, "frame_%02d.yuyv", i);
            FILE *fp = fopen(filename, "wb");
            if (fp) {
                fwrite(frame_buffer, 1, ret, fp);
                fclose(fp);
            }
        }
    }

    // Print all results AFTER reading
    printf("\n==============================\n");
    printf("      FRAME SIZES\n");
    printf("==============================\n");
    for (i = 0; i < NUM_FRAMES; i++) {
        if (frame_sizes[i] == FRAME_SIZE_160x120) {
            printf("Frame %d: %d bytes ✓ (saved)\n", i, frame_sizes[i]);
        } else {
            printf("Frame %d: %d bytes ✗ (incomplete)\n", i, frame_sizes[i]);
        }
    }

    // for (int i = 0; i < NUM_FRAMES; i++)
    // {
    //     printf("Reading frame %d...\n", i);

    //     ssize_t n = read(fd1, framebuf, FRAME_MAX_SIZE);
    //     if (n < 0) {
    //         perror("read failed");
    //         break;
    //     }

    //     printf("  -> Received %ld bytes\n", n);

    //     char fname[64];
    //     snprintf(fname, sizeof(fname), "frame_%02d.yuyv", i);

    //     FILE *f = fopen(fname, "wb");
    //     if (!f) {
    //         perror("fopen");
    //         break;
    //     }

    //     fwrite(framebuf, 1, n, f);
    //     fclose(f);

    //     printf("  -> Saved %s\n", fname);
    // }

    /*********************************************/

    printf("\n==============================\n");
    printf("      IOCTL_STREAMOFF\n");
    printf("==============================\n");

    if (ioctl(fd1, IOCTL_STREAMOFF, NULL) < 0) {
        perror("IOCTL_STREAMOFF failed");
    } else {
        printf("STREAMOFF done.\n");
    }

        
    close(fd);
    close(fd1);
    return 0;
}
