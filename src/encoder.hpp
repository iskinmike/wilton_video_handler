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

class Encoder
{
    AVCodec*         pEncodeCodec;
    // write structs
    AVFormatContext* pOutFormatCtx;
    AVStream*        pOutStream;

    std::string      out_file;

    int bit_rate;
    int width;
    int height;
    bool pts_flag;

    FILE *pFile;
    std::thread encoder_thread;
    void runEncoding();
    std::atomic_bool stop_flag;

public:
  Encoder(std::string out)
      : pEncodeCodec(NULL), out_file(out),
        stop_flag(false), pOutFormatCtx(NULL), pOutStream(NULL), pts_flag(false)  {}
  ~Encoder();

  std::string init(int _bit_rate, int _width, int _height);
  void startEncoding();
  void stopEncoding();

  int encodeFrame(AVFrame* frame);
  void closeFile();
  void fflushEncoder();
};



#endif  /* ENCODER_HPP */
