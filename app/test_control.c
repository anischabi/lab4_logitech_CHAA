#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "../driver/ioctl_cmds.h"
#include <string.h>

struct usb_request req;
struct usb_request pantilt;

void test_ioctl_get(int fd);
void test_ioctl_set(int fd);
void test_ioctl_pantilt_relative(int fd, uint8_t *bufferPanTilt, size_t size);
void test_ioctl_pantilt_reset(int fd);
void test_ioctl_streamon(int fd);
void test_ioctl_streamoff(int fd);


int main() {
    int fd = open("/dev/camera_control", O_RDWR);
    
    if (fd < 0) {
        perror("Erreur d’ouverture du périphérique");
        return -1;
    }

    // Test 4: IOCTL_PANTILT_RESET
    test_ioctl_pantilt_reset(fd);

    // Test 1: IOCTL_GET
    //test_ioctl_get(fd);

    // Test 2: IOCTL_SET
    //test_ioctl_set(fd);

    // Test 3: IOCTL_PANTILT_RELATIVE
    {/*
        uint8_t bufferPanTilt[4] = {

            // GET MIN : 0x80, 0xEE, 0x80, 0xF8 
            // GET MAX : 0x80, 0x11, 0x80, 0x07 

            0x80,   // bPanRelative
            0x11,   // bPanSpeed
            0x80,   // bTiltRelative
            0x07    // bTiltSpeed
        };

        test_ioctl_pantilt_relative(fd, bufferPanTilt, 4);

        bufferPanTilt[0] = 0x80;
        bufferPanTilt[1] = 0x11;
        bufferPanTilt[2] = 0x80;
        bufferPanTilt[3] = 0x07;

        test_ioctl_pantilt_reset(fd);
        test_ioctl_pantilt_relative(fd, bufferPanTilt, 4);

        bufferPanTilt[0] = 0x80;
        bufferPanTilt[1] = 0x11;
        bufferPanTilt[2] = 0x80;
        bufferPanTilt[3] = 0xF8;

        test_ioctl_pantilt_reset(fd);
        test_ioctl_pantilt_relative(fd, bufferPanTilt, 4);
        
        bufferPanTilt[0] = 0x80;
        bufferPanTilt[1] = 0xEE;
        bufferPanTilt[2] = 0x80;
        bufferPanTilt[3] = 0xF8;
        
        test_ioctl_pantilt_reset(fd);
        test_ioctl_pantilt_relative(fd, bufferPanTilt, 4);
    */}

    // Test 1: IOCTL_PANTILT_RESET
    //test_ioctl_pantilt_reset(fd);
    close(fd);

    // Changement d'interface pour le streaming
    fd = open("/dev/camera_stream", O_RDWR);

    // Test : IOCTL_STREAMON
    test_ioctl_streamon(fd);

    
    
    // Test : IOCTL_STREAMOFF
    //test_ioctl_streamoff(fd);

    return 0;
}

void test_ioctl_get(int fd) {
    printf("Test IOCTL GET...\n");

    memset(&req, 0, sizeof(req));

    uint8_t bufferGet[8] = {0};
    
    req.request   = 0x81; 
    req.value     = 0x01; 
    req.index     = 0x0B; 
    req.timeout   = 5000;
    req.data_size = sizeof(bufferGet);       
    req.data      = bufferGet;   

    ioctl(fd, IOCTL_GET, &req);
    
    printf("Données reçues :\n");
    for (int i = 0; i < sizeof(bufferGet); i++)
        printf("0x%02X ", bufferGet[i]);
    printf("\n");

    sleep(3);
}

