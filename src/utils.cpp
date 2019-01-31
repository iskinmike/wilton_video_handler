#include "utils.hpp"

std::string utils::construct_error(std::string what){
    std::string error("{ \"error\": \"");
    error += what;
    error += "\"}";
    return error;
}

AVFrame *utils::rescale_frame(AVFrame *frame, int new_width, int new_height, AVPixelFormat format, std::vector<uint8_t> &buffer){
    if (nullptr == frame) {
        return nullptr;
    }
    SwsContext* sws_ctx = sws_getContext(
                frame->width,
                frame->height,
                static_cast<AVPixelFormat>(frame->format),
                new_width,
                new_height,
                format,
                SWS_FAST_BILINEAR,
                NULL,
                NULL,
                NULL
                );

    AVFrame* frame_rgb = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(format, new_width, new_height);
    buffer.resize(numBytes*sizeof(uint8_t));

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frame_rgb, buffer.data(), format,
                   new_width, new_height);

    // setup frame sizes
    frame_rgb->width = new_width;
    frame_rgb->height = new_height;
    frame_rgb->format = format;
    frame_rgb->pts = 0;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
              ((AVPicture*)frame)->data,
              ((AVPicture*)frame)->linesize,
              0,
              frame->height,
              ((AVPicture *)frame_rgb)->data,
              ((AVPicture *)frame_rgb)->linesize);

    sws_freeContext(sws_ctx);
    return frame_rgb;
}