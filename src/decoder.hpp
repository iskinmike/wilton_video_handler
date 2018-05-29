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

#ifndef DECODER_HPP
#define DECODER_HPP

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h> // installed libavdevice-dev
}

#include <string>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <atomic>

void start_decode_video(std::string file_name);

class decoder
{
    std::string      filename;
    std::string      format;
    AVFormatContext* format_ctx;
    AVInputFormat*   file_iformat;
    AVCodecContext*  codec_ctx;
    AVCodec*         codec;
    AVFrame*         frame;
    AVFrame*         frame_out;

    struct SwsContext* sws_ctx;
    int                num_bytes;
    uint8_t*           buffer;

    AVPacket        packet;
    int             frame_finished;
    int             video_stream;

    std::thread decoder_thread;

    int bit_rate;
    int width;
    int height;
    void run_decoding();
    bool initialized;

    std::atomic_bool stop_flag;
public:
  decoder(std::string in, std::string format, int widtth, int height, int bit_rate)
      : filename(in), format(format),
        format_ctx(NULL),file_iformat(NULL),codec_ctx(NULL),
        codec(NULL), frame(NULL), frame_out(NULL),
        sws_ctx(NULL), buffer(NULL), stop_flag(false), initialized(false),
        width(widtth), height(height), bit_rate(bit_rate){}
  ~decoder();

  std::string init();

  void start_decoding();
  void stop_decoding();

  int get_bit_rate();
  int get_width();
  int get_height();

  bool is_initialized() const;
};

#endif  /* DECODER_HPP */
