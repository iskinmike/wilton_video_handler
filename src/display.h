#ifndef DISPLAY_H
#define DISPLAY_H

#include "SDL2/SDL.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class Display
{
    SDL_Renderer* renderer;
    SDL_Window*   screen;
    SDL_Texture *texture;

    int width;
    int height;
    std::string title;

    std::mutex cond_mtx;
    struct SyncWaiter {
        std::atomic_bool flag;
        std::condition_variable cond;
    } sync_point;
    std::string init_result;

    std::thread display_thread;
    void runDisplay();
    std::string waitResult();
    void sendResult(std::string result);
    std::atomic_bool stop_flag;
public:
    Display(const std::string& _title)
        : renderer(NULL), screen(NULL), texture(NULL), title(_title), stop_flag(false), init_result("can't init"){}
    ~Display();

    std::string init(int pos_x, int pos_y, int _width, int _height);
    std::string startDisplay(int pos_x, int pos_y, int _width, int _height);
    void stopDisplay();

    void displayFrame(AVFrame* frame);
};


#endif  /* DISPLAY_H */
