/*
 * Copyright 2018, myasnikov.mike at gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "SDL2/SDL.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "frame_keeper.hpp"

#ifdef WIN32
#include <windows.h>
#include <cstdint>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#define MAX_PROPERTY_VALUE_LEN 4096
#endif

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

struct display_settings {
    std::string title, parent_title;
    int width, height;
    int pos_x, pos_y;
};

class display
{
    SDL_Renderer* renderer;
    SDL_Window*   screen;
    SDL_Texture*  texture;

#ifdef WIN32
    HWND cam_window;
#else
    Window cam_window;
#endif

    int width;
    int height;
    int pos_x, pos_y;
    std::string title;
    std::string parent_title;
    std::mutex mtx;

    std::mutex cond_mtx;
    struct sync_waiter {
        std::atomic_bool flag;
        std::condition_variable cond;
    } sync_point;
    std::string init_result;

    std::thread display_thread;
    void run_display();
    std::string wait_result();
    void send_result(std::string result);
    std::atomic_bool stop_flag;
    bool initialized;

    std::shared_ptr<frame_keeper> keeper;
    AVFrame *get_frame_from_keeper();
public:
    display(display_settings set);
    ~display();

    std::string init();
    std::string start_display();
    void stop_display();

    bool is_initialized() const;
    void display_frame(AVFrame* frame);

    void set_display_topmost();
    void setup_frame_keeper(std::shared_ptr<frame_keeper> keeper);

};


#endif  /* DISPLAY_HPP */
