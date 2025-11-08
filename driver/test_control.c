#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "ioctl_cmds.h"

struct pantilt_relative {
    int8_t delta_pan;    // 0=stop, 1=cw, 0xFF=ccw
    uint8_t pan_speed;   // 1..255
    int8_t delta_tilt;   // 0=stop, 1=up, 0xFF=down
    uint8_t tilt_speed;  // 1..255
};

int main() {
    int fd = open("/dev/camera_control", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Test 1: Reset
    printf("Reset caméra...\n");
    ioctl(fd, IOCTL_PANTILT_RESET, 0);
    sleep(2);

    struct pantilt_relative pantilt;

    // Test 1: GET_INFO
    printf("Calling IOCTL_PANTILT_GET_INFO...\n");
    if (ioctl(fd, IOCTL_PANTILT_GET_INFO, 0) < 0) {
        perror("IOCTL_PANTILT_GET_INFO failed");
    }
    sleep(1);

    // Test 2: GET_CAPS
    printf("Calling IOCTL_PANTILT_GET_CAPS...\n");
    if (ioctl(fd, IOCTL_PANTILT_GET_CAPS, 0) < 0) {
        perror("IOCTL_PANTILT_GET_CAPS failed");
    }
    sleep(1);

    // Test movement: pan right
    printf("Test mouvement: droite...\n");
    pantilt.delta_pan = 1;       // clockwise
    pantilt.pan_speed = 1;       // required speed
    pantilt.delta_tilt = 0;      // no tilt
    pantilt.tilt_speed = 1;
    ioctl(fd, IOCTL_PANTILT_RELATIVE, &pantilt);
    sleep(3);

    // Test movement: tilt up
    printf("Test mouvement: haut...\n");
    pantilt.delta_pan = 0;
    pantilt.pan_speed = 1;
    pantilt.delta_tilt = 1;      // tilt up
    pantilt.tilt_speed = 1;
    ioctl(fd, IOCTL_PANTILT_RELATIVE, &pantilt);
    sleep(3);

    // Test 4: Reset final
    printf("Reset final...\n");
    ioctl(fd, IOCTL_PANTILT_RESET, 0);
    
    close(fd);
    return 0;
}