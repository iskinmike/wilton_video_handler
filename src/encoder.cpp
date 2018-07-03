
#include "encoder.hpp"
#include "frame_keeper.hpp"
#include <iostream>

#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
        #define AV_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
#endif

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);

    // On windows there is 10 times error for timing without this hack video will be 10 times longer, than standard video
#ifdef WIN32
    pkt->pts = pkt->pts/uint64_t(10);
    pkt->dts = pkt->dts/uint64_t(10);
#endif

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

void encoder::run_encoding()
{
    frame_keeper& fk = frame_keeper::instance();
    while (!stop_flag) {
        AVFrame *tmp_frame = fk.get_frame();
        encode_frame(tmp_frame);
        av_frame_free(&tmp_frame);
    }
    stop_flag.exchange(false);
}

bool encoder::is_initialized() const
{
    return initialized;
}

encoder::~encoder()
{
    stop_encoding();
    if (initialized) {
        avcodec_close(out_stream->codec);
        avformat_flush(out_format_ctx);
        // automatically set pOutFormatCtx to NULL and frees all its allocated data
        avformat_free_context(out_format_ctx);
    }
}

// OutFormat And OutStream based on muxing.c example by Fabrice Bellard
std::string encoder::init(int bit_rate, int width, int height, double framerate)
{
    this->bit_rate = bit_rate;
    this->width = width;
    this->height = height;
    const double default_framerate = 25.0;
    const int framerate_max_allowed_num_denum = 100;
    this->framerate = (framerate >= 0) ? framerate : default_framerate;

    /* allocate the output media context */
    int ret = 0;
    ret = avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, out_file.c_str());
    if (0 > ret) {
        ret = avformat_alloc_output_context2(&out_format_ctx, NULL, "avi", NULL);
    }
    if (0 > ret) return std::string("Could not allocate output_context");


    // find encoder codec
    encode_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
//    encode_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!encode_codec) return std::string("Codec not found");

    out_stream = avformat_new_stream(out_format_ctx, encode_codec);
    if (!out_stream) return std::string("Could not allocate stream");

    // set Context settings
    out_stream->codec->codec_id = encode_codec->id;
    out_stream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    out_stream->codec->gop_size = 0;/* emit every frame as intra frame*/
    out_stream->codec->time_base = av_inv_q(av_d2q(this->framerate, framerate_max_allowed_num_denum));
    out_stream->codec->bit_rate = bit_rate;
    out_stream->codec->width = width;
    out_stream->codec->height = height;
    out_stream->codec->max_b_frames = 0;
    out_stream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    out_stream->codec->bit_rate_tolerance = bit_rate;
    out_stream->codec->rc_buffer_size = bit_rate*10; // this is emperical value to contro bitrate
    out_stream->codec->rc_max_rate = bit_rate; // allows to control bitrate
    out_stream->codec->rc_min_rate = bit_rate; // allows to control bitrate
    out_stream->codec->ticks_per_frame = 2; // for H.264 codec

    out_stream->id = out_format_ctx->nb_streams-1;

    // Container requires header but codec not, so we need to setup flag for codec to supress warning.
    out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // need open codec
    if(avcodec_open2(out_stream->codec, encode_codec, NULL)<0) {
        return std::string("Can't open codec to encode");
    }

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. Actually this setup now only suppress deprecated warning.
     * time_base resetted by libavcodec after avformat_write_header() so
     * pOutStream->time_base not equal to pOutStream->codec->time_base after that call */
    out_stream->time_base = out_stream->codec->time_base;

//    av_dump_format(out_format_ctx, 0, out_file.c_str(), 1);

    // open file to write
    ret = avio_open(&(out_format_ctx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return (std::string("Could not open ") + out_file);
    }
    // header is musthave for this
    avformat_write_header(out_format_ctx, NULL);
    initialized = true;
    return std::string{};
}

void encoder::start_encoding()
{
    // start wait to frame keeper signal
    encoder_thread = std::thread([this] {return this->run_encoding();});
}

void encoder::stop_encoding()
{
    stop_flag.exchange(true);
    if (encoder_thread.joinable()) {
        encoder_thread.join();
    }
    if (initialized) {
        fflush_encoder();
        close_file();
    }
}

int encoder::encode_frame(AVFrame* frame)
{
    int out_size = 0;
    int got_pack = 0;

    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = NULL; // for autoinit
    tmp_pack.size = 0;
    uint64_t tmp = 0;
    if (frame != NULL) {
        // Based on: https://stackoverflow.com/questions/11466184/setting-video-bit-rate-through-ffmpeg-api-is-ignored-for-libx264-codec
        // also on ffmpeg documentation  doc/example/muxing.c and remuxing.c
        frame->pts = av_rescale_q(frame->pts, av_make_q(1, AV_TIME_BASE), out_stream->codec->time_base);
        //frame->pts = av_rescale_q(frame->pts, AV_TIME_BASE_Q, out_stream->codec->time_base);
        tmp = av_rescale_q(frame->pts, out_stream->codec->time_base, out_stream->time_base);
        // skip some frames
        if (last_pts == tmp) {
            return 0;
        }
        last_pts = tmp;
    }

    out_size = avcodec_encode_video2(out_stream->codec, &tmp_pack, frame, &got_pack);

    if (got_pack) {
        write_frame(out_format_ctx, &out_stream->codec->time_base, out_stream, &tmp_pack);
    }

    av_free_packet(&tmp_pack);
    return got_pack;
}

void encoder::close_file()
{
    if (NULL != out_format_ctx) {
        // need to write trailer to finalize video file
        if (NULL != out_format_ctx->pb){
            av_write_trailer(out_format_ctx);
            avio_closep(&(out_format_ctx->pb));
        }
    }
}

void encoder::fflush_encoder()
{
    while (encode_frame(NULL));
}
