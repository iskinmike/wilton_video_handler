#include "photo.hpp"
#include "frame_keeper.hpp"
#include "stdio.h"
#include <iostream>

namespace photo{

namespace {
    std::string error_return(std::string message){
        auto error = std::string{"{ \"error\": \""};
        error += message;
        error += "\"}";
        return error;
    }
    const int not_set = -1;
}

std::string make_photo(std::string out_file, int photo_width, int photo_height, AVFrame* frame) {
    AVFrame* frame_rgb;
    struct SwsContext* sws_ctx;

    int width = (not_set != photo_width) ? photo_width : frame->width;
    int height = (not_set != photo_height) ? photo_height : frame->height;
    AVPixelFormat new_format = AV_PIX_FMT_RGB24;

    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        static_cast<AVPixelFormat>(frame->format),
        width,
        height,
        new_format,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    frame_rgb = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(new_format, width,
                    height);
    uint8_t* buffer=new uint8_t[numBytes*sizeof(uint8_t)];

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frame_rgb, buffer, new_format,
           width, height);

    // setup frame sizes
    frame_rgb->width = width;
    frame_rgb->height = height;
    frame_rgb->format = new_format;
    frame_rgb->pts = 0;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
        ((AVPicture*)frame)->data,
        ((AVPicture*)frame)->linesize,
        0,
        frame->height,
        ((AVPicture *)frame_rgb)->data,
        ((AVPicture *)frame_rgb)->linesize);

    /* allocate the output media context */
    AVFormatContext* out_format_ctx = NULL;
    AVCodec* encode_codec = NULL;
    AVStream* out_stream = NULL;

    int ret = 0;
    ret = avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, out_file.c_str());
    if (0 > ret) return error_return("Could not allocate output_context for photo");

    // find encoder codec
    encode_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encode_codec) return error_return("Codec not found for photo");

    out_stream = avformat_new_stream(out_format_ctx, encode_codec);
    if (!out_stream) return error_return("Could not allocate stream for photo");

    // settings
    out_stream->codec->pix_fmt = new_format;
    out_stream->codec->codec_id = encode_codec->id;
    out_stream->codec->gop_size = 1;/* emit one intra frame every twelve frames at most */
    out_stream->codec->width = frame_rgb->width;
    out_stream->codec->height = frame_rgb->height;
    out_stream->id = out_format_ctx->nb_streams-1;
    out_stream->codec->time_base = av_make_q(1,15);
    out_stream->time_base = out_stream->codec->time_base;

    // need open codec
    if(avcodec_open2(out_stream->codec, encode_codec, NULL)<0) {
        return error_return("Can't open codec to encode photo");
    }

    // open file to write
    ret = avio_open(&(out_format_ctx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return error_return(std::string("Could not open ") + out_file);
    }
    // header is musthave for this
    avformat_write_header(out_format_ctx, NULL);

    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = NULL; // for autoinit
    tmp_pack.size = 0;
    int got_pack = 0;

    ret = avcodec_encode_video2(out_stream->codec, &tmp_pack, frame_rgb, &got_pack);
    if (0 != ret) {
        return error_return("photo encoder error");
    }

    av_write_frame(out_format_ctx, &tmp_pack);

    av_write_trailer(out_format_ctx);
    avio_closep(&(out_format_ctx->pb));

    sws_freeContext(sws_ctx);

    av_frame_free(&frame_rgb);
    av_free_packet(&tmp_pack);

    avcodec_close(out_stream->codec);
    avformat_flush(out_format_ctx);
    // automatically set pOutFormatCtx to NULL and frees all its allocated data
    avformat_free_context(out_format_ctx);

    delete[] buffer;

    return std::string{};
}

std::string make_photo(std::string out_file, int photo_width, int photo_height, std::shared_ptr<frame_keeper> keeper) {
    AVFrame* frame = keeper->get_frame();
    if (nullptr == frame) {
        return error_return("Can't make Photo. Get 'NULL'' frame.");
    }
    auto result = make_photo(out_file, photo_width, photo_height, frame);
    av_frame_free(&frame);
    return result;
}
}
