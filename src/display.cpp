
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

    SDL_UpdateYUVTexture(
        texture,
        NULL,
        frame->data[0], //vp->yPlane,
        frame->linesize[0],
        frame->data[1], //vp->yPlane,
        frame->linesize[1],
        frame->data[2], //vp->yPlane,
        frame->linesize[2]
    );
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
