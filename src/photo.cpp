#include "photo.hpp"
#include "frame_keeper.hpp"
#include "stdio.h"
namespace photo{
std::string make_photo(std::string out_file)
{
    FILE* file = fopen(out_file.c_str(), "wb");
    if (NULL == file) {
        return std::string("Can't open file for Photo.");
    }
    AVFrame* frame_rgb;
    struct SwsContext* sws_ctx;

    frame_keeper& fk = frame_keeper::instance();
    AVFrame* frame = fk.get_origin_frame();

    if (nullptr == frame) {
        return std::string("Can't make Photo. Get 'NULL'' frame.");
    }

    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        static_cast<AVPixelFormat>(frame->format),
        frame->width,
        frame->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    frame_rgb = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, frame->width,
                    frame->height);
    uint8_t* buffer=new uint8_t[numBytes*sizeof(uint8_t)];

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frame_rgb, buffer, AV_PIX_FMT_RGB24,
           frame->width, frame->height);

    // setup frame sizes
    frame_rgb->width = frame->width;
    frame_rgb->height = frame->height;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
        ((AVPicture*)frame)->data,
        ((AVPicture*)frame)->linesize,
        0,
        frame->height,
        ((AVPicture *)frame_rgb)->data,
        ((AVPicture *)frame_rgb)->linesize);

//     Write header
    fprintf(file, "P6\n%d %d\n255\n", frame_rgb->width, frame_rgb->height);

//     Write pixel data
    for(int y=0; y< frame_rgb->height; y++)
    fwrite(frame_rgb->data[0]+y*frame_rgb->linesize[0], 1, frame_rgb->width*3, file);

    fclose(file);
    av_frame_free(&frame);
    av_frame_free(&frame_rgb);
    return std::string{};
}
}
