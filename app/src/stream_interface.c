#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
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
// Pan / Tilt ranges (from spec / lab)
// =======================================
#define PAN_MIN   (-4480)
#define PAN_MAX   ( 4480)
#define TILT_MIN  (-1920)
#define TILT_MAX  ( 1920)

// =======================================
// Control panel width (for GUI)
// =======================================
#define CONTROL_PANEL_WIDTH 200

// =======================================
// Window scaling (logical canvas size)
// =======================================
#define SCALE 1
#define WINDOW_WIDTH  (WIDTH + CONTROL_PANEL_WIDTH) // 840
#define WINDOW_HEIGHT (HEIGHT)                      // 480

// =======================================
// UI constants
// =======================================
#define UI_PANEL_HEIGHT         40
#define STATUS_UPDATE_INTERVAL  500
#define INPUT_BUF_SIZE          16

typedef struct {
    int fd_stream;
    int fd_control;

    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    TTF_Font*     font;

    bool running;
    bool paused;
    bool show_info;

    uint32_t frame_count;
    uint32_t fps;
    uint32_t last_fps_time;
    uint32_t last_status_time;

    int pan;
    int tilt;

    char input_pan[INPUT_BUF_SIZE];
    char input_tilt[INPUT_BUF_SIZE];
    bool pan_input_active;
    bool tilt_input_active;

} CameraApp;

