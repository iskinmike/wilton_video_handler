
#include "display.h"
#include "frame_keeper.h"

void Display::runDisplay()
{
    FrameKeeper& fk = FrameKeeper::Instance();
    while (!stop_flag) {
        displayFrame(fk.getFrame());
    }
    stop_flag.exchange(false);
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
    return std::string{};
}

void Display::startDisplay()
{
    display_thread = std::thread([this] {return this->runDisplay();});
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

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STREAMING, width, height);

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
    SDL_DestroyTexture(texture);
    av_frame_free(&frame);
}
