
#include "decoder.hpp"
#include "utils.hpp"
#include "photo.hpp"
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <string.h> // memset()
#include <errno.h>
#include <linux/types.h>
#include <linux/videodev2.h>


static int xioctl(int fd, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && ((errno == EINTR)
#ifdef AR2VIDEO_V4L2_NONBLOCKING
                         || (errno == EAGAIN)
#endif
                         ));
    if (r == -1) {
        ARLOGperror("ioctl error");
    }
    return (r);
}


bool try_get_buffer(){
    int fd;
    fd = open("/dev/video0", O_RDWR);
    if (fd == -1)
    {
        // couldn't find capture device
//        perror("Opening Video device");
        return false;
    }
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        ARLOGe("Error calling VIDIOC_DQBUF: %d\n", errno);
        return false;
    }
    return true;
}



void decoder::run_decoding()
{
    std::vector<uint8_t> dest_buffer;
    std::vector<uint8_t> buffer;
    while(av_read_frame(format_ctx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==video_stream) {
            // Decode video frame
            avcodec_decode_video2(codec_ctx, frame, &frame_finished, &packet);
            // Did we get a video frame?
            if(frame_finished) {
                bool visible = false;
                bool upd = false;
                // AR_PIXEL_FORMAT_BGRA is AV_PIX_FMT_BGRA


                AVFrame* marker_frame = utils::rescale_frame(frame, 640, 480, AV_PIX_FMT_YUV420P, buffer);
//                AVFrame* marker_frame = utils::rescale_frame(frame, 640, 480, AV_PIX_FMT_YUYV422, buffer);

//                AVFrame* marker_frame = utils::rescale_frame(frame, 640, 480, AV_PIX_FMT_BGRA, buffer);

                std::vector<float> mtx(16);
                mtx = tracker->find_tracked_transform_matrix(marker_frame, upd, visible);
                if (visible) {
                    AVFrame* marker_frame_draw = utils::rescale_frame(frame, 640, 480, AV_PIX_FMT_RGB24, buffer);
//                    AVFrame* dest_frame = av_frame_clone(marker_frame_draw);
                    AVFrame* dest_frame = av_frame_alloc();
                    // Determine required buffer size and allocate buffer
                    int numBytes = avpicture_get_size(static_cast<AVPixelFormat>(marker_frame_draw->format), marker_frame_draw->width, marker_frame_draw->height);
                    dest_buffer.resize(numBytes*sizeof(uint8_t));

                    //setup buffer for new frame
                    avpicture_fill((AVPicture *)dest_frame, dest_buffer.data(), static_cast<AVPixelFormat>(marker_frame_draw->format),
                                   marker_frame_draw->width, marker_frame_draw->height);
                    dest_frame->width = marker_frame_draw->width;
                    dest_frame->height = marker_frame_draw->height;
                    dest_frame->format = marker_frame_draw->format;
                    dest_frame->pts = frame->best_effort_timestamp;
//                    dest_frame->width = frame->width;
//                    dest_frame->height = frame->height;
                    dest_frame->pkt_dts = frame->pkt_dts;
                    dest_frame->pkt_pts = frame->pkt_pts;
                    dest_frame->pkt_duration = frame->pkt_duration;
                    dest_frame->best_effort_timestamp = frame->best_effort_timestamp;
                    dest_frame->pts = 0;
//                    std::cout << "[*****] 1 frame ptr: " << std::hex <<  dest_frame->data << " " << (int*)dest_frame->data[0]  << std::endl;
                    drawer->ask_to_draw(mtx, marker_frame_draw->data[0], std::addressof(dest_frame->data[0]));
                    std::cout << "[*****] 2 frame ptr: " << std::hex <<  dest_frame->data << " " << std::addressof(dest_frame->data[0]) << std::endl;
//                    photo::make_photo("dest_frame.png", 640, 480, dest_frame);
//                    photo::make_photo("marker_frame_draw.png", 640, 480, marker_frame_draw);
                    av_frame_free(&frame);
                    frame = av_frame_clone(dest_frame);//utils::rescale_frame(dest_frame, 640, 480, AV_PIX_FMT_YUV420P, buffer); //av_frame_clone(dest_frame);
                    frame->pts = dest_frame->best_effort_timestamp;
//                    frame->width = dest_frame->width;
//                    frame->height = dest_frame->height;
                    frame->pkt_dts = dest_frame->pkt_dts;
                    frame->pkt_pts = dest_frame->pkt_pts;
                    frame->pkt_duration = dest_frame->pkt_duration;
                    frame->best_effort_timestamp = dest_frame->best_effort_timestamp;
                    av_frame_free(&dest_frame);
                }

//                std::cout << "[*****] finded count: " << upd << " | [" << visible << "]" << std::endl;
//                std::cout << "[*****] frame: [" << marker_frame->width << "] format[" << marker_frame->height<< "]" << std::endl;
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
      width(-1), height(-1), bit_rate(-1), tracker(nullptr), drawer(nullptr)
{
    stop_flag.exchange(false);
    keeper = std::make_shared<frame_keeper>();
    input_time_base.den = set.time_base_den;
    input_time_base.num = set.time_base_num;
    drawer = std::make_shared<marker_drawer>();

    auto token = marker_tracker::preconstruction();
    tracker = new marker_tracker(std::move(token));
}

decoder::~decoder()
{
    stop_decoding();
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);
    av_frame_free(&frame);
    delete tracker;
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

    drawer->run_draw_thread(640, 480);
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

