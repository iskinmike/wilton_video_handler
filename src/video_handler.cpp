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

/* 
 * File:   example.cpp
 * Authorы: alex, mike
 *
 * Created on March 28, 2018, 16:32 PM
 */

#include <cstring>
#include <string>
#include <memory>
#include <map>

//#include "staticlib/json.hpp"
#include "wilton/support/exception.hpp"

#include "wilton/wiltoncall.h"
#include "video_api.hpp"
#include "frame_keeper.hpp"

#include "jansson.h"

namespace video_handler {

namespace { //anonymous
std::map<int,std::shared_ptr<video_api>> vhandlers_keeper;
int64_t get_integer_or_throw(const std::string& key, json_t* value) {
    if (json_is_integer(value)) {
        return json_integer_value(value);
    }
    throw wilton::support::exception(TRACEMSG("Error: Key [" + key+ "] don't contains integer value"));
}
std::string get_string_or_throw(const std::string& key, json_t* value) {
    if (json_is_string(value)) {
        return std::string{json_string_value(value)};
    }
    throw wilton::support::exception(TRACEMSG("Error: Key [" + key + "] don't contains string value"));
}

}
// handler functions
int av_init_handler(int id, const std::string& in, const std::string& out,
                const std::string& fmt, const std::string& title, const std::string& photo_name, const int& width,
                const int& height, const int& pos_x, const int& pos_y, const int& bit_rate, double framerate){
    video_settings set;
    set.input_file = in;
    set.output_file = out;
    set.format = fmt;
    set.title = title;
    set.width = width;
    set.height = height;
    set.pos_x = pos_x;
    set.pos_y = pos_y;
    set.bit_rate = bit_rate;
    set.photo_name = photo_name;
    set.framerate = framerate;

    if (vhandlers_keeper.count(id)){
        vhandlers_keeper.erase(id);
    }
    vhandlers_keeper[id] =
            std::shared_ptr<video_api> (new video_api(set));
    return id;
}
std::string av_delete_handler(int id){
    vhandlers_keeper.erase(id);
    return std::string{};
}
// video functions includes both encoder and decoder functions calls
std::string av_start_video_record(int id){
    return vhandlers_keeper[id]->start_video_record();
}
std::string av_stop_video_record(int id){
    vhandlers_keeper[id]->stop_video_record();
    return std::string{};
}
// encoder functions. Wait for decoded frames and writes them to video file
std::string av_start_encoding(int id){
    return vhandlers_keeper[id]->start_encoding();
}
std::string av_stop_encoding(int id){
    vhandlers_keeper[id]->stop_encoding();
    return std::string{};
}
// decoder functions. Occupies a video device and start to decode frames to memory
std::string av_start_decoding(int id){
    return vhandlers_keeper[id]->start_decoding();
}
std::string av_stop_decoding(int id){
    vhandlers_keeper[id]->stop_decoding();
    return std::string{};
}
// display functions. Display decoded frames to user
std::string av_start_video_display(int id){
    return vhandlers_keeper[id]->start_video_display();
}
std::string av_stop_video_display(int id){
    vhandlers_keeper[id]->stop_video_display();
    return std::string{};
}
// takes photo from decoded frame
std::string av_make_photo(int id){
    return vhandlers_keeper[id]->make_photo();
}
// check functions. true if encoder/decoder started
std::string av_is_encoder_started(int id){
    bool flag = false;
    if (vhandlers_keeper.count(id)) {
        flag = vhandlers_keeper[id]->get_encoder_flag();
    }
    return std::to_string(flag);
}
std::string av_is_decoder_started(int id){
    bool flag = false;
    if (vhandlers_keeper.count(id)) {
        flag = vhandlers_keeper[id]->get_decoder_flag();
    }
    return std::to_string(flag);
}
std::string av_is_display_started(int id){
    bool flag = false;
    if (vhandlers_keeper.count(id)) {
        flag = vhandlers_keeper[id]->get_display_flag();
    }
    return std::to_string(flag);
}
// return if video write is started, i.e. (av_is_decoder_started() && av_is_encoder_started())
std::string av_is_started(int id){
    bool flag = false;
    if (vhandlers_keeper.count(id)) {
        flag = vhandlers_keeper[id]->get_start_flag();
    }
    return std::to_string(flag);
}

char* vahandler_wrapper(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int)> (ctx);
        auto str_id = std::string(data_in, static_cast<size_t>(data_in_len));
        auto id = std::stoi(str_id);
        // chek if handler with id initialized
        auto output = std::string{};
        if (nullptr == vhandlers_keeper[id]) {
            output = "{ \"error\": \"Wrong id [" + str_id + "]\"}";
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

char* vahandler_wrapper_init(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<int(*)(int, const std::string&, const std::string&,
                                           const std::string&, const std::string&, const std::string&,
                                           const int&, const int&, const int&, const int&, const int&, double)> (ctx);

        json_t *root;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw wilton::support::exception(TRACEMSG("Error: " + std::string{error.text}));
        }

        int id = 0;
        auto in = std::string{};
        auto out = std::string{};
        auto fmt = std::string{};
        auto title = std::string{};
        auto photo_name = std::string{};
        int pos_x = -1;
        int pos_y = -1;
        int width = -1;
        int height = -1;
        int bit_rate = -1;
        const double error_value = -1.0;
        double framerate = error_value;

        /* obj is a JSON object */
        const char *key;
        json_t *value;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("id" == key_str) {
                id = get_integer_or_throw(key_str, value);
            } else if ("in" == key_str) {
                in = get_string_or_throw(key_str, value);
            } else if ("out" == key_str) {
                out = get_string_or_throw(key_str, value);
            } else if ("fmt" == key_str) {
                fmt = get_string_or_throw(key_str, value);
            } else if ("title" == key_str) {
                title = get_string_or_throw(key_str, value);
            } else if ("width" == key_str) {
                width = get_integer_or_throw(key_str, value);
            } else if ("height" == key_str) {
                height = get_integer_or_throw(key_str, value);
            } else if ("pos_x" == key_str) {
                pos_x = get_integer_or_throw(key_str, value);
            } else if ("pos_y" == key_str) {
                pos_y = get_integer_or_throw(key_str, value);
            } else if ("bit_rate" == key_str) {
                bit_rate = get_integer_or_throw(key_str, value);
            } else if ("photo_name" == key_str) {
                photo_name = get_string_or_throw(key_str, value);
            } else if ("framerate" == key_str) {
                if (json_is_real(value)) {
                    framerate = json_real_value(value);
                } else {
                    framerate = get_integer_or_throw(key_str, value);
                }
            } else {
                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + key + "]"));
            }
        }

