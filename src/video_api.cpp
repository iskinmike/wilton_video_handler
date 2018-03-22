
#include "video_api.h"

VideoAPI::VideoAPI(VideoSettings set)
{
    settings = set;
    decoder = std::shared_ptr<Decoder> (new Decoder(set.input_file, set.format,
            set.width, set.height, set.bit_rate));
    encoder = std::shared_ptr<Encoder> (new Encoder(set.output_file));
    display = std::shared_ptr<Display> (new Display(set.title));
}

int VideoAPI::startVideoRecord()
{
    int result = 0;
    result = decoder->init();
    if (0 != result) {
        return result;
    }
    result = encoder->init(decoder->getBitRate(), decoder->getWidth(), decoder->getHeight());
    if (0 != result) {
        return result;
    }

    decoder->startDecoding();
    encoder->startEncoding();
    return result;
}

int VideoAPI::startVideoDisplay()
{
    int result = 0;
    result = display->init((-1 == settings.pos_x) ? 100 : settings.pos_x,
            (-1 == settings.pos_y) ? 100 : settings.pos_y,
            decoder->getWidth(), decoder->getHeight());
    display->startDisplay();
    return result;
}

void VideoAPI::stopVideoRecord()
{
    encoder->stopEncoding();
    decoder->stopDecoding();
}

void VideoAPI::stopVideoDisplay()
{
    display->stopDisplay();
}

void VideoAPI::makePhoto()
{
    photo::makePhoto(settings.photo_name);
}
