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

class encoder
{
    AVCodec*         encode_codec;
    AVFormatContext* out_format_ctx;
    AVStream*        out_stream;

    std::string      out_file;

    int bit_rate;
    double framerate;
    int width;
    int height;
    bool pts_flag;
    int64_t last_pts;

    FILE *file;
    std::thread encoder_thread;
    void run_encoding();
    std::atomic_bool stop_flag;
    bool initialized;

public:
  encoder(std::string out);
  ~encoder();

  std::string init(int bit_rate, int width, int height, double framerate);
  void start_encoding();
  void stop_encoding();

  int encode_frame(AVFrame* frame);

  void close_file();
  void fflush_encoder();
  bool is_initialized() const;
};



#endif  /* ENCODER_HPP */
