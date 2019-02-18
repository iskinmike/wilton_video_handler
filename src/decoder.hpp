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
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h> // installed libavdevice-dev
}

#include <string>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
#include "frame_keeper.hpp"


struct decoder_settings {
    std::string input_file, format;
    int time_base_den, time_base_num;
};

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
    std::shared_ptr<frame_keeper> keeper;
    AVRational input_time_base;

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
    explicit decoder(decoder_settings set);
    ~decoder();

    std::string init();

    void start_decoding();
    void stop_decoding();

    int get_bit_rate();
    int get_width();
    int get_height();

    std::shared_ptr<frame_keeper> get_keeper();

    bool is_initialized() const;
};

#endif  /* DECODER_HPP */
