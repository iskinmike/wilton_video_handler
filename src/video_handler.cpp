/*
 * Copyright 2018, alex at staticlibs.net
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
 * Author: alex
 *
 * Created on February 12, 2018, 9:22 PM
 */

#include <cstring>
#include <string>
#include <memory>
#include <map>

#include "staticlib/json.hpp"
#include "wilton/support/exception.hpp"

#include "wilton/wiltoncall.h"
#include "video_api.h"

namespace video_handler {

namespace { //anonymous
std::map<int,std::shared_ptr<VideoAPI>> vhandlers_keeper;
}

// Добавим функции для работы
int intiHandler(int id, const std::string& in, const std::string& out,
                const std::string& fmt, const std::string& title, const std::string& photo_name, const int& width,
                const int& height, const int& pos_x, const int& pos_y, const int& bit_rate){
    VideoSettings set;
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

    vhandlers_keeper[id] =
            std::shared_ptr<VideoAPI> (new VideoAPI(set));
    return id;
}

// TODO: Добавить валидаци параметров, id например
std::string startVideoRecord(int id){
    int res = vhandlers_keeper[id]->startVideoRecord();
    return std::to_string(res);
}

std::string displayVideo(int id){
    int res = vhandlers_keeper[id]->startVideoDisplay();
    return std::to_string(res);
}

std::string stopVideoRecord(int id){
    vhandlers_keeper[id]->stopVideoRecord();
    return std::string{};
}

std::string stopDisplayVideo(int id){
    vhandlers_keeper[id]->stopVideoDisplay();
    return std::string{};
}

std::string makePhoto(int id){
    vhandlers_keeper[id]->makePhoto();
    return std::string{};
}

char* vahandler_wrapper(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int)> (ctx);
        auto input = std::stoi(std::string(data_in, static_cast<size_t>(data_in_len)));
        std::string output = fun(input);
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
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
                                           const int&, const int&, const int&, const int&, const int& )> (ctx);

        sl::json::value json = sl::json::loads(std::string(data_in, data_in_len));
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
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("id" == name) {
                id = fi.as_int64_or_throw(name);
            } else if ("in" == name) {
                in = fi.as_string_nonempty_or_throw(name);
            } else if ("out" == name) {
                out = fi.as_string_nonempty_or_throw(name);
            } else if ("fmt" == name) {
                fmt = fi.as_string_nonempty_or_throw(name);
            } else if ("title" == name) {
                title = fi.as_string_nonempty_or_throw(name);
            } else if ("width" == name) {
                width = fi.as_int64_or_throw(name);
            } else if ("height" == name) {
                height = fi.as_int64_or_throw(name);
            } else if ("pos_x" == name) {
                pos_x = fi.as_int64_or_throw(name);
            } else if ("pos_y" == name) {
                pos_y = fi.as_int64_or_throw(name);
            } else if ("bit_rate" == name) {
                bit_rate = fi.as_int64_or_throw(name);
            } else if ("photo_name" == name) {
                photo_name = fi.as_string_nonempty_or_throw(name);
            } else {
                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }

        // check json data, some of them optional
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
                width, height, pos_x, pos_y, bit_rate));
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
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

    // register 'startVideoRecord' function
    auto name_startVideoRecord = std::string("startVideoRecord");
    err = wiltoncall_register(name_startVideoRecord.c_str(), static_cast<int> (name_startVideoRecord.length()),
            reinterpret_cast<void*> (video_handler::startVideoRecord), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'stopVideoRecord' function
    auto name_stopVideoRecord = std::string("stopVideoRecord");
    err = wiltoncall_register(name_stopVideoRecord.c_str(), static_cast<int> (name_stopVideoRecord.length()),
            reinterpret_cast<void*> (video_handler::stopVideoRecord), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'stopDisplayVideo' function
    auto name_stopDisplayVideo = std::string("stopDisplayVideo");
    err = wiltoncall_register(name_stopDisplayVideo.c_str(), static_cast<int> (name_stopDisplayVideo.length()),
            reinterpret_cast<void*> (video_handler::stopDisplayVideo), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'displayVideo' function
    auto name_displayVideo = std::string("displayVideo");
    err = wiltoncall_register(name_displayVideo.c_str(), static_cast<int> (name_displayVideo.length()),
            reinterpret_cast<void*> (video_handler::displayVideo), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'intiHandler' function
    auto name_intiHandler = std::string("intiHandler");
    err = wiltoncall_register(name_intiHandler.c_str(), static_cast<int> (name_intiHandler.length()),
            reinterpret_cast<void*> (video_handler::intiHandler), video_handler::vahandler_wrapper_init);
    if (nullptr != err) return err;

    // register 'makePhoto' function
    auto name_makePhoto = std::string("makePhoto");
    err = wiltoncall_register(name_makePhoto.c_str(), static_cast<int> (name_makePhoto.length()),
            reinterpret_cast<void*> (video_handler::makePhoto), video_handler::vahandler_wrapper);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
