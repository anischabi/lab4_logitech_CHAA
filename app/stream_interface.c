#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <SDL2/SDL.h>
#include "../driver/ioctl_cmds.h"

// Your current resolution:
#define WIDTH 160
#define HEIGHT 120
#define FRAME_SIZE (WIDTH * HEIGHT * 2)

static void yuyv_to_rgb(uint8_t *yuyv, uint8_t *rgb)
{
    for (int i = 0, j = 0; i < FRAME_SIZE; i += 4, j += 6) {
        int Y1 = yuyv[i + 0];
        int U  = yuyv[i + 1] - 128;
        int Y2 = yuyv[i + 2];
        int V  = yuyv[i + 3] - 128;

        int R1 = Y1 + 1.402 * V;
        int G1 = Y1 - 0.344 * U - 0.714 * V;
        int B1 = Y1 + 1.772 * U;

        int R2 = Y2 + 1.402 * V;
        int G2 = Y2 - 0.344 * U - 0.714 * V;
        int B2 = Y2 + 1.772 * U;

        rgb[j+0] = (R1 < 0 ? 0 : (R1 > 255 ? 255 : R1));
        rgb[j+1] = (G1 < 0 ? 0 : (G1 > 255 ? 255 : G1));
        rgb[j+2] = (B1 < 0 ? 0 : (B1 > 255 ? 255 : B1));

        rgb[j+3] = (R2 < 0 ? 0 : (R2 > 255 ? 255 : R2));
        rgb[j+4] = (G2 < 0 ? 0 : (G2 > 255 ? 255 : G2));
        rgb[j+5] = (B2 < 0 ? 0 : (B2 > 255 ? 255 : B2));
    }
}

int main()
{
    int fd = open("/dev/camera_stream", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    if (ioctl(fd, IOCTL_STREAMON) < 0) {
        perror("STREAMON");
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("ELE784 Camera",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH * 3, HEIGHT * 3, 0);
    SDL_Renderer* rnd = SDL_CreateRenderer(win, -1, 0);
    SDL_Texture* tex = SDL_CreateTexture(rnd,
        SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT);

    uint8_t yuyv[FRAME_SIZE];
    uint8_t rgb[WIDTH * HEIGHT * 3];

    while (1)
    {
        int ret = read(fd, yuyv, FRAME_SIZE);
        if (ret <= 0) continue;

        yuyv_to_rgb(yuyv, rgb);

        SDL_UpdateTexture(tex, NULL, rgb, WIDTH * 3);

        SDL_RenderClear(rnd);
        SDL_RenderCopy(rnd, tex, NULL, NULL);
        SDL_RenderPresent(rnd);

        SDL_Event e;
        if (SDL_PollEvent(&e) && e.type == SDL_QUIT)
            break;
    }

    ioctl(fd, IOCTL_STREAMOFF);
    close(fd);
    SDL_Quit();
    return 0;
}
