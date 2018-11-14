
#include "decoder.hpp"

void decoder::run_decoding()
{
    while(av_read_frame(format_ctx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==video_stream) {
            // Decode video frame
            avcodec_decode_video2(codec_ctx, frame, &frame_finished, &packet);
            // Did we get a video frame?
            if(frame_finished) {
                keeper->assig_new_frame(frame);
                av_log(nullptr, AV_LOG_DEBUG, "Decoder new frame pts: [%ld], pkt_dts: [%ld],  pkt_pts: [%ld], timestamp: [%ld]\n",
                        frame->pts, frame->pkt_dts, frame->pkt_pts, frame->best_effort_timestamp);
                av_frame_unref(frame);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        if (stop_flag) {
            break;
        }
    }
    stop_flag.exchange(false);
}

std::string decoder::construct_error(std::string what){
    std::string error("{ \"error\": \"");
    error += what;
    error += "\"}";
    av_log(nullptr, AV_LOG_DEBUG, "Decoder error. [%s]\n", what.c_str());
    return error;
}

bool decoder::is_initialized() const
{
    return initialized;
}

decoder::decoder(decoder_settings set)
    : filename(set.input_file), format(set.format),
      format_ctx(NULL),file_iformat(NULL),codec_ctx(NULL),
      codec(NULL), frame(NULL), initialized(false),
      width(-1), height(-1), bit_rate(-1)
{
    stop_flag.exchange(false);
    keeper = std::make_shared<frame_keeper>();
    input_time_base.den = set.time_base_den;
    input_time_base.num = set.time_base_num;
}

decoder::~decoder()
{
    stop_decoding();
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);
    av_frame_free(&frame);
}

std::string decoder::init()
{
    // Register all formats and codecs
    av_register_all();
    avdevice_register_all();

    // determine format context
    format_ctx = avformat_alloc_context();
    if (!format_ctx) {
        return construct_error("Memory error");
    }

    file_iformat = av_find_input_format(format.c_str());
    if (file_iformat == NULL) {
        return construct_error("Unknown input format: " + format);
    }

    // Open video file
    if(avformat_open_input(&format_ctx, filename.c_str(), file_iformat, NULL)!=0){
        if(avformat_open_input(&format_ctx, filename.c_str(), NULL, NULL)!=0){
            return construct_error("Can't Open video file anyway");
        }
    }

    // Retrieve stream information
    if(avformat_find_stream_info(format_ctx, NULL)<0) {
        return construct_error("Can't Retrieve stream information");
    }

    // Dump information about file onto standard error
    av_dump_format(format_ctx, 0, filename.c_str(), 0);

    // Find the first video stream
    video_stream=-1;
    for(int i=0; i<format_ctx->nb_streams; i++)
    if(format_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      video_stream=i;
      break;
    }
    if(video_stream==-1) {
        return construct_error("Can't find streams");
    }

    // Get a pointer to the codec context for the video stream
    codec_ctx=format_ctx->streams[video_stream]->codec;

    // Find the decoder for the video stream
    codec=avcodec_find_decoder(codec_ctx->codec_id);
    if(codec==NULL) {
        return construct_error("Unsupported codec!");
    }

    // Open codec
    if(avcodec_open2(codec_ctx, codec, NULL)<0) {
        printf("Can't open codec\n");
        return construct_error("Can't open codec"); // Could not open codec
    }

    // Allocate video frame
    frame=av_frame_alloc();

    if (-1 == input_time_base.den || -1 == input_time_base.num) {
        input_time_base = codec_ctx->time_base;
    }

    av_log(nullptr, AV_LOG_DEBUG, "Decoder Time base: [%d/%d]\n",
           input_time_base.den, input_time_base.num);

    initialized = true;
    keeper->setup_time_base(input_time_base);
    av_log(nullptr, AV_LOG_DEBUG, "Decoder time base setted to keeper. Time base: [%d/%d]\n",
           keeper->get_time_base().den, keeper->get_time_base().num);
    return std::string{};
}

void decoder::start_decoding()
{
    decoder_thread = std::thread([this] {return this->run_decoding();});
}

void decoder::stop_decoding()
{
    stop_flag.exchange(true);
    if (decoder_thread.joinable()) {
        decoder_thread.join();
    }
}

int decoder::get_bit_rate()
{
    return bit_rate;
}

int decoder::get_width()
{
    return width;
}

int decoder::get_height()
{
    return height;
}

std::shared_ptr<frame_keeper> decoder::get_keeper(){
    return keeper;
}

