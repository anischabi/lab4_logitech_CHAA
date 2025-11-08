#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "ioctl_cmds.h"

struct pantilt_relative {
    int8_t delta_pan;    // 0=stop, 1=cw, 0xFF=ccw
    uint8_t pan_speed;   // 0..255
    int8_t delta_tilt;   // 0=stop, 1=up, 0xFF=down
    uint8_t tilt_speed;  // 0..255
};

int main() {
    int fd = open("/dev/camera_control", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct pantilt_relative stop = {0,0,0, 0};// stop pan and tilt safely
    // Stop after movement
    ioctl(fd, IOCTL_PANTILT_RELATIVE, &stop);
    sleep(1);

    // Test 1: Reset
    printf("Reset caméra...\n");
    ioctl(fd, IOCTL_PANTILT_RESET, 0);
    sleep(2);

    struct pantilt_relative pantilt;

    // Test 2: Mouvement droite
    printf("Pan droite...\n");
    pantilt.delta_pan = 1;    // clockwise
    pantilt.pan_speed = 1;    // minimum speed
    pantilt.delta_tilt = 0;   // no tilt
    pantilt.tilt_speed = 0;
    ioctl(fd, IOCTL_PANTILT_RELATIVE, &pantilt);
    sleep(1);

    // Test 3: Mouvement haut
    printf("Tilt haut...\n");
    pantilt.delta_pan = 0;    // no pan
    pantilt.pan_speed = 0;
    pantilt.delta_tilt = 1;   // tilt up
    pantilt.tilt_speed = 1;   // minimum speed
    ioctl(fd, IOCTL_PANTILT_RELATIVE, &pantilt);
    sleep(1);

    // Test 4: Reset final
    printf("Reset final...\n");
    ioctl(fd, IOCTL_PANTILT_RESET, 0);

    close(fd);
    return 0;
}