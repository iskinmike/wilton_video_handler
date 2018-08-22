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

#ifndef ENCODER_HPP
#define ENCODER_HPP

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <stdio.h>
#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <mutex>
#include "frame_keeper.hpp"

struct encoder_settings {
    std::string output_file;
    int width, height;
    double framerate;
    int bit_rate;
};

class encoder
{
    AVCodec*         encode_codec;
    AVFormatContext* out_format_ctx;
    AVStream*        out_stream;
    std::string      out_file;

    struct SwsContext* sws_ctx;
    int                num_bytes;
    uint8_t*           buffer;
    AVFrame*         frame_out;

    int bit_rate;
    double framerate;
    int width;
    int height;
    bool pts_flag;
    int64_t last_pts;
    AVRational input_time_base;

    uint64_t pts_offset;
    uint64_t last_time;
    bool first_run;

    FILE *file;
    std::thread encoder_thread;
    void run_encoding();
    std::atomic_bool stop_flag;
    std::atomic_bool encoding_started;
    bool initialized;

    std::mutex mtx;
    std::shared_ptr<frame_keeper> keeper;


    AVFrame* get_frame_from_keeper();
    AVRational get_time_base_from_keeper();
    void rescale_frame(AVFrame* frame);

public:
  explicit encoder(encoder_settings set);
  ~encoder();

  std::string init();
  std::string start_encoding();
  void stop_encoding();
  void pause_encoding();

  int encode_frame(AVFrame* frame);

  void close_file();
  void fflush_encoder();
  bool is_initialized() const;

  void setup_frame_keeper(std::shared_ptr<frame_keeper> keeper);
};



#endif  /* ENCODER_HPP */