void test_ioctl_set(int fd) {
    printf("Test IOCTL SET...\n");
    
    memset(&req, 0, sizeof(req));

    uint8_t bufferSet[4] = {

        // GET MIN : 0x80, 0xEE, 0x80, 0xF8 
        // GET MAX : 0x80, 0x11, 0x80, 0x07 

        0x80,   // bPanRelative
        0x00,   // bPanSpeed
        0x80,   // bTiltRelative
        0x00    // bTiltSpeed
    };

    // === Remplissage de la requête ===
    req.request    = 0x01;            // SET_CUR
    req.value      = 0x01;            // controlSelector = 1
    req.index      = 0x0B;            // bUnitID = 6
    req.timeout    = 5000;
    req.data_size  = sizeof(bufferSet);
    req.data       = bufferSet;

    int ret = ioctl(fd, IOCTL_SET, &req);  // Aucun argument à passer

    if (ret < 0)
        perror("Erreur ioctl IOCTL_SET");
    else
        printf("Commande envoyée avec succès.\n");

    sleep(3);
}

void test_ioctl_pantilt_relative(int fd, uint8_t *bufferPanTilt, size_t size) {

    memset(&req, 0, sizeof(req));

    req.request    = 0x01;            // SET_CUR
    req.value      = 0x01;            // controlSelector = 1
    req.index      = 0x0B;            // bUnitID = 11
    req.timeout    = 5000;
    req.data_size  = 4;
    req.data       = bufferPanTilt;

    int ret = ioctl(fd, IOCTL_PANTILT_RELATIVE, &req);  // Aucun argument à passer

    if (ret < 0)
        perror("Erreur ioctl IOCTL_SET");
    else
        printf("Commande envoyée avec succès.\n");

    sleep(3);
}

void test_ioctl_pantilt_reset(int fd) {
    printf("Test IOCTL RESET...\n");

    int ret = ioctl(fd, IOCTL_PANTILT_RESET);

    if (ret < 0)
        perror("Erreur IOCTL_PANTILT_RESET");
    else
        printf("Commande de reset envoyée avec succès.\n");

    sleep(3);
}

void test_ioctl_streamon(int fd) {
    printf("Test IOCTL STREAMON...\n");

    memset(&req, 0, sizeof(req));
    uint8_t bufferStreamOn[26] = {0};

    /*SET_CUR (set configuration)*/
    req.request    = 0x01;    // SET_CUR
    req.value      = 0x01;    // VS_PROBE_CONTROL
    req.index      = 0x01;    // Interface 1
    req.timeout    = 5000;
    req.data_size  = sizeof(bufferStreamOn);
    req.data       = bufferStreamOn;

    ioctl(fd, IOCTL_SET, &req);  // Aucun argument à passer

    printf("\nSetCur :\n");
    for (int i = 0; i < sizeof(bufferStreamOn); i++)
        printf("0x%02X ", bufferStreamOn[i]);
    printf("\n");
    
    /*GET_CUR (configuration has changed?)*/
    req.request    = 0x81;            // GET_CUR
    req.value      = 0x01;            // VS_PROBE_CONTROL
    req.index      = 0x01;            // Interface 1
    req.timeout    = 5000;
    req.data_size  = sizeof(bufferStreamOn);
    req.data       = bufferStreamOn;

    ioctl(fd, IOCTL_GET, &req);  // Aucun argument à passer

    printf("\nGetCur :\n");
    for (int i = 0; i < sizeof(bufferStreamOn); i++)
        printf("0x%02X ", bufferStreamOn[i]);
    printf("\n");

    /*SET_CUR (commit configuration)*/
    req.request    = 0x01;            // SET_CUR
    req.value      = 0x02;            // VS_COMMIT_CONTROL
    req.index      = 0x01;            // Interface 1
    req.timeout    = 5000;
    req.data_size  = sizeof(bufferStreamOn);
    req.data       = bufferStreamOn;

    ioctl(fd, IOCTL_SET, &req);  // Aucun argument à passer

    printf("\nLast SetCur :\n");
    for (int i = 0; i < sizeof(bufferStreamOn); i++)
        printf("0x%02X ", bufferStreamOn[i]);
    printf("\n");

    ioctl(fd, IOCTL_STREAMON, &req); 

    sleep(3);
}

void test_ioctl_streamoff(int fd) {
    printf("Test IOCTL STREAMOFF...\n");

    int ret = ioctl(fd, IOCTL_STREAMOFF);

    if (ret < 0)
        perror("Erreur IOCTL_STREAMOFF");
    else
        printf("Commande de stream off envoyée avec succès.\n");

    sleep(3);
}