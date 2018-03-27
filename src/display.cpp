
#include "display.h"
#include "frame_keeper.h"
#include <iostream>

struct Frame{
    uint8_t data[3][400*400*3];
    uint64_t linesize[3];
} _frame;

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


    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_TARGET, 400, 400);
    _frame.linesize[0] = 400;
    _frame.linesize[1] = 200;
    _frame.linesize[2] = 200;

    /* Y */
    int x,y;
    int i = 1;
    for (y = 0; y < 400; y++)
        for (x = 0; x < 400; x++)
            _frame.data[0][y * _frame.linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < 400 / 2; y++) {
        for (x = 0; x < 400 / 2; x++) {
            _frame.data[1][y * _frame.linesize[1] + x] = 128 + y + i * 2;
            _frame.data[2][y * _frame.linesize[2] + x] = 64 + x + i * 5;
        }
    }

    std::cout << "init frame->linesize[0]: " << _frame.linesize[0] << std::endl;
    std::cout << "init frame->linesize[1]: " << _frame.linesize[1] << std::endl;
    std::cout << "init frame->linesize[2]: " << _frame.linesize[2] << std::endl;

    SDL_UpdateYUVTexture(
        texture,
        NULL,
        _frame.data[0], //vp->yPlane,
        _frame.linesize[0],
        _frame.data[1], //vp->yPlane,
        _frame.linesize[1],
        _frame.data[2], //vp->yPlane,
        _frame.linesize[2]
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);

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
                            SDL_TEXTUREACCESS_TARGET, 400, 400);
    _frame.linesize[0] = 400;
    _frame.linesize[1] = 200;
    _frame.linesize[2] = 200;

    /* Y */
    int x,y;
    int i = 1;
    for (y = 0; y < 400; y++)
        for (x = 0; x < 400; x++)
            _frame.data[0][y * _frame.linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < 400 / 2; y++) {
        for (x = 0; x < 400 / 2; x++) {
            _frame.data[1][y * _frame.linesize[1] + x] = 128 + y + i * 2;
            _frame.data[2][y * _frame.linesize[2] + x] = 64 + x + i * 5;
        }
    }

    std::cout << "frame->linesize[0]: " << _frame.linesize[0] << std::endl;
    std::cout << "frame->linesize[1]: " << _frame.linesize[1] << std::endl;
    std::cout << "frame->linesize[2]: " << _frame.linesize[2] << std::endl;

    SDL_UpdateYUVTexture(
        texture,
        NULL,
        _frame.data[0], //vp->yPlane,
        _frame.linesize[0],
        _frame.data[1], //vp->yPlane,
        _frame.linesize[1],
        _frame.data[2], //vp->yPlane,
        _frame.linesize[2]
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
    av_frame_free(&frame);
}
