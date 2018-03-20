#ifndef MAKE_PHOTO_H
#define MAKE_PHOTO_H

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <stdio.h>
#include <string>

int makePhoto(std::string out_file);

#endif  /* MAKE_PHOTO_H */
