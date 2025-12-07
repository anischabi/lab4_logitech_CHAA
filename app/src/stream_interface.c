#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <ioctl_cmds.h>

// =======================================
// Resolution settings
// =======================================
#define WIDTH 640
#define HEIGHT 480
#define FRAME_SIZE (WIDTH * HEIGHT * 2)

// =======================================
// NEW: Control panel width
// =======================================
#define CONTROL_PANEL_WIDTH 200       // Space reserved for controls

// =======================================
// Window scaling
// =======================================
#define SCALE 1
#define WINDOW_WIDTH (WIDTH * SCALE + CONTROL_PANEL_WIDTH)
#define WINDOW_HEIGHT (HEIGHT * SCALE)

// =======================================
// UI constants
// =======================================
#define UI_PANEL_HEIGHT 40
#define STATUS_UPDATE_INTERVAL 500 // ms

typedef struct {
    int fd;              // stream device
    int fd_control;      // NEW: control device
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

// =============================================================
// DRAW TEXT
// =============================================================
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

// =============================================================
// INFO BAR
// =============================================================
static void draw_ui(CameraApp* app)
{
    if (!app->show_info) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color green = { 0, 255, 0, 255 };
    SDL_Color red = { 255, 0, 0, 255 };
    SDL_Color yellow = { 255, 255, 0, 255 };
    SDL_Color bg = { 0, 0, 0, 180 };

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect status_bg = { 0, 0, WINDOW_WIDTH, UI_PANEL_HEIGHT };
    SDL_SetRenderDrawColor(app->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(app->renderer, &status_bg);

    char status_text[256];
    snprintf(status_text, sizeof(status_text), 
             "FPS: %u | Frames: %u | %s | HW-YUV | Press: SPACE=Pause, I=Info, Q/ESC=Quit",
             app->fps, app->frame_count,
             app->paused ? "PAUSED" : "RUNNING");

    draw_text(app, status_text, 10, 10, app->paused ? red : green);

    char res_text[64];
    snprintf(res_text, sizeof(res_text), "%dx%d YUY2", WIDTH, HEIGHT);
    SDL_Rect res_bg = { 0, WINDOW_HEIGHT - 30, 150, 30 };
    SDL_SetRenderDrawColor(app->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(app->renderer, &res_bg);
    draw_text(app, res_text, 10, WINDOW_HEIGHT - 25, yellow);
}

// =============================================================
// NEW: Draw empty control panel (placeholder)
// =============================================================
static void draw_control_panel(CameraApp* app)
{
    SDL_Rect panel = { WIDTH, 0, CONTROL_PANEL_WIDTH, WINDOW_HEIGHT };

    SDL_SetRenderDrawColor(app->renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(app->renderer, &panel);

    SDL_Color white = {255, 255, 255, 255};
    draw_text(app, "CONTROL PANEL", WIDTH + 20, 20, white);
    draw_text(app, "(not implemented)", WIDTH + 20, 50, white);
}

// =============================================================
// FPS update
// =============================================================
static void update_fps(CameraApp* app)
{
    static uint32_t total_frames = 0;
    uint32_t current_time = SDL_GetTicks();
    
    total_frames++;  
    
    if (current_time - app->last_fps_time >= 1000) {
        app->fps = total_frames;
        total_frames = 0;
        app->last_fps_time = current_time;
    }
    
    app->frame_count = total_frames;
}

// =============================================================
// Event handling
// =============================================================
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
                    
                    case SDLK_f: {
                        uint32_t flags = SDL_GetWindowFlags(app->window);
                        SDL_SetWindowFullscreen(app->window, 
                            (flags & SDL_WINDOW_FULLSCREEN)
                                ? 0
                                : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    } break;
                }
                break;
        }
    }
}

// =============================================================
// UPDATED CAMERA INIT (with RESET + sleep(5))
// =============================================================
static bool init_camera(CameraApp* app)
{
    // ------------------------------------------
    // OPEN CONTROL DEVICE
    // ------------------------------------------
    app->fd_control = open("/dev/camera_control", O_RDWR);
    if (app->fd_control < 0) {
        perror("Failed to open /dev/camera_control");
        return false;
    }

    printf("Sending RESET command #1 ... ");
    if (ioctl(app->fd_control, IOCTL_PANTILT_RESET, 0) < 0) {
        perror("IOCTL_PANTILT_RESET failed");
    } else {
        printf("RESET sent successfully! (sleep 5s)\n");
    }
    sleep(5);

    // ------------------------------------------
    // OPEN STREAM DEVICE
    // ------------------------------------------
    app->fd = open("/dev/camera_stream", O_RDWR);
    if (app->fd < 0) {
        perror("Failed to open /dev/camera_stream");
        close(app->fd_control);
        return false;
    }

    printf("\n==============================\n");
    printf("      IOCTL_STREAMON\n");
    printf("==============================\n");

    if (ioctl(app->fd, IOCTL_STREAMON) < 0) {
        perror("IOCTL_STREAMON failed");
        close(app->fd);
        close(app->fd_control);
        return false;
    }

    printf("STREAMON successfully submitted.\n");
    return true;
}

// =============================================================
// SDL init
// =============================================================
static bool init_sdl(CameraApp* app)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (TTF_Init() < 0) return false;

    app->window = SDL_CreateWindow(
        "ELE784 Camera Stream - Hardware YUV Acceleration",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
    app->texture = SDL_CreateTexture(
        app->renderer,
        SDL_PIXELFORMAT_YUY2,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
    );

    app->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);

    return true;
}

// =============================================================
// Cleanup
// =============================================================
static void cleanup(CameraApp* app)
{
    printf("Cleaning up...\n");

    if (app->fd >= 0) {
        ioctl(app->fd, IOCTL_STREAMOFF);
        close(app->fd);
    }

    if (app->fd_control >= 0)
        close(app->fd_control);

    if (app->font) TTF_CloseFont(app->font);
    if (app->texture) SDL_DestroyTexture(app->texture);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);

    TTF_Quit();
    SDL_Quit();
}

// =============================================================
// MAIN
// =============================================================
int main(int argc, char* argv[])
{
    CameraApp app = {0};

    printf("ELE784 Camera Stream Interface\n");
    printf("===============================\n");

    if (!init_camera(&app)) return 1;
    if (!init_sdl(&app)) {
        cleanup(&app);
        return 1;
    }

    uint8_t* yuyv = malloc(FRAME_SIZE);

    app.running = true;
    app.last_fps_time = SDL_GetTicks();
    app.last_status_time = SDL_GetTicks();

    while (app.running) {
        handle_events(&app);

        if (!app.paused) {
            int ret = read(app.fd, yuyv, FRAME_SIZE);
            if (ret > 0) {
                SDL_UpdateTexture(app.texture, NULL, yuyv, WIDTH * 2);
                update_fps(&app);
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);

        SDL_Rect video_rect = {0, 0, WIDTH, HEIGHT};
        SDL_RenderCopy(app.renderer, app.texture, NULL, &video_rect);

        draw_control_panel(&app);
        draw_ui(&app);

        SDL_RenderPresent(app.renderer);
    }

    free(yuyv);
    cleanup(&app);

    return 0;
}
