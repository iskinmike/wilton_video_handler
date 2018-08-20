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

#ifndef FRAME_KEEPER_HPP
#define FRAME_KEEPER_HPP

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <vector>

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

// Try to do it as Meyerce's Singletone [https://ru.stackoverflow.com/questions/327136/singleton-%D0%B8-%D1%80%D0%B5%D0%B0%D0%BB%D0%B8%D0%B7%D0%B0%D1%86%D0%B8%D1%8F]
class frame_keeper
{
    std::mutex cond_mtx;
    struct sync_waiter {
        std::atomic_bool flag;
        std::condition_variable cond;
    };

    AVFrame *frame;
    AVFrame *origin_frame;
    std::mutex mtx;
    AVRational time_base;

    std::vector<sync_waiter*> sync_array;
    void wait_new_frame();
    ~frame_keeper();
    frame_keeper(){}
public:
    static frame_keeper& instance()
    {
        // Not thread safe in vs2013
        static frame_keeper fk;
        return fk;
    }
    frame_keeper(frame_keeper const&) = delete;
    frame_keeper& operator= (frame_keeper const&) = delete;
  
  void assig_new_frames(AVFrame *new_frame, AVFrame* new_origin_frame);
  AVFrame* get_frame();
  AVFrame* get_frame(int& id);
  AVFrame* get_origin_frame();
  AVFrame* get_current_frame();
  void setup_time_base(AVRational& base);
  AVRational get_time_base();
};

#endif  /* FRAME_KEEPER_HPP */
