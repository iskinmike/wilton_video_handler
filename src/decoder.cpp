
#include "decoder.hpp"

void Decoder::runDecoding()
{
    bool first_run = true;
    uint64_t pts_offset = 0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to AV_PIX_FMT_YUV420P
                sws_scale(sws_ctx,
                    ((AVPicture*)pFrame)->data, ((AVPicture*)pFrame)->linesize, 0,
                    pCodecCtx->height, ((AVPicture *)pFrameOut)->data,
                    ((AVPicture *)pFrameOut)->linesize);
                if (first_run) {
                    pts_offset = pFrame->best_effort_timestamp;
                    first_run =  false;
                }

                pFrameOut->pts = pFrame->best_effort_timestamp - pts_offset;
                pFrameOut->format = AV_PIX_FMT_YUV420P;
                pFrameOut->width = width;
                pFrameOut->height = height;
                pFrameOut->pkt_dts = pFrame->pkt_dts;
                pFrameOut->pkt_pts = pFrame->pkt_pts;
                pFrameOut->pkt_duration = pFrame->pkt_duration;
                
                FrameKeeper& fk = FrameKeeper::Instance();
                fk.assigNewFrames(pFrameOut, pFrame);
                av_frame_unref(pFrame);
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

Decoder::~Decoder()
{
    stopDecoding();
    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameOut);
    delete[] buffer;
    sws_freeContext(sws_ctx);
}

std::string Decoder::init()
{
    // Register all formats and codecs
    av_register_all();
    avdevice_register_all();

    // determine format context
    pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx) {
        return std::string("Memory error");
    }

    file_iformat = av_find_input_format(format.c_str());
    if (file_iformat == NULL) {
        return std::string("Unknown input format: ") + format; 
    }

    // Open video file
    if(avformat_open_input(&pFormatCtx, filename.c_str(), file_iformat, NULL)!=0){
      if(avformat_open_input(&pFormatCtx, filename.c_str(), NULL, NULL)!=0){
          return std::string("Can't Open video file anyway");
      }
    }

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
      return std::string("Can't Retrieve stream information");
    }

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, filename.c_str(), 0);

    // Find the first video stream
    videoStream=-1;
    for(int i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
    if(videoStream==-1) {
        return std::string("Can't find streams");
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
      return std::string("Unsupported codec!");
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0) {
        printf("Can't open codec\n");
        return std::string("Can't open codec"); // Could not open codec
    }

    // Allocate video frame
    pFrame=av_frame_alloc();

    // Allocate an AVFrame structure
    pFrameOut=av_frame_alloc();
    if(pFrameOut==NULL){
        return std::string("Can't allocate frame");
    }

    // setup omitted settings if exists
    if (-1 == width) width = pCodecCtx->width;
    if (-1 == height) height = pCodecCtx->height;
    if (-1 == bit_rate) bit_rate = pCodecCtx->bit_rate;

    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(AV_PIX_FMT_YUV420P, width, height);
    buffer = new uint8_t[numBytes*sizeof(uint8_t)];

    sws_ctx = sws_getContext
        (
            pCodecCtx->width,
            pCodecCtx->height,
            pCodecCtx->pix_fmt,
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
    avpicture_fill((AVPicture *)pFrameOut, buffer, AV_PIX_FMT_YUV420P, width, height);

    return std::string{};
}

void Decoder::startDecoding()
{
    decoder_thread = std::thread([this] {return this->runDecoding();});
}

void Decoder::stopDecoding()
{
    stop_flag.exchange(true);
    if (decoder_thread.joinable()) {
        decoder_thread.join();
    }
}

int Decoder::getBitRate()
{
    return bit_rate;
}

int Decoder::getWidth()
{
    return width;
}

int Decoder::getHeight()
{
    return height;
}

