
#include "encoder.hpp"
#include "utils.hpp"

#include <iostream>
#include <cmath>

#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
        #define AV_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
#endif

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

void encoder::run_encoding()
{
    input_time_base = get_time_base_from_keeper();
    first_run = true;
    pts_offset = 0;
    while (!stop_flag) {
        AVFrame *tmp_frame = get_frame_from_keeper();
        if (nullptr != tmp_frame) {
            rescale_frame(tmp_frame);
            encode_frame(frame_out);
            av_frame_free(&tmp_frame);
            av_frame_free(&frame_out);
            delete[] buffer;
        }
    }
    stop_flag.exchange(false);
    encoding_started.exchange(false);
}

AVFrame *encoder::get_frame_from_keeper() {
    std::lock_guard<std::mutex> guard(mtx);
    return keeper->get_frame();
}

AVRational encoder::get_time_base_from_keeper(){
    return keeper->get_time_base();
}

void encoder::rescale_frame(AVFrame *frame){
    frame_out=av_frame_alloc();

    // Determine required buffer size and allocate buffer
    num_bytes=avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
    buffer = new uint8_t[num_bytes*sizeof(uint8_t)];

    // Assign appropriate parts of buffer to image planes in pFrameOut
    // Note that pFrameOut is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)frame_out, buffer, AV_PIX_FMT_YUV420P, width, height);

    if (first_run) {
        first_run = false;
        pts_offset = frame->best_effort_timestamp - last_time;
        frame->key_frame = 1;
    }
    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        static_cast<AVPixelFormat>(frame->format),
        width,
        height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    sws_scale(sws_ctx,
              ((AVPicture*)frame)->data, ((AVPicture*)frame)->linesize, 0,
              frame->height, ((AVPicture *)frame_out)->data,
              ((AVPicture *)frame_out)->linesize);

    frame_out->pts = frame->best_effort_timestamp - pts_offset;
    frame_out->format = AV_PIX_FMT_YUV420P;
    frame_out->width = width;
    frame_out->height = height;
    frame_out->pkt_dts = frame->pkt_dts;
    frame_out->pkt_pts = frame->pkt_pts;
    frame_out->pkt_duration = frame->pkt_duration;

    sws_freeContext(sws_ctx);
}

bool encoder::is_initialized() const
{
    return initialized;
}

void encoder::setup_frame_keeper(std::shared_ptr<frame_keeper> keeper){
    if (encoding_started) {
        pause_encoding();
        {
            std::lock_guard<std::mutex> guard(mtx);
            this->keeper = keeper;
        }
        start_encoding();
    } else {
        std::lock_guard<std::mutex> guard(mtx);
        this->keeper = keeper;
    }
}

encoder::encoder(encoder_settings set)
    : encode_codec(NULL), out_file(set.output_file), initialized(false),
      out_format_ctx(NULL), out_stream(NULL), pts_flag(false), last_pts(-1), last_time(0),
      bit_rate(set.bit_rate), width(set.width), height(set.height), framerate(set.framerate)
{
    stop_flag.exchange(false);
    encoding_started.exchange(false);
}

encoder::~encoder()
{
    stop_encoding();
    if (initialized) {
        avcodec_close(out_stream->codec);
        avformat_flush(out_format_ctx);
        // automatically set pOutFormatCtx to NULL and frees all its allocated data
        avformat_free_context(out_format_ctx);
        av_frame_free(&frame_out);
        delete[] buffer;
    }
}

// OutFormat And OutStream based on muxing.c example by Fabrice Bellard
std::string encoder::init()
{
    const double default_framerate = 25.0;
    const int framerate_max_allowed_num_denum = 100;
    framerate = (framerate >= 0) ? framerate : default_framerate;

    /* allocate the output media context */
    int ret = 0;
    ret = avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, out_file.c_str());
    if (0 > ret) {
        ret = avformat_alloc_output_context2(&out_format_ctx, NULL, "avi", NULL);
    }
    if (0 > ret) return utils::construct_error("Could not allocate output_context");


    // find encoder codec
    encode_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
//    encode_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!encode_codec) return utils::construct_error("Codec not found");

    out_stream = avformat_new_stream(out_format_ctx, encode_codec);
    if (!out_stream) return utils::construct_error("Could not allocate stream");

    const int half_divider = 2;
    // set Context settings
    out_stream->codec->codec_id = encode_codec->id;
    out_stream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    out_stream->codec->gop_size = std::lround(this->framerate/half_divider);/* emit every frame as intra frame*/
    out_stream->codec->time_base = av_inv_q(av_d2q(this->framerate, framerate_max_allowed_num_denum));

    out_stream->codec->width = width;
    out_stream->codec->height = height;
    out_stream->codec->max_b_frames = 0;
    out_stream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    out_stream->codec->bit_rate = bit_rate;
    out_stream->codec->bit_rate_tolerance = bit_rate * av_q2d(out_stream->codec->time_base);
    out_stream->codec->rc_buffer_size = bit_rate*10; // this is emperical value to contro bitrate
    out_stream->codec->rc_max_rate = bit_rate; // allows to control bitrate
    out_stream->codec->rc_min_rate = bit_rate; // allows to control bitrate
    out_stream->codec->ticks_per_frame = 2; // for H.264 codec

    out_stream->id = out_format_ctx->nb_streams-1;

    // Container requires header but codec not, so we need to setup flag for codec to supress warning.
    out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // need open codec
    if(avcodec_open2(out_stream->codec, encode_codec, NULL)<0) {
        return utils::construct_error("Can't open codec to encode");
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
        return utils::construct_error("Could not open " + out_file);
    }
    // header is musthave for this
    avformat_write_header(out_format_ctx, NULL);

    initialized = true;
    return std::string{};
}

std::string encoder::start_encoding()
{
    // start wait to frame keeper signal
    if (keeper && !encoding_started){
        encoder_thread = std::thread([this] {return this->run_encoding();});
        encoding_started.exchange(true);
    } else {
        return std::string{"{ \"error\": \"Decoder not setted\"}"};
    }
    return std::string();
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

void encoder::pause_encoding(){
    stop_flag.exchange(true);
    if (encoder_thread.joinable()) {
        encoder_thread.join();
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
        last_time = frame->pts;
        frame->pts = av_rescale_q(frame->pts, input_time_base, out_stream->codec->time_base);
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
