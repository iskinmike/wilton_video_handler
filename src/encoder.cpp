
#include "encoder.h"
#include "frame_keeper.h"

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
    if (encoder_thread.joinable()) {
        encoder_thread.join();
    }
    closeFile();
    avcodec_close(pEncodeCodecCtx);
    av_free(pEncodeCodecCtx);
}

// OutFormat And OutStream based on muxing.c example by Fabrice Bellard
std::string Encoder::init(int _bit_rate, int _width, int _height)
{
    bit_rate = _bit_rate;
    width = _width;
    height = _height;

    /* allocate the output media context */
    int ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, "avi", NULL);
    if (0 > ret) {
        ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, out_file.c_str());
    }
    if (0 > ret) return std::string("Could not allocate output_context");

    // find encoder codec
    pEncodeCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!pEncodeCodec) return std::string("Codec not found");

    pOutStream = avformat_new_stream(pOutFormatCtx, pEncodeCodec);
    if (!pOutStream) return std::string("Could not allocate stream");

    // set Context and it settings
    pEncodeCodecCtx = avcodec_alloc_context3(pEncodeCodec);
    pEncodeCodecCtx->codec_id = pEncodeCodec->id;
    pEncodeCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pEncodeCodecCtx->gop_size = 1;/* emit one intra frame every twelve frames at most */ //pCodecCtx->gop_size;
    pEncodeCodecCtx->bit_rate = bit_rate;
    pEncodeCodecCtx->width = width;
    pEncodeCodecCtx->height = height;
    pEncodeCodecCtx->time_base = (AVRational){1,25};
    pEncodeCodecCtx->max_b_frames = 0;
    pEncodeCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pEncodeCodecCtx->bit_rate_tolerance = bit_rate;


    pOutStream->id = pOutFormatCtx->nb_streams-1;
    ret = avcodec_copy_context(pOutStream->codec, pEncodeCodecCtx);
    if (0 > ret) {
        return std::string("Failed to copy context from input to output stream codec context");
    }

    if(avcodec_open2(pEncodeCodecCtx, pEncodeCodec, NULL)<0) {
        return std::string("Can't open codec to encode");
    }
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    pOutStream->time_base = pEncodeCodecCtx->time_base;

    av_dump_format(pOutFormatCtx, 0, out_file.c_str(), 1);

    // open file to write
    ret = avio_open(&(pOutFormatCtx->pb), out_file.c_str(), AVIO_FLAG_WRITE);
    if (0 > ret) {
        return (std::string("Could not open ") + out_file);
    }

    // header is musthave for
    avformat_write_header(pOutFormatCtx, NULL);

    return std::string{};
}

void Encoder::startEncoding()
{
    // start wait to frame keeper signal
    cur_pts = 0;
    encoder_thread = std::thread([this] {return this->runEncoding();});
}

void Encoder::stopEncoding()
{
    stop_flag.exchange(true);
    if (encoder_thread.joinable()) {
        encoder_thread.join();
    }
    closeFile();
}

void Encoder::encodeFrame(AVFrame* frame)
{
    if (nullptr == frame) {
        return;
    }

    int out_size = 0;
    int got_pack = 0;

    AVPacket tmp_pack;
    av_init_packet(&tmp_pack);
    tmp_pack.data = NULL; // for autoinit
    tmp_pack.size = 0;

    out_size = avcodec_encode_video2(pEncodeCodecCtx, &tmp_pack, frame, &got_pack);
    // reset pts for MPEG4 format
    tmp_pack.pts = tmp_pack.dts = cur_pts;
    if (got_pack) {
        write_frame(pOutFormatCtx, &pEncodeCodecCtx->time_base, pOutStream, &tmp_pack);
    }
    // increment pts
    cur_pts++;

    av_free(frame);
}

void Encoder::closeFile()
{
    if (NULL != pOutFormatCtx->pb){
        avio_closep(&(pOutFormatCtx->pb));
    }
}
