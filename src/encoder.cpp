
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
    first_run = true;
    pts_offset = 0;
    frame_out = av_frame_alloc();
    while (!stop_flag) {
        AVFrame *tmp_frame = get_frame_from_keeper();
        if (nullptr != tmp_frame) {
            rescale_frame(tmp_frame);
            encode_frame(frame_out);
            av_frame_free(&tmp_frame);
        }
    }
    av_frame_free(&frame_out);
    stop_flag.exchange(false);
    encoding_started.exchange(false);
}

AVFrame *encoder::get_frame_from_keeper() {
    std::lock_guard<std::mutex> guard(mtx);
    return keeper->get_frame();
}

AVRational encoder::get_time_base_from_keeper(){
    av_log(nullptr, AV_LOG_DEBUG, "Encoder get time base from keeper. Time base: [%d/%d]\n",
           keeper->get_time_base().den, keeper->get_time_base().num);
    return keeper->get_time_base();
}

void encoder::rescale_frame(AVFrame *frame) {
    if (first_run) {
        first_run = false;
        pts_offset = frame->best_effort_timestamp - last_time;
        frame->key_frame = 1;
    }

    auto tmp_pts = av_rescale_q(frame->pts, input_time_base, out_stream->codec->time_base);
    if (last_pts != tmp_pts) { // this check repeated in encode_frame but i think it allows not to do usless work and keeps code simple. I tried to rearrange but its is my most simple idea
        rescaler.rescale_frame_to_existed(frame, frame_out);
    }

    frame_out->pts = frame->best_effort_timestamp - pts_offset;
    frame_out->pkt_dts = frame->pkt_dts;
    frame_out->pkt_pts = frame->pkt_pts;
    frame_out->pkt_duration = frame->pkt_duration;
    frame_out->best_effort_timestamp = frame->best_effort_timestamp;
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
            input_time_base = get_time_base_from_keeper();
        }
        start_encoding();
    } else {
        std::lock_guard<std::mutex> guard(mtx);
        this->keeper = keeper;
        input_time_base = get_time_base_from_keeper();
    }
}

std::string encoder::get_out_file(){
    return out_file;
}

encoder::encoder(encoder_settings set)
    : encode_codec(nullptr), out_file(set.output_file), initialized(false),
      out_format_ctx(nullptr), out_stream(nullptr), pts_flag(false), last_pts(-1), last_time(0),
      bit_rate(set.bit_rate), width(set.width), height(set.height), framerate(set.framerate), rescaler(set.width, set.height, AV_PIX_FMT_YUV420P)
{
    stop_flag.exchange(false); // to avoid warnings from VS2013
    encoding_started.exchange(false);
    input_time_base.den = 999999; // This value is used to distinguish the specified parameters from the default parameters.
    input_time_base.num = 1;
}

encoder::~encoder()
{
    stop_encoding();
    if (initialized) {
        avcodec_close(out_stream->codec);
        avformat_flush(out_format_ctx);
        // automatically set out_format_ctx to nullptr and frees all its allocated data
        avformat_free_context(out_format_ctx);
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
    ret = avformat_alloc_output_context2(&out_format_ctx, nullptr, nullptr, out_file.c_str());
    if (0 > ret) {
        ret = avformat_alloc_output_context2(&out_format_ctx, nullptr, "avi", nullptr);
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
    out_stream->codec->gop_size = std::lround(this->framerate/half_divider); /*emit 2 intra frames every second*/
    out_stream->codec->time_base =av_inv_q(av_d2q(this->framerate, framerate_max_allowed_num_denum)); // framerate can't be more than camera framerate

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
    if(0 > avcodec_open2(out_stream->codec, encode_codec, nullptr)) {
        return utils::construct_error("Can't open codec to encode");
    }

    av_log(nullptr, AV_LOG_DEBUG, "Encoder format. Time base: [%d/%d]\n",
           out_stream->codec->time_base.den, out_stream->codec->time_base.num);

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. Actually this setup now only suppress deprecated warning.
     * time_base resetted by libavcodec after avformat_write_header() so
     * pOutStream->time_base not equal to pOutStream->codec->time_base after that call */
    out_stream->time_base = out_stream->codec->time_base;

    av_dump_format(out_format_ctx, 0, out_file.c_str(), 1);

    // open file to write
    ret = avio_open(&(out_format_ctx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return utils::construct_error("Could not open " + out_file);
    }
    // header is musthave for this
    avformat_write_header(out_format_ctx, nullptr);

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
        return utils::construct_error("Decoder not setted");
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
    rescaler.clear_sws_context();
}

int encoder::encode_frame(AVFrame* inpt_frame)
{
    int out_size = 0;
    int got_pack = 0;
    AVFrame* frame = inpt_frame;
    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = nullptr; // for autoinit
    tmp_pack.size = 0;
    if (nullptr != frame) {
        // Based on: https://stackoverflow.com/questions/11466184/setting-video-bit-rate-through-ffmpeg-api-is-ignored-for-libx264-codec
        // also on ffmpeg documentation  doc/example/muxing.c and remuxing.c
        last_time = frame->pts; // used to calculate frame time to append frames from another camera
        frame->pts = av_rescale_q(frame->pts, input_time_base, out_stream->codec->time_base);
        // skip frames if they get same pts avcodec will drop it anyway, byt it supress warnings
        if (last_pts == frame->pts) {
            return 0;
        }

        last_pts = frame->pts;
        av_log(nullptr, AV_LOG_DEBUG, "Encoder frame last stream pts: [%ld], rescaled pts: [%ld], effort timestamp: [%ld], "
                                      "input_time_base: [%d/%d], stream base: [%d/%d], codec_base: [%d/%d]\n",
                last_time, frame->pts, frame->best_effort_timestamp,
                input_time_base.den, input_time_base.num,
                out_stream->time_base.den, out_stream->time_base.num,
                out_stream->codec->time_base.den, out_stream->codec->time_base.num
               );
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
    if (nullptr != out_format_ctx) {
        // need to write trailer to finalize video file
        if (nullptr != out_format_ctx->pb){
            av_write_trailer(out_format_ctx);
            avio_closep(&(out_format_ctx->pb));
        }
    }
}

void encoder::fflush_encoder()
{
    while (encode_frame(nullptr));
}
