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
extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class display
{
    SDL_Renderer* renderer;
    SDL_Window*   screen;
    SDL_Texture*  texture;

    int width;
    int height;
    std::string title;

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
public:
    display(const std::string& title)
        : renderer(NULL), screen(NULL), texture(NULL), title(title), stop_flag(false), init_result("can't init"){}
    ~display();

    std::string init(int pos_x, int pos_y, int width, int height);
    std::string start_display(int pos_x, int pos_y, int width, int height);
    void stop_display();

    void display_frame(AVFrame* frame);
};


#endif  /* DISPLAY_HPP */
