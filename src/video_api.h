#ifndef VIDEO_API_H
#define VIDEO_API_H

#include "decoder.h"
#include "encoder.h"
#include "display.h"
#include "photo.h"
#include <iostream>
#include <memory>

struct VideoSettings{
    std::string input_file, output_file, format, photo_name, title;
    int pos_x, pos_y;
    int width, height;
    int bit_rate;
//    int encoder_type;
};

class VideoAPI
{
    std::shared_ptr<Decoder> decoder;
    std::shared_ptr<Encoder> encoder;
    std::shared_ptr<Display> display;

    // settings
    std::string input_file, output_file, format;
    int pos_x, pos_y;
    int width, height;
    int bit_rate;

    VideoSettings settings;

public:
    VideoAPI(VideoSettings set);
    ~VideoAPI(){}
    int startVideoRecord();
    int startVideoDisplay();
    void stopVideoRecord();
    void stopVideoDisplay();
    void makePhoto();
};







#endif  /* VIDEO_API_H */
