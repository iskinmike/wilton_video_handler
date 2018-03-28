
#include "display.hpp"
#include "frame_keeper.hpp"

void Display::runDisplay()
{
    auto fk = shared_framekeeper();
    while (!stop_flag) {
        displayFrame(fk->getFrame());
    }
    stop_flag.exchange(false);
}

std::string Display::waitResult(){
    sync_point.flag.exchange(false);
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!sync_point.flag) sync_point.cond.wait_for(lck, std::chrono::seconds(4));
    return init_result;
}

void Display::sendResult(std::string result){
    init_result = result;
    sync_point.flag.exchange(true);
    sync_point.cond.notify_all();
}

Display::~Display(){
    if (display_thread.joinable()) {
        display_thread.join();
    }
}

std::string Display::init(int pos_x, int pos_y, int _width, int _height)
{
    width = _width;
    height = _height;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      return std::string("Could not initialize SDL - ") + std::string(SDL_GetError());
    }

    screen = SDL_CreateWindow(title.c_str(), pos_x, pos_y, width, height, 0);
    if(!screen) {
        return std::string("SDL: could not set video mode - exiting");
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        return std::string("SDL: could not create renderer - exiting");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        return std::string("SDL: could not create texture - exiting");
    }

    return std::string{};
}

std::string Display::startDisplay(int pos_x, int pos_y, int _width, int _height)
{
    display_thread = std::thread([this, pos_x, pos_y, _width, _height] {
        sendResult(init(pos_x, pos_y, _width, _height));
        return this->runDisplay();
    });
    return waitResult();
}

void Display::stopDisplay()
{
    stop_flag.exchange(true);
    if (display_thread.joinable()) display_thread.join();
    SDL_DestroyWindow(screen);
    SDL_DestroyRenderer(renderer);
}

void Display::displayFrame(AVFrame *frame)
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
    av_frame_free(&frame);
}
