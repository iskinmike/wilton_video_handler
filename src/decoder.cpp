
#include "decoder.hpp"
#include "frame_keeper.hpp"

void decoder::run_decoding()
{
    bool first_run = true;
    uint64_t pts_offset = 0;
    while(av_read_frame(format_ctx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==video_stream) {
            // Decode video frame
            avcodec_decode_video2(codec_ctx, frame, &frame_finished, &packet);
            // Did we get a video frame?
            if(frame_finished) {
                // Convert the image from its native format to AV_PIX_FMT_YUV420P
                sws_scale(sws_ctx,
                    ((AVPicture*)frame)->data, ((AVPicture*)frame)->linesize, 0,
                    codec_ctx->height, ((AVPicture *)frame_out)->data,
                    ((AVPicture *)frame_out)->linesize);
                if (first_run) {
                    pts_offset = frame->best_effort_timestamp;
                    first_run =  false;
                }

                frame_out->pts = frame->best_effort_timestamp - pts_offset;
                frame_out->format = AV_PIX_FMT_YUV420P;
                frame_out->width = width;
                frame_out->height = height;
                frame_out->pkt_dts = frame->pkt_dts;
                frame_out->pkt_pts = frame->pkt_pts;
                frame_out->pkt_duration = frame->pkt_duration;
                
                frame_keeper& fk = frame_keeper::instance();
                fk.assig_new_frames(frame_out, frame);
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

decoder::~decoder()
{
    stop_decoding();
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);
    av_frame_free(&frame);
    av_frame_free(&frame_out);
    delete[] buffer;
    sws_freeContext(sws_ctx);
}

std::string decoder::init()
{
    // Register all formats and codecs
    av_register_all();
    avdevice_register_all();

    // determine format context
    format_ctx = avformat_alloc_context();
    if (!format_ctx) {
        return std::string("Memory error");
    }

    file_iformat = av_find_input_format(format.c_str());
    if (file_iformat == NULL) {
        return std::string("Unknown input format: ") + format; 
    }

    // Open video file
    if(avformat_open_input(&format_ctx, filename.c_str(), file_iformat, NULL)!=0){
      if(avformat_open_input(&format_ctx, filename.c_str(), NULL, NULL)!=0){
          return std::string("Can't Open video file anyway");
      }
    }

    // Retrieve stream information
    if(avformat_find_stream_info(format_ctx, NULL)<0) {
      return std::string("Can't Retrieve stream information");
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
        return std::string("Can't find streams");
    }

    // Get a pointer to the codec context for the video stream
    codec_ctx=format_ctx->streams[video_stream]->codec;

    // Find the decoder for the video stream
    codec=avcodec_find_decoder(codec_ctx->codec_id);
    if(codec==NULL) {
      return std::string("Unsupported codec!");
    }

    // Open codec
    if(avcodec_open2(codec_ctx, codec, NULL)<0) {
        printf("Can't open codec\n");
        return std::string("Can't open codec"); // Could not open codec
    }

    // Allocate video frame
    frame=av_frame_alloc();

    // Allocate an AVFrame structure
    frame_out=av_frame_alloc();
    if(frame_out==NULL){
        return std::string("Can't allocate frame");
    }

    // setup omitted settings if exists
    if (-1 == width) width = codec_ctx->width;
    if (-1 == height) height = codec_ctx->height;
    if (-1 == bit_rate) bit_rate = codec_ctx->bit_rate;

    // Determine required buffer size and allocate buffer
    num_bytes=avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
    buffer = new uint8_t[num_bytes*sizeof(uint8_t)];

    sws_ctx = sws_getContext
        (
            codec_ctx->width,
            codec_ctx->height,
            codec_ctx->pix_fmt,
            width, // new frame width
            height, // new frame height
            AV_PIX_FMT_YUV420P,
            SWS_BICUBIC,
            NULL,
            NULL,
            NULL
        );

    // Assign appropriate parts of buffer to image planes in pFrameOut
    // Note that pFrameOut is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)frame_out, buffer, AV_PIX_FMT_YUV420P, width, height);

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

