
#include "display.hpp"
#include "frame_keeper.hpp"

void display::run_display()
{
    frame_keeper& fk = frame_keeper::instance();
    while (!stop_flag) {
        AVFrame *tmp_frame = fk.get_current_frame();
        display_frame(tmp_frame);
        av_frame_free(&tmp_frame);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if( event.type == SDL_QUIT ) break;
        }
    }
    stop_flag.exchange(false);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
}

std::string display::wait_result(){
    sync_point.flag.exchange(false);
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!sync_point.flag) {
        std::cv_status status = sync_point.cond.wait_for(lck, std::chrono::seconds(4));
        if (std::cv_status::timeout == status) {
            break;
        }
    }
    return init_result;
}

void display::send_result(std::string result){
    init_result = result;
    sync_point.flag.exchange(true);
    sync_point.cond.notify_all();
}

display::~display(){
    stop_display();
}

std::string display::init(int pos_x, int pos_y, int width, int height)
{
    this->width = width;
    this->height = height;

    const int default_screen_pos = 100;
    int screen_pos_x = (-1 != pos_x) ? pos_x : default_screen_pos ;
    int screen_pos_y = (-1 != pos_y) ? pos_y : default_screen_pos ;

    if(SDL_Init(SDL_INIT_VIDEO)) {
      return std::string("Could not initialize SDL - ") + std::string(SDL_GetError());
    }

    screen = SDL_CreateWindow(title.c_str(), screen_pos_x, screen_pos_y, width, height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    SDL_SetWindowFullscreen(screen, SDL_TRUE);
    SDL_RaiseWindow(screen); // rise above other windows
    SDL_SetWindowFullscreen(screen, SDL_FALSE);

    if(!screen) {
        SDL_Quit();
        return std::string("SDL: could not set video mode - exiting");
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        SDL_DestroyWindow(screen);
        SDL_Quit();
        return std::string("SDL: could not create renderer - exiting");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        SDL_DestroyWindow(screen);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return std::string("SDL: could not create texture - exiting");
    }

    initialized = true;
    return std::string{};
}

std::string display::start_display(int pos_x, int pos_y, int width, int height)
{
    display_thread = std::thread([this, pos_x, pos_y, width, height] {
        send_result(init(pos_x, pos_y, width, height));
        this->run_display();
    });
    return wait_result();
}

void display::stop_display()
{
    stop_flag.exchange(true);
    if (display_thread.joinable()) display_thread.join();
}

bool display::is_initialized() const {
    return initialized;
}

void display::display_frame(AVFrame *frame)
{
    if (nullptr == frame) {
        return;
    }

    struct SwsContext* sws_ctx;
    AVPixelFormat format = static_cast<AVPixelFormat>(frame->format);
    AVPixelFormat new_format = AV_PIX_FMT_YUV420P;

    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        format,
        this->width,
        this->height,
        new_format,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    AVFrame *frame_rgb = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(new_format, width,
                    height);
    uint8_t* buffer=new uint8_t[numBytes*sizeof(uint8_t)];

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frame_rgb, buffer, new_format,
           width, height);

    // setup frame sizes
    frame_rgb->width = width;
    frame_rgb->height = height;
    frame_rgb->format = new_format;
    frame_rgb->pts = 0;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
        ((AVPicture*)frame)->data,
        ((AVPicture*)frame)->linesize,
        0,
        frame->height,
        ((AVPicture *)frame_rgb)->data,
        ((AVPicture *)frame_rgb)->linesize);

    SDL_UpdateYUVTexture(
        texture,
        NULL,
        frame_rgb->data[0], //vp->yPlane,
        frame_rgb->linesize[0],
        frame_rgb->data[1], //vp->yPlane,
        frame_rgb->linesize[1],
        frame_rgb->data[2], //vp->yPlane,
        frame_rgb->linesize[2]
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);


    delete[] buffer;
    sws_freeContext(sws_ctx);
    av_frame_free(&frame_rgb);
}
