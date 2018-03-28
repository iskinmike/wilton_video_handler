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

class Decoder
{
    std::string      filename;
    std::string      format;
    AVFormatContext *pFormatCtx;
    AVInputFormat   *file_iformat;
    AVCodecContext*  pCodecCtx;
    AVCodec*         pCodec;
    AVDictionary    *optionsDict;
    AVFrame         *pFrame;
    AVFrame         *pFrameOut;

    struct SwsContext      *sws_ctx;
    int             numBytes;
    uint8_t         *buffer;

    AVPacket        packet;
    int             frameFinished;
    int videoStream;

    std::thread decoder_thread;

    int bit_rate;
    int width;
    int height;
    void runDecoding();

    std::atomic_bool stop_flag;
public:
  Decoder(std::string in, std::string _format, int _widtth, int _height, int _bit_rate)
      : filename(in), format(_format),
        pFormatCtx(NULL),file_iformat(NULL),pCodecCtx(NULL),
        pCodec(NULL), optionsDict(NULL), pFrame(NULL),
        pFrameOut(NULL), sws_ctx(NULL), buffer(NULL), stop_flag(false),
        width(_widtth), height(_height), bit_rate(_bit_rate){}
  ~Decoder();

  std::string init();

  void startDecoding();
  void stopDecoding();

  int getBitRate();
  int getWidth();
  int getHeight();

};

#endif  /* DECODER_HPP */