// =============================================================
// Helper: Draw text
// =============================================================
static void draw_text(CameraApp* app, const char* text, int x, int y, SDL_Color color)
{
    if (!app->font || !text) return;

    SDL_Surface* s = TTF_RenderText_Blended(app->font, text, color);
    if (!s) return;

    SDL_Texture* t = SDL_CreateTextureFromSurface(app->renderer, s);
    if (t) {
        SDL_Rect r = { x, y, s->w, s->h };
        SDL_RenderCopy(app->renderer, t, NULL, &r);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

// =============================================================
// Info bar
// =============================================================
static void draw_ui(CameraApp* app)
{
    if (!app->show_info) return;

    SDL_Color white={255,255,255,255};
    SDL_Color green={0,255,0,255};
    SDL_Color red={255,0,0,255};
    SDL_Color yellow={255,255,0,255};
    SDL_Color bg={0,0,0,180};

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect bar = {0,0,WINDOW_WIDTH,UI_PANEL_HEIGHT};
    SDL_SetRenderDrawColor(app->renderer,bg.r,bg.g,bg.b,bg.a);
    SDL_RenderFillRect(app->renderer,&bar);

    char text[256];
    snprintf(text,sizeof(text),
             "FPS:%u | Frames:%u | %s | HW-YUV | SPACE=Pause  I=Info  Q=Quit  F=Fullscreen",
             app->fps, app->frame_count, app->paused?"PAUSED":"RUNNING");

    draw_text(app, text, 10, 10, app->paused ? red : green);

    // bottom-left resolution
    SDL_Rect rbg={0,WINDOW_HEIGHT-30,150,30};
    SDL_RenderFillRect(app->renderer,&rbg);
    draw_text(app,"640x480 YUY2",10,WINDOW_HEIGHT-25,yellow);

    char ctext[64];
    snprintf(ctext,sizeof(ctext),"Center: pan=%d tilt=%d",app->pan,app->tilt);
    draw_text(app, ctext, WINDOW_WIDTH-260, 10, white);
}

// =============================================================
// Layout helper rects
// =============================================================
static SDL_Rect get_panel() {
    SDL_Rect r={WIDTH,0,CONTROL_PANEL_WIDTH,WINDOW_HEIGHT};
    return r;
}
static SDL_Rect get_box() {
    SDL_Rect r={WIDTH+20,80,CONTROL_PANEL_WIDTH-40,CONTROL_PANEL_WIDTH-40};
    return r;
}
static SDL_Rect get_pan_input() {
    SDL_Rect b=get_box();
    SDL_Rect r={ WIDTH+20, b.y+b.h+28, CONTROL_PANEL_WIDTH-40,30 };
    return r;
}
static SDL_Rect get_tilt_input() {
    SDL_Rect p=get_pan_input();
    SDL_Rect r={ WIDTH+20, p.y+p.h+28, CONTROL_PANEL_WIDTH-40,30 };
    return r;
}
static SDL_Rect get_move_btn() {
    SDL_Rect t=get_tilt_input();
    SDL_Rect r={ WIDTH+40, t.y+t.h+20, CONTROL_PANEL_WIDTH-80,35 };
    return r;
}

// =============================================================
// Draw control panel (with center display + key instructions)
// =============================================================
static void draw_control_panel(CameraApp* app)
{
    SDL_Color text={255,255,255,255};
    SDL_Color inactive={180,180,180,255};
    SDL_Color active={255,215,0,255};
    SDL_Color panel_bg={40,40,40,255};

    SDL_Rect p = get_panel();
    SDL_SetRenderDrawColor(app->renderer,panel_bg.r,panel_bg.g,panel_bg.b,panel_bg.a);
    SDL_RenderFillRect(app->renderer,&p);

    //---------------------------------------------------------
    // Title
    //---------------------------------------------------------
    draw_text(app,"CONTROL PANEL",WIDTH+20,20,text);

    //---------------------------------------------------------
    // Show current center (pan, tilt)
    //---------------------------------------------------------
    char c1[64], c2[64];
    snprintf(c1,sizeof(c1),"Current pan  = %d", app->pan);
    snprintf(c2,sizeof(c2),"Current tilt = %d", app->tilt);

    draw_text(app, c1, WIDTH+20, 50, text);
    draw_text(app, c2, WIDTH+20, 68, text);

    //---------------------------------------------------------
    // Box representing movement range
    //---------------------------------------------------------
    SDL_Rect box=get_box();
    SDL_SetRenderDrawColor(app->renderer,60,60,60,255);
    SDL_RenderFillRect(app->renderer,&box);

    // crosshair
    SDL_SetRenderDrawColor(app->renderer,100,100,100,255);
    int cx=box.x+box.w/2;
    int cy=box.y+box.h/2;
    SDL_RenderDrawLine(app->renderer,cx,box.y,cx,box.y+box.h);
    SDL_RenderDrawLine(app->renderer,box.x,cy,box.x+box.w,cy);

    // dot showing current center
    float pn=(float)(app->pan-PAN_MIN)/(PAN_MAX-PAN_MIN);
    float tn=(float)(app->tilt-TILT_MIN)/(TILT_MAX-TILT_MIN);
    int dx=box.x+(int)(pn*box.w);
    int dy=box.y+(int)(tn*box.h);

    SDL_SetRenderDrawColor(app->renderer,0,255,0,255);
    SDL_Rect dot={dx-3,dy-3,6,6};
    SDL_RenderFillRect(app->renderer,&dot);

    //---------------------------------------------------------
    // Input boxes + labels
    //---------------------------------------------------------
    SDL_Rect panr=get_pan_input();
    SDL_Rect tiltr=get_tilt_input();
    SDL_Rect move=get_move_btn();

    draw_text(app,"Pan (X):",  WIDTH+20, panr.y-18, text);
    draw_text(app,"Tilt (Y):", WIDTH+20, tiltr.y-18, text);

    // pan box
    SDL_SetRenderDrawColor(app->renderer,
        app->pan_input_active?active.r:inactive.r,
        app->pan_input_active?active.g:inactive.g,
        app->pan_input_active?active.b:inactive.b,255);
    SDL_RenderDrawRect(app->renderer,&panr);
    draw_text(app,app->input_pan, panr.x+5, panr.y+5, text);

    // tilt box
    SDL_SetRenderDrawColor(app->renderer,
        app->tilt_input_active?active.r:inactive.r,
        app->tilt_input_active?active.g:inactive.g,
        app->tilt_input_active?active.b:inactive.b,255);
    SDL_RenderDrawRect(app->renderer,&tiltr);
    draw_text(app,app->input_tilt, tiltr.x+5, tiltr.y+5, text);

    //---------------------------------------------------------
    // MOVE button
    //---------------------------------------------------------
    SDL_SetRenderDrawColor(app->renderer,80,80,160,255);
    SDL_RenderFillRect(app->renderer,&move);

    SDL_SetRenderDrawColor(app->renderer,200,200,255,255);
    SDL_RenderDrawRect(app->renderer,&move);

    draw_text(app,"MOVE", move.x+20, move.y+8, text);

    //---------------------------------------------------------
    // Key instructions
    //---------------------------------------------------------
    draw_text(app,"Keys:", WIDTH+20, move.y+55, text);
    draw_text(app," Q / ESC = Quit", WIDTH+20, move.y+75, text);
}


// =============================================================
// FPS
// =============================================================
static void update_fps(CameraApp* app)
{
    static uint32_t count=0;
    uint32_t now=SDL_GetTicks();

    count++;
    if(now-app->last_fps_time>=1000){
        app->fps=count;
        app->last_fps_time=now;
        count=0;
    }
    app->frame_count = app->fps;
}

// =============================================================
// Movement logic
// =============================================================
static int clamp_step(int d,int lim){
    if(d> lim) return lim;
    if(d<-lim) return -lim;
    return d;
}

static void move_to_new_center(CameraApp* app,int tp,int tt)
{
    if(tp>PAN_MAX) tp=PAN_MAX;
    if(tp<PAN_MIN) tp=PAN_MIN;
    if(tt>TILT_MAX)tt=TILT_MAX;
    if(tt<TILT_MIN)tt=TILT_MIN;

    int dpan=tp-app->pan;
    int dtilt=tt-app->tilt;

    printf("Move request pan=%d tilt=%d delta=(%d,%d)\n",tp,tt,dpan,dtilt);

    while(dpan!=0 || dtilt!=0){
        int sp=clamp_step(dpan,4480);
        int st=clamp_step(dtilt,1920);

        struct pantilt_relative r={.pan=sp,.tilt=st};
        if(ioctl(app->fd_control,IOCTL_PANTILT_RELATIVE,&r)<0){
            perror("pantilt step failed");
            break;
        }
        app->pan+=sp;
        app->tilt+=st;
        dpan-=sp;
        dtilt-=st;
        sleep(1);
    }
}

// =============================================================
// Event handling
// =============================================================
static void handle_events(CameraApp* app)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        switch(e.type){
            case SDL_QUIT: app->running=false; break;

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym){
                case SDLK_q:
                case SDLK_ESCAPE:
                    app->running=false; break;
                case SDLK_SPACE:
                    app->paused=!app->paused; break;
                case SDLK_i:
                    app->show_info=!app->show_info; break;
                case SDLK_f:{
                    uint32_t f=SDL_GetWindowFlags(app->window);
                    SDL_SetWindowFullscreen(app->window,
                        (f & SDL_WINDOW_FULLSCREEN)?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
                } break;
                case SDLK_BACKSPACE:
                    if(app->pan_input_active){
                        size_t n=strlen(app->input_pan);
                        if(n>0)app->input_pan[n-1]='\0';
                    } else if(app->tilt_input_active){
                        size_t n=strlen(app->input_tilt);
                        if(n>0)app->input_tilt[n-1]='\0';
                    }
                break;
                }
            break;

            case SDL_TEXTINPUT:
                if(app->pan_input_active){
                    size_t n=strlen(app->input_pan);
                    if(n<INPUT_BUF_SIZE-1){
                        char c=e.text.text[0];
                        if((c>='0'&&c<='9')||(c=='-'&&n==0)){
                            app->input_pan[n]=c;
                            app->input_pan[n+1]='\0';
                        }
                    }
                } else if(app->tilt_input_active){
                    size_t n=strlen(app->input_tilt);
                    if(n<INPUT_BUF_SIZE-1){
                        char c=e.text.text[0];
                        if((c>='0'&&c<='9')||(c=='-'&&n==0)){
                            app->input_tilt[n]=c;
                            app->input_tilt[n+1]='\0';
                        }
                    }
                }
            break;

            case SDL_MOUSEBUTTONDOWN:{
                SDL_Point p={e.button.x,e.button.y};
                SDL_Rect panr=get_pan_input();
                SDL_Rect tiltr=get_tilt_input();
                SDL_Rect move=get_move_btn();

                if(SDL_PointInRect(&p,&panr)){
                    app->pan_input_active=true;
                    app->tilt_input_active=false;
                    SDL_StartTextInput();
                }
                else if(SDL_PointInRect(&p,&tiltr)){
                    app->pan_input_active=false;
                    app->tilt_input_active=true;
                    SDL_StartTextInput();
                }
                else if(SDL_PointInRect(&p,&move)){
                    SDL_StopTextInput();
                    app->pan_input_active=false;
                    app->tilt_input_active=false;
                    int np=atoi(app->input_pan);
                    int nt=atoi(app->input_tilt);
                    move_to_new_center(app,np,nt);
                }
            } break;
        }
    }
}

