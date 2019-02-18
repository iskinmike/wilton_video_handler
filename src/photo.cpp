#include "photo.hpp"
#include "frame_keeper.hpp"
#include "stdio.h"
#include <iostream>

#include "utils.hpp"

namespace photo{

std::string make_photo(std::string out_file, int photo_width, int photo_height, std::shared_ptr<frame_keeper> keeper)
{
    AVFrame* frame = keeper->get_frame();

    if (nullptr == frame) {
        return utils::construct_error("Can't make Photo. Get 'nullptr'' frame.");
    }

    // prepare rgb frame
    const int not_set = -1;
    int width = (not_set != photo_width) ? photo_width : frame->width;
    int height = (not_set != photo_height) ? photo_height : frame->height;
    AVPixelFormat new_format = AV_PIX_FMT_RGB24;

    utils::frame_rescaler rescaler(width, height, new_format);
    AVFrame* frame_rgb = rescaler.rescale_frame(frame);

    frame_rgb->width = width;
    frame_rgb->height = height;
    frame_rgb->format = new_format;
    frame_rgb->pts = 0;

    /* allocate the output media context */
    AVFormatContext* out_format_ctx = nullptr;
    AVCodec* encode_codec = nullptr;
    AVStream* out_stream = nullptr;

    int ret = 0;
    ret = avformat_alloc_output_context2(&out_format_ctx, nullptr, nullptr, out_file.c_str());
    if (0 > ret) return utils::construct_error("Could not allocate output_context for photo");

    // find encoder codec
    encode_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!encode_codec) return utils::construct_error("Codec not found for photo");

    out_stream = avformat_new_stream(out_format_ctx, encode_codec);
    if (!out_stream) return utils::construct_error("Could not allocate stream for photo");

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
    if(avcodec_open2(out_stream->codec, encode_codec, nullptr)<0) {
        return utils::construct_error("Can't open codec to encode photo");
    }

    // open file to write
    ret = avio_open(&(out_format_ctx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return utils::construct_error(std::string("Could not open ") + out_file);
    }

    // header is musthave for this
    avformat_write_header(out_format_ctx, nullptr);

    // prepare packet
    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = nullptr; // for autoinit
    tmp_pack.size = 0;
    int got_pack = 0;

    ret = avcodec_encode_video2(out_stream->codec, &tmp_pack, frame_rgb, &got_pack);
    if (0 != ret) {
        return utils::construct_error("photo encoder error");
    }

    av_write_frame(out_format_ctx, &tmp_pack);
    // trailer is musthave
    av_write_trailer(out_format_ctx);
    avio_closep(&(out_format_ctx->pb));

    // free all allocated memory
    av_frame_free(&frame);
    av_frame_free(&frame_rgb);
    av_free_packet(&tmp_pack);

    avcodec_close(out_stream->codec);
    avformat_flush(out_format_ctx);
    // automatically set out_format_ctx to nullptr and frees all its allocated data
    avformat_free_context(out_format_ctx);
    return std::string{};
}
} // photo
