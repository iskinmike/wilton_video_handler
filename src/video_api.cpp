
#include "video_api.hpp"

video_api::video_api(video_settings set)
{
    settings = set;
    api_decoder = std::shared_ptr<decoder> (new decoder(set.input_file, set.format,
            set.width, set.height, set.bit_rate));
    api_encoder = std::shared_ptr<encoder> (new encoder(set.output_file));
    api_display = std::shared_ptr<display> (new display(set.title));
}

std::string video_api::start_video_record()
{
    auto result = std::string{};
    result = api_decoder->init();
    if (!result.empty()) {
        return result;
    }
    result = api_encoder->init(api_decoder->get_bit_rate(), api_decoder->get_width(), api_decoder->get_height());
    if (!result.empty()) {
        return result;
    }

    api_decoder->start_decoding();
    api_encoder->start_encoding();
    return result;
}

std::string video_api::start_video_display()
{
    return api_display->start_display(settings.pos_x, settings.pos_y,
            api_decoder->get_width(), api_decoder->get_height());
}

void video_api::stop_video_record()
{
    api_encoder->stop_encoding();
    api_decoder->stop_decoding();
}

void video_api::stop_video_display()
{
    api_display->stop_display();
}

std::string video_api::make_photo()
{
    return photo::make_photo(settings.photo_name);
}
