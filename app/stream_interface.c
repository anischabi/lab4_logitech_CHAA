#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../driver/ioctl_cmds.h"

// Resolution settings
#define WIDTH 640
#define HEIGHT 480
#define FRAME_SIZE (WIDTH * HEIGHT * 2)

// Window scaling
#define SCALE 1
#define WINDOW_WIDTH (WIDTH * SCALE)
#define WINDOW_HEIGHT (HEIGHT * SCALE)

// UI constants
#define UI_PANEL_HEIGHT 40
#define STATUS_UPDATE_INTERVAL 500 // ms

typedef struct {
    int fd;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    TTF_Font* font;
    bool running;
    bool paused;
    bool show_info;
    uint32_t frame_count;
    uint32_t fps;
    uint32_t last_fps_time;
    uint32_t last_status_time;
} CameraApp;

static void draw_text(CameraApp* app, const char* text, int x, int y, SDL_Color color)
{
    if (!app->font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(app->font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(app->renderer, surface);
    if (texture) {
        SDL_Rect rect = { x, y, surface->w, surface->h };
        SDL_RenderCopy(app->renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

static void draw_ui(CameraApp* app)
{
    if (!app->show_info) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color green = { 0, 255, 0, 255 };
    SDL_Color red = { 255, 0, 0, 255 };
    SDL_Color yellow = { 255, 255, 0, 255 };
    SDL_Color bg = { 0, 0, 0, 180 };

    // Semi-transparent background for status bar
    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect status_bg = { 0, 0, WINDOW_WIDTH, UI_PANEL_HEIGHT };
    SDL_SetRenderDrawColor(app->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(app->renderer, &status_bg);

    // Status information
    char status_text[256];
    snprintf(status_text, sizeof(status_text), 
             "FPS: %u | Frames: %u | %s | HW-YUV | Press: SPACE=Pause, I=Info, Q/ESC=Quit",
             app->fps, app->frame_count,
             app->paused ? "PAUSED" : "RUNNING");

    draw_text(app, status_text, 10, 10, app->paused ? red : green);

    // Draw resolution info in bottom left
    char res_text[64];
    snprintf(res_text, sizeof(res_text), "%dx%d YUY2", WIDTH, HEIGHT);
    SDL_Rect res_bg = { 0, WINDOW_HEIGHT - 30, 150, 30 };
    SDL_SetRenderDrawColor(app->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(app->renderer, &res_bg);
    draw_text(app, res_text, 10, WINDOW_HEIGHT - 25, yellow);
}

static void update_fps(CameraApp* app)
{
    static uint32_t total_frames = 0;
    uint32_t current_time = SDL_GetTicks();
    
    total_frames++;  // Keep counting total frames
    
    // Update FPS display every second
    if (current_time - app->last_fps_time >= 1000) {
        app->fps = total_frames;
        total_frames = 0;
        app->last_fps_time = current_time;
    }
    
    // Separate counter for total frames displayed in UI
    app->frame_count = total_frames;
}

static void handle_events(CameraApp* app)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app->running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        app->running = false;
                        break;
                    
                    case SDLK_SPACE:
                        app->paused = !app->paused;
                        printf("Camera %s\n", app->paused ? "paused" : "resumed");
                        break;
                    
                    case SDLK_i:
                        app->show_info = !app->show_info;
                        break;
                    
                    case SDLK_f:
                        // Toggle fullscreen
                        {
                            uint32_t flags = SDL_GetWindowFlags(app->window);
                            SDL_SetWindowFullscreen(app->window, 
                                flags & SDL_WINDOW_FULLSCREEN ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                        break;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    app->running = false;
                }
                break;
        }
    }
}

static bool init_camera(CameraApp* app)
{
    app->fd = open("/dev/camera_stream", O_RDWR);
    if (app->fd < 0) {
        perror("Failed to open /dev/camera_stream");
        return false;
    }

    if (ioctl(app->fd, IOCTL_STREAMON) < 0) {
        perror("Failed to start camera stream (IOCTL_STREAMON)");
        close(app->fd);
        app->fd = -1;
        return false;
    }

    printf("Camera stream started successfully\n");
    return true;
}

static bool init_sdl(CameraApp* app)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    app->window = SDL_CreateWindow(
        "ELE784 Camera Stream - Hardware YUV Acceleration",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!app->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!app->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // *** CHANGED: Use YUY2 (YUYV) format for hardware acceleration ***
    app->texture = SDL_CreateTexture(
        app->renderer,
        SDL_PIXELFORMAT_YUY2,  // Hardware-accelerated YUV format
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (!app->texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        fprintf(stderr, "Your GPU may not support YUY2 format\n");
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // Try to load a system font (optional - will work without it)
    app->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!app->font) {
        // Try alternative font path
        app->font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 14);
        if (!app->font) {
            fprintf(stderr, "Warning: Could not load font, UI text disabled\n");
        }
    }

    printf("SDL initialized successfully with hardware YUV acceleration\n");
    return true;
}

static void cleanup(CameraApp* app)
{
    printf("Cleaning up...\n");

    if (app->fd >= 0) {
        ioctl(app->fd, IOCTL_STREAMOFF);
        close(app->fd);
        printf("Camera stream stopped\n");
    }

    if (app->font) {
        TTF_CloseFont(app->font);
    }

    if (app->texture) {
        SDL_DestroyTexture(app->texture);
    }

    if (app->renderer) {
        SDL_DestroyRenderer(app->renderer);
    }

    if (app->window) {
        SDL_DestroyWindow(app->window);
    }

    TTF_Quit();
    SDL_Quit();
    
    printf("Cleanup complete\n");
}

int main(int argc, char* argv[])
{
    CameraApp app = {
        .fd = -1,
        .window = NULL,
        .renderer = NULL,
        .texture = NULL,
        .font = NULL,
        .running = true,
        .paused = false,
        .show_info = true,
        .frame_count = 0,
        .fps = 0,
        .last_fps_time = 0,
        .last_status_time = 0
    };

    printf("ELE784 Camera Stream Interface\n");
    printf("===============================\n");
    printf("Using hardware-accelerated YUV rendering\n\n");

    if (!init_camera(&app)) {
        fprintf(stderr, "Failed to initialize camera\n");
        return 1;
    }

    if (!init_sdl(&app)) {
        fprintf(stderr, "Failed to initialize SDL\n");
        cleanup(&app);
        return 1;
    }

    // *** CHANGED: Only allocate YUYV buffer (no RGB needed) ***
    uint8_t* yuyv = malloc(FRAME_SIZE);

    if (!yuyv) {
        fprintf(stderr, "Failed to allocate frame buffer\n");
        free(yuyv);
        cleanup(&app);
        return 1;
    }

    app.last_fps_time = SDL_GetTicks();
    app.last_status_time = SDL_GetTicks();

    printf("Starting main loop...\n");
    printf("Controls:\n");
    printf("  SPACE - Pause/Resume\n");
    printf("  I     - Toggle info overlay\n");
    printf("  F     - Toggle fullscreen\n");
    printf("  Q/ESC - Quit\n\n");

    // Main loop
    while (app.running) {
        handle_events(&app);

        if (!app.paused) {
            int ret = read(app.fd, yuyv, FRAME_SIZE);
            
            if (ret > 0) {
                // *** CHANGED: Direct YUV upload - no conversion needed! ***
                SDL_UpdateTexture(app.texture, NULL, yuyv, WIDTH * 2);
                update_fps(&app);
            } else if (ret < 0) {
                perror("read");
                usleep(10000); // Brief pause on error
            }
        } else {
            SDL_Delay(16); // ~60 FPS when paused
        }

        // Render
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, app.texture, NULL, NULL);
        draw_ui(&app);
        SDL_RenderPresent(app.renderer);
    }

    // Cleanup
    free(yuyv);
    cleanup(&app);

    printf("Application exited successfully\n");
    return 0;
}