#ifndef ENCODER_H
#define ENCODER_H

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



#endif  /* ENCODER_H */