//        for (const sl::json::field& fi : json.as_object()) {
//            auto& name = fi.name();
//            if ("id" == name) {
//                id = fi.as_int64_or_throw(name);
//            } else if ("in" == name) {
//                in = fi.as_string_nonempty_or_throw(name);
//            } else if ("out" == name) {
//                out = fi.as_string_nonempty_or_throw(name);
//            } else if ("fmt" == name) {
//                fmt = fi.as_string_nonempty_or_throw(name);
//            } else if ("title" == name) {
//                title = fi.as_string_nonempty_or_throw(name);
//            } else if ("width" == name) {
//                width = fi.as_int64_or_throw(name);
//            } else if ("height" == name) {
//                height = fi.as_int64_or_throw(name);
//            } else if ("pos_x" == name) {
//                pos_x = fi.as_int64_or_throw(name);
//            } else if ("pos_y" == name) {
//                pos_y = fi.as_int64_or_throw(name);
//            } else if ("bit_rate" == name) {
//                bit_rate = fi.as_int64_or_throw(name);
//            } else if ("photo_name" == name) {
//                photo_name = fi.as_string_nonempty_or_throw(name);
//            } else if ("framerate" == name) {
//                framerate = fi.as_double(error_value);
//                if (error_value == framerate) {
//                    framerate = fi.as_int64_or_throw(name);
//                }
//            } else {
//                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
//            }
//        }

        // check not optional json data
        if (in.empty())  throw wilton::support::exception(TRACEMSG(
                "Required parameter 'in' not specified"));
        if (out.empty())  throw wilton::support::exception(TRACEMSG(
                "Required parameter 'out' not specified"));
        if (fmt.empty())  throw wilton::support::exception(TRACEMSG(
                "Required parameter 'fmt' not specified"));
        if (title.empty())  throw wilton::support::exception(TRACEMSG(
                "Required parameter 'title' not specified"));
        if (photo_name.empty())  throw wilton::support::exception(TRACEMSG(
                "Required parameter 'photo_name' not specified"));

        std::string output = std::to_string(fun(id, in, out, fmt, title, photo_name,
                width, height, pos_x, pos_y, bit_rate, framerate));
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
        auto what = std::string("CALL ERROR");
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

} // namespace

