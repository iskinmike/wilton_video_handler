#ifndef VIDEO_API_HPP
#define VIDEO_API_HPP

#include "decoder.hpp"
#include "encoder.hpp"
#include "display.hpp"
#include "photo.hpp"
#include <iostream>
#include <memory>

struct VideoSettings{
    std::string input_file, output_file, format, photo_name, title;
    int pos_x, pos_y;
    int width, height;
    int bit_rate;
};

class VideoAPI
{
    std::shared_ptr<Decoder> decoder;
    std::shared_ptr<Encoder> encoder;
    std::shared_ptr<Display> display;

    // settings
    VideoSettings settings;

public:
    VideoAPI(VideoSettings set);
    ~VideoAPI(){}
    std::string startVideoRecord();
    std::string startVideoDisplay();
    void stopVideoRecord();
    void stopVideoDisplay();
    std::string makePhoto();
};







#endif  /* VIDEO_API_HPP */
