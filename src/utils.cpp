#include "utils.hpp"

std::string utils::construct_error(std::string what){
    std::string error("{ \"error\": \"");
    error += what;
    error += "\"}";
    return error;
}

AVFrame* utils::rescale_frame(AVFrame *frame, int new_width, int new_height, AVPixelFormat format, std::vector<uint8_t> &buffer){
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

utils::frame_rescaler::frame_rescaler(int width, int height, AVPixelFormat format)
    : sws_ctx(nullptr), width(width), height(height), format(format) {
    int num_bytes = avpicture_get_size(format, width, height);
    buffer.resize(num_bytes*sizeof(uint8_t));
}

void utils::frame_rescaler::clear_sws_context(){
    if (nullptr != sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
}

void utils::frame_rescaler::rescale_frame_to_existed(AVFrame *frame, AVFrame *out_frame) {
    if (nullptr == frame) return;
    if (nullptr == sws_ctx) {
        sws_ctx = sws_getContext(
                    frame->width,
                    frame->height,
                    static_cast<AVPixelFormat>(frame->format),
                    width,
                    height,
                    format,
                    SWS_FAST_BILINEAR,
                    nullptr,
                    nullptr,
                    nullptr);
    }

    //setup buffer for new frame
    avpicture_fill((AVPicture *)out_frame, buffer.data(), format,
                   width, height);

    // setup frame sizes
    out_frame->width = width;
    out_frame->height = height;
    out_frame->format = format;
    out_frame->pts = 0;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
              ((AVPicture*)frame)->data,
              ((AVPicture*)frame)->linesize,
              0,
              frame->height,
              ((AVPicture*)out_frame)->data,
              ((AVPicture*)out_frame)->linesize);
}

AVFrame *utils::frame_rescaler::rescale_frame(AVFrame *frame) {
    if (nullptr == frame) return nullptr;
    AVFrame* frame_out = av_frame_alloc();
    rescale_frame_to_existed(frame, frame_out);
    return frame_out;
}

utils::frame_rescaler::~frame_rescaler() {
    clear_sws_context();
}
