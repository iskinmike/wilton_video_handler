
#include "encoder.hpp"
#include "frame_keeper.hpp"

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

void Encoder::runEncoding()
{
    FrameKeeper& fk = FrameKeeper::Instance();
    while (!stop_flag) {
        encodeFrame(fk.getFrame());
    }
    stop_flag.exchange(false);
}

Encoder::~Encoder()
{
    stopEncoding();
    avcodec_close(pOutStream->codec);
    avformat_flush(pOutFormatCtx);
    // automatically set pOutFormatCtx to NULL and frees all its allocated data
    avformat_free_context(pOutFormatCtx);
}

// OutFormat And OutStream based on muxing.c example by Fabrice Bellard
std::string Encoder::init(int _bit_rate, int _width, int _height)
{
    bit_rate = _bit_rate;
    width = _width;
    height = _height;

    /* allocate the output media context */
    int ret = 0;
    ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, out_file.c_str());
    if (0 > ret) {
        ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, "avi", NULL);
    }
    if (0 > ret) return std::string("Could not allocate output_context");


    // find encoder codec
    pEncodeCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
//    pEncodeCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!pEncodeCodec) return std::string("Codec not found");

    pOutStream = avformat_new_stream(pOutFormatCtx, pEncodeCodec);
    if (!pOutStream) return std::string("Could not allocate stream");

    // set Context settings
    pOutStream->codec->codec_id = pEncodeCodec->id;
    pOutStream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    pOutStream->codec->gop_size = 12;/* emit one intra frame every twelve frames at most */
    pOutStream->codec->bit_rate = bit_rate;
    pOutStream->codec->width = width;
    pOutStream->codec->height = height;
    pOutStream->codec->time_base = av_make_q(1,30); // empirical value. Lower value cause non monotonical pts errors
    pOutStream->codec->max_b_frames = 0;
    pOutStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    pOutStream->codec->bit_rate_tolerance = bit_rate;
    pOutStream->codec->ticks_per_frame = 2; // for H.264 codec

    pOutStream->id = pOutFormatCtx->nb_streams-1;

    // Container requires header but codec not, so we need to setup flag for codec to supress warning.
    pOutStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // need open codec
    if(avcodec_open2(pOutStream->codec, pEncodeCodec, NULL)<0) {
        return std::string("Can't open codec to encode");
    }

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. Actually this setup now only suppress deprecated warning.
     * time_base resetted by libavcodec after avformat_write_header() so
     * pOutStream->time_base not equal to pOutStream->codec->time_base after that call */
    pOutStream->time_base = pOutStream->codec->time_base;

    av_dump_format(pOutFormatCtx, 0, out_file.c_str(), 1);

    // open file to write
    ret = avio_open(&(pOutFormatCtx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return (std::string("Could not open ") + out_file);
    }
    // header is musthave for this
    avformat_write_header(pOutFormatCtx, NULL);

    return std::string{};
}

void Encoder::startEncoding()
{
    // start wait to frame keeper signal
    encoder_thread = std::thread([this] {return this->runEncoding();});
}

void Encoder::stopEncoding()
{
    stop_flag.exchange(true);
    if (encoder_thread.joinable()) {
        encoder_thread.join();
    }
    fflushEncoder();
    closeFile();
}

int Encoder::encodeFrame(AVFrame* frame)
{
    int out_size = 0;
    int got_pack = 0;

    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = NULL; // for autoinit
    tmp_pack.size = 0;

    if (frame != NULL) {
        // Based on: https://stackoverflow.com/questions/11466184/setting-video-bit-rate-through-ffmpeg-api-is-ignored-for-libx264-codec
        // also on ffmpeg documentation  doc/example/muxing.c and remuxing.c
        frame->pts = av_rescale_q(frame->pts, AV_TIME_BASE_Q, pOutStream->codec->time_base);
    }

    out_size = avcodec_encode_video2(pOutStream->codec, &tmp_pack, frame, &got_pack);

    if (got_pack) {
        write_frame(pOutFormatCtx, &pOutStream->codec->time_base, pOutStream, &tmp_pack);
    }

    av_frame_free(&frame);
    av_free_packet(&tmp_pack);
    return got_pack;
}

void Encoder::closeFile()
{
    if (NULL != pOutFormatCtx) {
        // need to write trailer to finalize video file
        if (NULL != pOutFormatCtx->pb){
            av_write_trailer(pOutFormatCtx);
            avio_closep(&(pOutFormatCtx->pb));
        }
    }
}

void Encoder::fflushEncoder()
{
    while (encodeFrame(NULL));
}
