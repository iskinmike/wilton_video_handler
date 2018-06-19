
#include "video_api.hpp"

bool video_api::get_start_flag() const
{
    return start_flag;
}

bool video_api::get_decoder_flag() const
{
    return decoder_start_flag;
}

bool video_api::get_encoder_flag() const
{
    return encoder_start_flag;
}

bool video_api::get_display_flag() const
{
    return display_start_flag;
}

video_api::video_api(video_settings set) : start_flag(false),
    encoder_start_flag(false), decoder_start_flag(false), display_start_flag(false)
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

    result = start_decoding();
    if (!result.empty()) {
        return result;
    }

    result = start_encoding();
    if (!result.empty()) {
        return result;
    }

    start_flag = encoder_start_flag && decoder_start_flag;
    return result;
}

std::string video_api::start_video_display()
{
    auto result = std::string{};
    if (decoder_start_flag) {
        int width = (-1 != settings.display_width) ? settings.display_width : api_decoder->get_width();
        int height = (-1 != settings.display_height) ? settings.display_height : api_decoder->get_height();
        result = api_display->start_display(settings.pos_x, settings.pos_y, width, height);
        if (result.empty()) display_start_flag = true;
    }
    return result;
}

std::string video_api::init_encoder() {
    if (!api_decoder->is_initialized()){
        return std::string{};
    }
    return api_encoder->init(api_decoder->get_bit_rate(), api_decoder->get_width(), api_decoder->get_height(), settings.framerate);
}

std::string video_api::init_decoder(){
    return api_decoder->init();
}

std::string video_api::start_encoding(){
    auto result = std::string{};
    if (!api_encoder->is_initialized()) {
        result = init_encoder();
        if (!result.empty()) {
            return result;
        }
    }
    if (!encoder_start_flag) {
        api_encoder->start_encoding();
        encoder_start_flag = true;
    }
    return result;
}

std::string video_api::start_decoding(){
    auto result = std::string{};
    if (!api_decoder->is_initialized()){
        result = init_decoder();
        if (!result.empty()) {
            return result;
        }
    }
    if (!decoder_start_flag) {
        api_decoder->start_decoding();
        decoder_start_flag = true;
    }
    return result;
}

void video_api::stop_decoding(){
    api_decoder->stop_decoding();
    decoder_start_flag = false;
}

void video_api::stop_encoding(){
    api_encoder->stop_encoding();
    encoder_start_flag = false;
}

void video_api::stop_video_record()
{
    stop_encoding();
    stop_decoding();
    start_flag = encoder_start_flag && decoder_start_flag;
}

void video_api::stop_video_display()
{
    if (display_start_flag) {
        api_display->stop_display();
        display_start_flag = false;
    }
}

std::string video_api::make_photo()
{
    return photo::make_photo(settings.photo_name);
}
