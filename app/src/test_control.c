#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <ioctl_cmds.h>
#define TIMEOUT 400
#define FRAME_MAX_SIZE   (640*480*2)   // big enough for 320x240 or 640x480 YUYV
#define  FRAME_SIZE_160x120          38400
#define FRAME_SIZE_640x480          (640*480*2)  // 640*480*2


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
    printf("Sending RESET command #1 ... ");
    if (ioctl(fd, IOCTL_PANTILT_RESET, 0) < 0) {
        perror("IOCTL_PANTILT_RESET failed");
    } else {
        printf("RESET sent successfully! (sleep 5s)\n");
    }
    sleep(5);
    
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

    /*********************************************
    * READ N FRAMES
    *********************************************/
    const int NUM_FRAMES = 10;
    uint8_t frame_buffer[FRAME_SIZE_640x480];
    int frame_sizes[NUM_FRAMES];
    int i, ret;

    // Read all frames as fast as possible - NO PRINTS
    for (i = 0; i < NUM_FRAMES; i++) {
        ret = read(fd1, frame_buffer, FRAME_SIZE_640x480);
        frame_sizes[i] = ret;
        
        // Save complete frames immediately
        if (ret == FRAME_SIZE_640x480) {
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
        if (frame_sizes[i] == FRAME_SIZE_640x480) {
            printf("Frame %d: %d bytes ✓ (saved)\n", i, frame_sizes[i]);
        } else {
            printf("Frame %d: %d bytes ✗ (incomplete)\n", i, frame_sizes[i]);
        }
    }

    printf("\n==============================\n");
    printf("      IOCTL_STREAMOFF\n");
    printf("==============================\n");
    if (ioctl(fd1, IOCTL_STREAMOFF, NULL) < 0) {
        perror("IOCTL_STREAMOFF failed");
    } else {
        printf("STREAMOFF done. (sleep 5s)\n");
    }
    sleep(5);
    
    /*********************************************
    * PAN / TILT movement tests
    *********************************************/
    struct pantilt_relative rel;
    printf("\nSending PAN_RELATIVE +4000...\n");
    rel.pan = 4000;
    rel.tilt = 0;
    if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
        perror("IOCTL_PANTILT_RELATIVE (pan = +4000) failed");
    } else {
        printf("Pan +4000 command sent!\n");
    }
    sleep(2);

    printf("\nSending PAN_RELATIVE -4000...\n");
    rel.pan = -4000;
    rel.tilt = 0;
    if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
        perror("IOCTL_PANTILT_RELATIVE (pan = -4000) failed");
    } else {
        printf("Pan -4000 command sent!\n");
    }
    sleep(2);

    printf("\nSending TILT_RELATIVE = 1000...\n");
    rel.pan = 0;
    rel.tilt = 1000;
    if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
        perror("IOCTL_PANTILT_RELATIVE (tilt = 1000) failed");
    } else {
        printf("tilt 1000 command sent!\n");
    }
    sleep(2);

    printf("\nSending TILT_RELATIVE = -1000...\n");
    rel.pan = 0;
    rel.tilt = -1000;
    if (ioctl(fd, IOCTL_PANTILT_RELATIVE, &rel) < 0) {
        perror("IOCTL_PANTILT_RELATIVE (tilt = -1000) failed");
    } else {
        printf("tilt -1000 command sent!\n");
    }
    sleep(2);


        
    close(fd);
    close(fd1);
    return 0;

}