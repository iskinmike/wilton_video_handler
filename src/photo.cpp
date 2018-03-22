#include "photo.h"
#include "frame_keeper.h"
#include "stdio.h"
#include <iostream>
namespace photo{
int makePhoto(std::string out_file)
{
    int result = 0;
    FILE* file = fopen(out_file.c_str(), "wb");
    AVFrame* frameRGB;
    struct SwsContext* sws_ctx;

    FrameKeeper& fk = FrameKeeper::Instance();
    AVFrame* frame = fk.getFrame();

    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        AV_PIX_FMT_YUV420P, //static_cast<AVPixelFormat>(frame->format),
        frame->width,
        frame->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    frameRGB = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, frame->width,
                    frame->height);
    uint8_t* buffer=new uint8_t[numBytes*sizeof(uint8_t)];

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frameRGB, buffer, AV_PIX_FMT_RGB24,
           frame->width, frame->height);

    // setup frame sizes
    frameRGB->width = frame->width;
    frameRGB->height = frame->height;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
        ((AVPicture*)frame)->data,
        ((AVPicture*)frame)->linesize,
        0,
        frame->height,
        ((AVPicture *)frameRGB)->data,
        ((AVPicture *)frameRGB)->linesize);

//     Write header
    fprintf(file, "P6\n%d %d\n255\n", frameRGB->width, frameRGB->height);

//     Write pixel data
    for(int y=0; y< frameRGB->height; y++)
    fwrite(frameRGB->data[0]+y*frameRGB->linesize[0], 1, frameRGB->width*3, file);

    fclose(file);
    return result;
}
}
