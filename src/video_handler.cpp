/*
 * Copyright 2018, alex at staticlibs.net
 * Copyright 2018, myasnikov.mike at gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include <string>
#include <memory>
#include <map>
#include <type_traits>

#include "wilton/wiltoncall.h"
#include "decoder.hpp"
#include "encoder.hpp"
#include "display.hpp"
#include "photo.hpp"
#include "frame_keeper.hpp"
#include "video_logger.hpp"

#include "jansson.h"

#ifndef VERSION_STR
#define VERSION_STR "VIDEO_HANDLER. Ver 1.02. Added decoding settings"
#endif

namespace video_handler {

namespace { //anonymous
std::map<int,std::shared_ptr<encoder>> encoders;
std::map<int,std::shared_ptr<decoder>> decoders;
std::map<int,std::shared_ptr<display>> displays;
//video_logger logger;
int get_integer_or_throw(const std::string& key, json_t* value) {
    if (json_is_integer(value)) {
        return static_cast<int>(json_integer_value(value));
    }
    throw std::invalid_argument(std::string{"Error: Key [" + key+ "] don't contains integer value"});
}
std::string get_string_or_throw(const std::string& key, json_t* value) {
    if (json_is_string(value)) {
        return std::string{json_string_value(value)};
    }
    throw std::invalid_argument(std::string{"Error: Key [" + key+ "] don't contains string value"});
}
} // anonymous namespace

// handler functions
int av_init_encoder(int id, encoder_settings set){
    if (encoders.count(id)){
        encoders.erase(id);
    }
    encoders[id] = std::make_shared<encoder> (set);
    return id;
}
std::string av_delete_encoder(int id){
    encoders.erase(id);
    return std::string{};
}
std::string av_init_decoder(int id, decoder_settings set){
    if (decoders.count(id)){
        decoders.erase(id);
    }
    decoders[id] = std::make_shared<decoder> (set);
    std::string res = decoders[id]->init();
    if (res.empty()) {
        res = std::to_string(id);
    }
    return res;
}
std::string av_delete_decoder(int id){
    decoders.erase(id);
    return std::string{};
}
int av_init_display(int id, display_settings set){
    if (displays.count(id)){
        displays.erase(id);
    }
    displays[id] = std::make_shared<display> (set);
    return id;
}
std::string av_setup_decoder_to_display(int display_id, int decoder_id){
    displays[display_id]->setup_frame_keeper(decoders[decoder_id]->get_keeper());
    return std::string{};
}
std::string av_setup_decoder_to_encoder(int encoder_id, int decoder_id){
    encoders[encoder_id]->setup_frame_keeper(decoders[decoder_id]->get_keeper());
    return std::string{};
}
std::string av_delete_display(int id){
    displays.erase(id);
    return std::string{};
}
// encoder functions. Wait for decoded frames and writes them to video file
std::string av_start_encoding(int id){
    std::string res = encoders[id]->init();
    if (res.empty()) {
        res = encoders[id]->start_encoding();
    }
    return res;
}
std::string av_stop_encoding(int id){
    encoders[id]->stop_encoding();
    video_logger& logger = video_logger::instance();
    logger.copy_logfile_to(encoders[id]->get_out_file());
    logger.drop_callback_function();
    return std::string{};
}
// decoder functions. Occupies a video device and start to decode frames to memory
std::string av_start_decoding(int id){
    auto res= std::string{};
    if (res.empty()) {
        decoders[id]->start_decoding();
    }
    return res;
}
std::string av_stop_decoding(int id){
    decoders[id]->stop_decoding();
    return std::string{};
}
// display functions. Display decoded frames to user
std::string av_start_video_display(int id){
    return displays[id]->start_display();
}
std::string av_stop_video_display(int id){
    displays[id]->stop_display();
    return std::string{};
}
// takes photo from decoded frame
std::string av_make_photo(int id, const std::string& out, int width,  int height){
    return photo::make_photo(out,width,height, decoders[id]->get_keeper());
}
// check functions. true if encoder/decoder/display started
std::string av_is_encoder_started(int id){
    bool flag = false;
    if (encoders.count(id)) {
        flag = encoders[id]->is_initialized();
    }
    return std::to_string(flag);
}
std::string av_is_decoder_started(int id){
    bool flag = false;
    if (decoders.count(id)) {
        flag = decoders[id]->is_initialized();
    }
    return std::to_string(flag);
}
std::string av_is_display_started(int id){
    bool flag = false;
    if (displays.count(id)) {
        flag = displays[id]->is_initialized();
    }
    return std::to_string(flag);
}

std::string av_get_version(){
    return VERSION_STR;
}

template <typename T>
char* vahandler_wrapper_for(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int)> (ctx);
        auto str_id = std::string(data_in, static_cast<size_t>(data_in_len));

        int id = 0;
        try {
            id = std::stoi(str_id);
        } catch (...) {
            throw std::invalid_argument("Wrong data format used. Excpected integer value, sended '" + str_id + "'");
        }

        // chek if handler with id initialized
        auto output = std::string{};
        auto name = std::string{};

        //
        bool existance_check = false;
        if (std::is_same<T, display>::value){
            existance_check = (nullptr == displays[id]);
            name = "display";
        } else if (std::is_same<T, encoder>::value){
            existance_check = (nullptr == encoders[id]);
            name = "encoder";
        } else if (std::is_same<T, decoder>::value) {
            existance_check = (nullptr == decoders[id]);
            name = "decoder";
        } else {
            throw std::invalid_argument("Wrong use of function template [vahandler_wrapper_for]");
        }


        if (existance_check) {
            output = "{ \"error\": \"Wrong " + name + " id [" + str_id + "]\"}";
        } else {
            output = fun(id);
        }
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

template <typename T>
char* vahandler_wrapper_is_started(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int)> (ctx);
        auto str_id = std::string(data_in, static_cast<size_t>(data_in_len));
        int id = 0;
        try {
            id = std::stoi(str_id);
        } catch (...) {
            throw std::invalid_argument("Wrong data format used. Excpected integer value, sended '" + str_id + "'");
        }
        // chek if handler with id initialized
        auto output = std::string{};

        bool existance_check = false;
        if (std::is_same<T, display>::value){
            existance_check = (nullptr == displays[id]);
        } else if (std::is_same<T, encoder>::value){
            existance_check = (nullptr == encoders[id]);
        } else if (std::is_same<T, decoder>::value) {
            existance_check = (nullptr == decoders[id]);
        } else {
            throw std::invalid_argument("Wrong use of function template [vahandler_wrapper_is_started]");
        }

        if (existance_check) {
            output = "0";
        } else {
            output = fun(id);
        }
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

template <typename T>
char* vahandler_wrapper_setup_decoder_to(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        std::string key_name{};
        std::string destination_name{};

        if (std::is_same<T, display>::value){
            key_name = "display_id";
            destination_name = "Display";
        } else if (std::is_same<T, encoder>::value){
            key_name = "encoder_id";
            destination_name = "Encoder";
        } else {
            throw std::invalid_argument("Wrong use of function template [vahandler_wrapper_setup_decoder_to]");
        }

        auto fun = reinterpret_cast<std::string(*)(int, int)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }


        int something_id = 0;
        int decoder_id = 0;

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if (key_name == key_str) {
                something_id = get_integer_or_throw(key_str, value);
            } else if ("decoder_id" == key_str) {
                decoder_id = get_integer_or_throw(key_str, value);
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        // chek if handler with id initialized
        bool existance_check = false;
        if (std::is_same<T, display>::value){
            existance_check = (nullptr == displays[something_id]);
        } else if (std::is_same<T, encoder>::value){
            existance_check = (nullptr == encoders[something_id]);
        }

        auto output = std::string{};
        if (nullptr == decoders[decoder_id]) {
            output = "{ \"error\": \"Decoder with id [" + std::to_string(decoder_id) + "] not exists\"}";
        } else if (existance_check) {
            output = "{ \"error\": \"" + destination_name + "with id [" + std::to_string(something_id) + "] not exists\"}";
        } else {
            output = fun(something_id, decoder_id);
        }
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

char* vahandler_wrapper_init_encoder(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<int(*)(int, encoder_settings)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int id = 0;
        auto out = std::string{};
        int video_width = not_set;
        int video_height = not_set;
        int bit_rate = not_set;
        const double error_value = -1.0;
        double framerate = error_value;

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("id" == key_str) {
                id = get_integer_or_throw(key_str, value);
            } else if ("out" == key_str) {
                out = get_string_or_throw(key_str, value);
            } else if ("width" == key_str) {
                video_width = get_integer_or_throw(key_str, value);
            } else if ("height" == key_str) {
                video_height = get_integer_or_throw(key_str, value);
            } else if ("bit_rate" == key_str) {
                bit_rate = get_integer_or_throw(key_str, value);
            } else if ("framerate" == key_str) {
                if (json_is_real(value)) {
                    framerate = json_real_value(value);
                } else {
                    framerate = get_integer_or_throw(key_str, value);
                }
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        // check not optional json data
        if (out.empty())  throw std::invalid_argument(
                "Required parameter 'out' not specified");

        encoder_settings settings;
        settings.output_file = out;
        settings.width = video_width;
        settings.height = video_height;
        settings.bit_rate = bit_rate;
        settings.framerate = framerate;

        std::string output = std::to_string(fun(id, settings));
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        json_decref(root);
        return nullptr;

    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR");
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}
char* vahandler_wrapper_init_decoder(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int, decoder_settings)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int id = 0;
        auto in = std::string{};
        auto fmt = std::string{};
        auto framerate = std::string{};
        auto videoformat = std::string{};
        auto size = std::string{};
        int time_base_den = not_set;
        int time_base_num = not_set;

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        // logger
        auto logger_tmp_file = std::string{};

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("id" == key_str) {
                id = get_integer_or_throw(key_str, value);
            } else if ("in" == key_str) {
                in = get_string_or_throw(key_str, value);
            } else if ("fmt" == key_str) {
                fmt = get_string_or_throw(key_str, value);
            } else if ("framerate" == key_str) {
                framerate = get_string_or_throw(key_str, value);
            } else if ("videoformat" == key_str) {
                videoformat = get_string_or_throw(key_str, value);
            } else if ("size" == key_str) {
                size = get_string_or_throw(key_str, value);
            } else if ("time_base_den" == key_str) {
                time_base_den = get_integer_or_throw(key_str, value);
            } else if ("time_base_num" == key_str) {
                time_base_num = get_integer_or_throw(key_str, value);
            } else if ("loggerSettings" == key_str) {
                if (json_is_object(value)) {
                    json_t *subobject = nullptr;
                    const char *subobject_key = nullptr;
                    json_object_foreach(value, subobject_key, subobject){
                        auto subobject_key_str = std::string{subobject_key};
                        if ("path" == subobject_key_str) {
                            logger_tmp_file = get_string_or_throw(subobject_key_str, subobject);
                        } else {
                            std::string err_msg = std::string{"Unknown data field: [loggerSettings]["} + subobject_key_str + "]";
                            throw std::invalid_argument(err_msg);
                        }
                    }
                }
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        // check not optional json data
        if (in.empty())  throw std::invalid_argument(
                "Required parameter 'in' not specified");
        if (fmt.empty())  throw std::invalid_argument(
                "Required parameter 'fmt' not specified");

        decoder_settings settings;
        settings.input_file = in;
        settings.format = fmt;
        settings.framerate = framerate;
        settings.videoformat = videoformat;
        settings.size = size;
        settings.time_base_den = time_base_den;
        settings.time_base_num = time_base_num;

        if (!logger_tmp_file.empty()) {
            video_logger& logger = video_logger::instance();
            if (!logger.is_started()) {
                logger.setup_tmp_file(logger_tmp_file);
                logger.setup_callback_function();
                av_log(nullptr, AV_LOG_DEBUG, "Decoder init. [%s]\n", av_get_version().c_str());
            }
        }

        std::string output = fun(id, settings);
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        json_decref(root);
        return nullptr;
    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR");
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}
char* vahandler_wrapper_init_display(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<int(*)(int, display_settings)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int id = 0;
        auto title = std::string{"CAM"};
        auto parent_title = std::string{"Firefox"};
        int pos_x = not_set;
        int pos_y = not_set;
        int display_width = not_set;
        int display_height = not_set;


        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("id" == key_str) {
                id = get_integer_or_throw(key_str, value);
            } else if ("title" == key_str) {
                title = get_string_or_throw(key_str, value);
            } else if ("parent_title" == key_str) {
                parent_title = get_string_or_throw(key_str, value);
            } else if ("width" == key_str) {
                display_width = get_integer_or_throw(key_str, value);
            } else if ("height" == key_str) {
                display_height = get_integer_or_throw(key_str, value);
            } else if ("pos_x" == key_str) {
                pos_x = get_integer_or_throw(key_str, value);
            } else if ("pos_y" == key_str) {
                pos_y = get_integer_or_throw(key_str, value);
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        // check not optional json data
        if (title.empty())  throw std::invalid_argument(
                "Required parameter 'title' not specified");

        display_settings settings;
        settings.title = title;
        settings.parent_title = parent_title;
        settings.width = display_width;
        settings.height = display_height;
        settings.pos_x = pos_x;
        settings.pos_y = pos_y;


        std::string output = std::to_string(fun(id, settings));
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        json_decref(root);
        return nullptr;

    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR");
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

char* vahandler_wrapper_make_photo(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int, const std::string, int, int)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int id = 0;
        auto photo_name = std::string{};
        int video_width = not_set;
        int video_height = not_set;

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("id" == key_str) {
                id = get_integer_or_throw(key_str, value);
            } else if ("photo_name" == key_str) {
                photo_name = get_string_or_throw(key_str, value);
            } else if ("width" == key_str) {
                video_width = get_integer_or_throw(key_str, value);
            } else if ("height" == key_str) {
                video_height = get_integer_or_throw(key_str, value);
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        // check not optional json data
        if (photo_name.empty())  throw std::invalid_argument(
                "Required parameter 'photo_name' not specified");

        // chek if handler with id initialized
        auto output = std::string{};
        if (nullptr == decoders[id]) {
            output = "{ \"error\": \"Ddecoder with id [" + std::to_string(id) + "] not exists\"}";
        } else {
            output = fun(id, photo_name, video_width, video_height);
        }
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (const std::exception& e) {
        auto what = std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}


char* vahandler_wrapper_get_version(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    auto fun = reinterpret_cast<std::string(*)()> (ctx);
    auto output = fun();
    *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
    std::memcpy(*data_out, output.c_str(), output.length() + 1);
    *data_out_len = static_cast<int>(output.length());
    return nullptr;
}

} // namespace video_handler

// this function is called on module load,
// must return nullptr on success
extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
char* wilton_module_init() {
    char* err = nullptr;

    av_log_set_level(AV_LOG_QUIET);

    // register 'av_get_version' function
    auto name_av_get_version = std::string("av_get_version");
    err = wiltoncall_register(name_av_get_version.c_str(), static_cast<int> (name_av_get_version.length()),
            reinterpret_cast<void*> (video_handler::av_get_version), video_handler::vahandler_wrapper_get_version);
    if (nullptr != err) return err;

    // register 'av_start_encoding' function
    auto name_av_start_encoding = std::string("av_start_encoding");
    err = wiltoncall_register(name_av_start_encoding.c_str(), static_cast<int> (name_av_start_encoding.length()),
            reinterpret_cast<void*> (video_handler::av_start_encoding), video_handler::vahandler_wrapper_for<encoder>);
    if (nullptr != err) return err;
    // register 'av_stop_encoding' function
    auto name_av_stop_encoding = std::string("av_stop_encoding");
    err = wiltoncall_register(name_av_stop_encoding.c_str(), static_cast<int> (name_av_stop_encoding.length()),
            reinterpret_cast<void*> (video_handler::av_stop_encoding), video_handler::vahandler_wrapper_for<encoder>);
    if (nullptr != err) return err;


    // register 'av_start_decoding' function
    auto name_av_start_decoding = std::string("av_start_decoding");
    err = wiltoncall_register(name_av_start_decoding.c_str(), static_cast<int> (name_av_start_decoding.length()),
            reinterpret_cast<void*> (video_handler::av_start_decoding), video_handler::vahandler_wrapper_for<decoder>);
    if (nullptr != err) return err;
    // register 'av_stop_decoding' function
    auto name_av_stop_decoding = std::string("av_stop_decoding");
    err = wiltoncall_register(name_av_stop_decoding.c_str(), static_cast<int> (name_av_stop_decoding.length()),
            reinterpret_cast<void*> (video_handler::av_stop_decoding), video_handler::vahandler_wrapper_for<decoder>);
    if (nullptr != err) return err;

    // register 'av_stop_video_display' function
    auto name_av_stop_video_display = std::string("av_stop_video_display");
    err = wiltoncall_register(name_av_stop_video_display.c_str(), static_cast<int> (name_av_stop_video_display.length()),
            reinterpret_cast<void*> (video_handler::av_stop_video_display), video_handler::vahandler_wrapper_for<display>);
    if (nullptr != err) return err;
    // register 'av_start_video_display' function
    auto name_av_start_video_display = std::string("av_start_video_display");
    err = wiltoncall_register(name_av_start_video_display.c_str(), static_cast<int> (name_av_start_video_display.length()),
            reinterpret_cast<void*> (video_handler::av_start_video_display), video_handler::vahandler_wrapper_for<display>);
    if (nullptr != err) return err;

    // register 'av_make_photo' function
    auto name_av_make_photo = std::string("av_make_photo");
    err = wiltoncall_register(name_av_make_photo.c_str(), static_cast<int> (name_av_make_photo.length()),
            reinterpret_cast<void*> (video_handler::av_make_photo), video_handler::vahandler_wrapper_make_photo);
    if (nullptr != err) return err;

    // register 'av_is_decoder_started' function
    auto name_av_is_decoder_started = std::string("av_is_decoder_started");
    err = wiltoncall_register(name_av_is_decoder_started.c_str(), static_cast<int> (name_av_is_decoder_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_decoder_started), video_handler::vahandler_wrapper_is_started<decoder>);
    if (nullptr != err) return err;
    // register 'av_is_encoder_started' function
    auto name_av_is_encoder_started = std::string("av_is_encoder_started");
    err = wiltoncall_register(name_av_is_encoder_started.c_str(), static_cast<int> (name_av_is_encoder_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_encoder_started), video_handler::vahandler_wrapper_is_started<encoder>);
    if (nullptr != err) return err;
    // register 'av_is_display_started' function
    auto name_av_is_display_started = std::string("av_is_display_started");
    err = wiltoncall_register(name_av_is_display_started.c_str(), static_cast<int> (name_av_is_display_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_display_started), video_handler::vahandler_wrapper_is_started<display>);
    if (nullptr != err) return err;
//////////////////////////
    // register 'av_init_decoder' function
    auto name_av_init_decoder = std::string("av_init_decoder");
    err = wiltoncall_register(name_av_init_decoder.c_str(), static_cast<int> (name_av_init_decoder.length()),
            reinterpret_cast<void*> (video_handler::av_init_decoder), video_handler::vahandler_wrapper_init_decoder);
    if (nullptr != err) return err;
    // register 'av_init_encoder' function
    auto name_av_init_encoder = std::string("av_init_encoder");
    err = wiltoncall_register(name_av_init_encoder.c_str(), static_cast<int> (name_av_init_encoder.length()),
            reinterpret_cast<void*> (video_handler::av_init_encoder), video_handler::vahandler_wrapper_init_encoder);
    if (nullptr != err) return err;
    // register 'av_init_display' function
    auto name_av_init_display = std::string("av_init_display");
    err = wiltoncall_register(name_av_init_display.c_str(), static_cast<int> (name_av_init_display.length()),
            reinterpret_cast<void*> (video_handler::av_init_display), video_handler::vahandler_wrapper_init_display);
    if (nullptr != err) return err;
///////////////////////////
    // register 'av_delete_decoder' function
    auto name_av_delete_decoder = std::string("av_delete_decoder");
    err = wiltoncall_register(name_av_delete_decoder.c_str(), static_cast<int> (name_av_delete_decoder.length()),
            reinterpret_cast<void*> (video_handler::av_delete_decoder), video_handler::vahandler_wrapper_for<decoder>);
    if (nullptr != err) return err;

    // register 'av_delete_encoder' function
    auto name_av_delete_encoder = std::string("av_delete_encoder");
    err = wiltoncall_register(name_av_delete_encoder.c_str(), static_cast<int> (name_av_delete_encoder.length()),
            reinterpret_cast<void*> (video_handler::av_delete_encoder), video_handler::vahandler_wrapper_for<encoder>);
    if (nullptr != err) return err;

    // register 'av_delete_display' function
    auto name_av_delete_display = std::string("av_delete_display");
    err = wiltoncall_register(name_av_delete_display.c_str(), static_cast<int> (name_av_delete_display.length()),
            reinterpret_cast<void*> (video_handler::av_delete_display), video_handler::vahandler_wrapper_for<display>);
    if (nullptr != err) return err;
///////////////////////////
    auto name_av_setup_decoder_to_display = std::string("av_setup_decoder_to_display");
    err = wiltoncall_register(name_av_setup_decoder_to_display.c_str(), static_cast<int> (name_av_setup_decoder_to_display.length()),
            reinterpret_cast<void*> (video_handler::av_setup_decoder_to_display), video_handler::vahandler_wrapper_setup_decoder_to<display>);
    if (nullptr != err) return err;

    auto name_av_setup_decoder_to_encoder = std::string("av_setup_decoder_to_encoder");
    err = wiltoncall_register(name_av_setup_decoder_to_encoder.c_str(), static_cast<int> (name_av_setup_decoder_to_encoder.length()),
            reinterpret_cast<void*> (video_handler::av_setup_decoder_to_encoder), video_handler::vahandler_wrapper_setup_decoder_to<encoder>);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
