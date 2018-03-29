#include "photo.hpp"
#include "frame_keeper.hpp"
#include "stdio.h"
#include <iostream>

namespace photo{
std::string make_photo(std::string out_file)
{
    AVFrame* frame_rgb;
    struct SwsContext* sws_ctx;

//    frame_keeper& fk = frame_keeper::instance();
    auto fk = shared_frame_keeper();
    AVFrame* frame = fk->get_frame_keeper()->get_origin_frame();

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
    frame_rgb->format = AV_PIX_FMT_RGB24;
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
    if (0 > ret) return std::string("Could not allocate output_context for photo");

    // find encoder codec
    encode_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encode_codec) return std::string("Codec not found for photo");

    out_stream = avformat_new_stream(out_format_ctx, encode_codec);
    if (!out_stream) return std::string("Could not allocate stream for photo");

    // settings
    out_stream->codec->pix_fmt = AV_PIX_FMT_RGB24;
    out_stream->codec->codec_id = encode_codec->id;
    out_stream->codec->gop_size = 1;/* emit one intra frame every twelve frames at most */
    out_stream->codec->width = frame_rgb->width;
    out_stream->codec->height = frame_rgb->height;
    out_stream->id = out_format_ctx->nb_streams-1;

    // need open codec
    if(avcodec_open2(out_stream->codec, encode_codec, NULL)<0) {
        return std::string("Can't open codec to encode photo");
    }

    // open file to write
    ret = avio_open(&(out_format_ctx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return (std::string("Could not open ") + out_file);
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
        return std::string("photo encoder error");
    }

    av_write_frame(out_format_ctx, &tmp_pack);

    av_write_trailer(out_format_ctx);
    avio_closep(&(out_format_ctx->pb));

    av_dump_format(out_format_ctx, 0, out_file.c_str(), 1);

    av_frame_free(&frame);
    av_frame_free(&frame_rgb);

    avcodec_close(out_stream->codec);
    avformat_flush(out_format_ctx);
    // automatically set pOutFormatCtx to NULL and frees all its allocated data
    avformat_free_context(out_format_ctx);

    return std::string{};
}
}