// this function is called on module load,
// must return NULL on success
extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
char* wilton_module_init() {
    char* err = nullptr;

    // Call to initialize frame_keeper in single thread
    frame_keeper::instance();

    // register 'av_start_video_record' function
    auto name_av_start_video_record = std::string("av_start_video_record");
    err = wiltoncall_register(name_av_start_video_record.c_str(), static_cast<int> (name_av_start_video_record.length()),
            reinterpret_cast<void*> (video_handler::av_start_video_record), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_stop_video_record' function
    auto name_av_stop_video_record = std::string("av_stop_video_record");
    err = wiltoncall_register(name_av_stop_video_record.c_str(), static_cast<int> (name_av_stop_video_record.length()),
            reinterpret_cast<void*> (video_handler::av_stop_video_record), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'av_start_encoding' function
    auto name_av_start_encoding = std::string("av_start_encoding");
    err = wiltoncall_register(name_av_start_encoding.c_str(), static_cast<int> (name_av_start_encoding.length()),
            reinterpret_cast<void*> (video_handler::av_start_encoding), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_stop_encoding' function
    auto name_av_stop_encoding = std::string("av_stop_encoding");
    err = wiltoncall_register(name_av_stop_encoding.c_str(), static_cast<int> (name_av_stop_encoding.length()),
            reinterpret_cast<void*> (video_handler::av_stop_encoding), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;


    // register 'av_start_decoding' function
    auto name_av_start_decoding = std::string("av_start_decoding");
    err = wiltoncall_register(name_av_start_decoding.c_str(), static_cast<int> (name_av_start_decoding.length()),
            reinterpret_cast<void*> (video_handler::av_start_decoding), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_stop_decoding' function
    auto name_av_stop_decoding = std::string("av_stop_decoding");
    err = wiltoncall_register(name_av_stop_decoding.c_str(), static_cast<int> (name_av_stop_decoding.length()),
            reinterpret_cast<void*> (video_handler::av_stop_decoding), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;


    // register 'av_stop_video_display' function
    auto name_av_stop_video_display = std::string("av_stop_video_display");
    err = wiltoncall_register(name_av_stop_video_display.c_str(), static_cast<int> (name_av_stop_video_display.length()),
            reinterpret_cast<void*> (video_handler::av_stop_video_display), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_start_video_display' function
    auto name_av_start_video_display = std::string("av_start_video_display");
    err = wiltoncall_register(name_av_start_video_display.c_str(), static_cast<int> (name_av_start_video_display.length()),
            reinterpret_cast<void*> (video_handler::av_start_video_display), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'av_make_photo' function
    auto name_av_make_photo = std::string("av_make_photo");
    err = wiltoncall_register(name_av_make_photo.c_str(), static_cast<int> (name_av_make_photo.length()),
            reinterpret_cast<void*> (video_handler::av_make_photo), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'av_delete_handler' function
    auto name_av_delete_handler = std::string("av_delete_handler");
    err = wiltoncall_register(name_av_delete_handler.c_str(), static_cast<int> (name_av_delete_handler.length()),
            reinterpret_cast<void*> (video_handler::av_delete_handler), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'av_is_started' function
    auto name_av_is_started = std::string("av_is_started");
    err = wiltoncall_register(name_av_is_started.c_str(), static_cast<int> (name_av_is_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_started), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_is_decoder_started' function
    auto name_av_is_decoder_started = std::string("av_is_decoder_started");
    err = wiltoncall_register(name_av_is_decoder_started.c_str(), static_cast<int> (name_av_is_decoder_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_decoder_started), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_is_encoder_started' function
    auto name_av_is_encoder_started = std::string("av_is_encoder_started");
    err = wiltoncall_register(name_av_is_encoder_started.c_str(), static_cast<int> (name_av_is_encoder_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_encoder_started), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'av_is_display_started' function
    auto name_av_is_display_started = std::string("av_is_display_started");
    err = wiltoncall_register(name_av_is_display_started.c_str(), static_cast<int> (name_av_is_display_started.length()),
            reinterpret_cast<void*> (video_handler::av_is_display_started), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'av_inti_handler' function
    auto name_av_inti_handler = std::string("av_inti_handler");
    err = wiltoncall_register(name_av_inti_handler.c_str(), static_cast<int> (name_av_inti_handler.length()),
            reinterpret_cast<void*> (video_handler::av_init_handler), video_handler::vahandler_wrapper_init);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