// =============================================================
// Init camera devices
// =============================================================
static bool init_devices(CameraApp* app)
{
    app->fd_control=open("/dev/camera_control",O_RDWR);
    if(app->fd_control<0){ perror("open control"); return false; }

    printf("RESET... "); fflush(stdout);
    if(ioctl(app->fd_control,IOCTL_PANTILT_RESET,0)<0)
        perror("reset failed");
    else
        printf("OK\n");

    sleep(5);
    app->pan=0; app->tilt=0;

    app->fd_stream=open("/dev/camera_stream",O_RDWR);
    if(app->fd_stream<0){ perror("open stream"); return false; }

    if(ioctl(app->fd_stream,IOCTL_STREAMON,NULL)<0){
        perror("STREAMON failed");
        return false;
    }

    return true;
}

// =============================================================
// SDL init
// =============================================================
static bool init_sdl(CameraApp* app)
{
    if(SDL_Init(SDL_INIT_VIDEO)<0){ fprintf(stderr,"SDL fail\n"); return false; }
    if(TTF_Init()<0){ fprintf(stderr,"TTF fail\n"); SDL_Quit(); return false; }

    app->window = SDL_CreateWindow("ELE784 Camera Stream",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH,WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

    if(!app->window){ fprintf(stderr,"win fail\n"); return false; }

    app->renderer = SDL_CreateRenderer(app->window,-1,SDL_RENDERER_ACCELERATED);
    if(!app->renderer){ fprintf(stderr,"rnd fail\n"); return false; }

    SDL_RenderSetLogicalSize(app->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_YUY2,
                                     SDL_TEXTUREACCESS_STREAMING, WIDTH,HEIGHT);
    if(!app->texture){ fprintf(stderr,"tex fail\n"); return false; }

    app->font=TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",14);

    return true;
}

// =============================================================
// Cleanup
// =============================================================
static void cleanup(CameraApp* app)
{
    if(app->fd_stream>=0) ioctl(app->fd_stream,IOCTL_STREAMOFF,NULL);
    if(app->fd_stream>=0) close(app->fd_stream);
    if(app->fd_control>=0) close(app->fd_control);

    if(app->font)    TTF_CloseFont(app->font);
    if(app->texture) SDL_DestroyTexture(app->texture);
    if(app->renderer)SDL_DestroyRenderer(app->renderer);
    if(app->window)  SDL_DestroyWindow(app->window);

    TTF_Quit();
    SDL_Quit();
}

// =============================================================
// MAIN
// =============================================================
int main()
{
    CameraApp app={0};
    app.running=true;
    strcpy(app.input_pan,"0");
    strcpy(app.input_tilt,"0");

    if(!init_devices(&app)) return 1;
    if(!init_sdl(&app)){ cleanup(&app); return 1; }

    uint8_t* buf=malloc(FRAME_SIZE);
    app.last_fps_time=SDL_GetTicks();

    SDL_StartTextInput();

    while(app.running){
        handle_events(&app);

        if(!app.paused){
            int r=read(app.fd_stream,buf,FRAME_SIZE);
            if(r>0){
                SDL_UpdateTexture(app.texture,NULL,buf,WIDTH*2);
                update_fps(&app);
            }
        }

        SDL_SetRenderDrawColor(app.renderer,0,0,0,255);
        SDL_RenderClear(app.renderer);

        SDL_Rect vr={0,0,WIDTH,HEIGHT};
        SDL_RenderCopy(app.renderer, app.texture, NULL, &vr);

        draw_control_panel(&app);
        draw_ui(&app);

        SDL_RenderPresent(app.renderer);
    }

    free(buf);
    cleanup(&app);
    return 0;
}
