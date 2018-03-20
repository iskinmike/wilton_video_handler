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

//#include "staticlib/config.hpp"
//#include "staticlib/io.hpp"
#include "staticlib/json.hpp"
//#include "staticlib/utils.hpp"
//#include "staticlib/support.hpp"

//#include "wilton/support/handle_registry.hpp"
//#include "wilton/support/buffer.hpp"
#include "wilton/support/exception.hpp"
//#include "wilton/support/registrar.hpp"

#include "wilton/wiltoncall.h"
#include "video_api.h"

//namespace wilton {
namespace example {

namespace { //anonymous

int genID(){
    static int id = 0;
    id++;
    return id;
}

std::map<int,std::shared_ptr<VideoAPI>> vhandlers_keeper;
}

std::string hello(const std::string& input) {
    // arbitrary C++ code here
    return input + " from C++!";
}

std::string hello_again(const std::string& input) {
    // arbitrary C++ code here
    VideoAPI api("argv[1]", "argv[2]", "video4linux2", "CAM VIDEO");
    return input + " again from C++!";
}


// Добавим функции для работы
int intiHandler(const std::string& in, const std::string& out,
                 const std::string& fmt, const std::string& title){
//    VideoAPI api(in.c_str(), out.c_str(), fmt.c_str(), title.c_str());
    int id = genID();
    vhandlers_keeper[id] =
            std::shared_ptr<VideoAPI> (new VideoAPI(in.c_str(), out.c_str(), fmt.c_str(), title.c_str()));
    return id;
}

// TODO: Добавить валидаци параметров, id например
std::string startVideoRecord(int id){
    auto result = std::string{};
    vhandlers_keeper[id]->startVideoRecord();
    return result;
}

std::string displayVideo(int id, int pos_x, int pos_y){
    auto result = std::string{};
    vhandlers_keeper[id]->startVideoDisplay(pos_x, pos_y);
    return result;
}

std::string stopVideoRecord(int id){
    auto result = std::string{};
    vhandlers_keeper[id]->stopVideoRecord();
    return result;
}

std::string stopDisplayVideo(int id){
    auto result = std::string{};
    vhandlers_keeper[id]->stopVideoDisplay();
    return result;
}

std::string makePhoto(int id, std::string& out){
    auto result = std::string{};
    vhandlers_keeper[id]->takePhoto(out);
    return result;
}

// helper function
char* wrapper_fun(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(const std::string&)> (ctx);
        auto input = std::string(data_in, static_cast<size_t>(data_in_len));
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


char* vahandler_wrapper_3int(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int, int, int)> (ctx);
        sl::json::value json = sl::json::loads(std::string(data_in, data_in_len));
        int id = 0;
        int pos_x = 0;
        int pos_y = 0;
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("id" == name) {
                id = std::stoi(fi.as_string_nonempty_or_throw());
            } else if ("x" == name) {
                pos_x = fi.as_int64_or_throw(name);
            } else if ("y" == name) {
                pos_y = fi.as_int64_or_throw(name);
            } else {
                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }

        std::string output = fun(id, pos_x, pos_y);
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
        auto what = std::string("CALL ERROR [vahandler_wrapper_3int]"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

char* vahandler_wrapper_init(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<int(*)(const std::string&, const std::string&, const std::string&, const std::string&)> (ctx);

        sl::json::value json = sl::json::loads(std::string(data_in, data_in_len));
        auto in = std::string{};
        auto out = std::string{};
        auto fmt = std::string{};
        auto title = std::string{};
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("in" == name) {
                in = fi.as_string_nonempty_or_throw(name);
            } else if ("out" == name) {
                out = fi.as_string_nonempty_or_throw(name);
            } else if ("fmt" == name) {
                fmt = fi.as_string_nonempty_or_throw(name);
            } else if ("title" == name) {
                title = fi.as_string_nonempty_or_throw(name);
            } else {
                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }

        std::string output = std::to_string(fun(in, out, fmt, title));
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

char* vahandler_wrapper_photo(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(int, const std::string&)> (ctx);
        sl::json::value json = sl::json::loads(std::string(data_in, data_in_len));
        int id = 0;
        auto out = std::string{};
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("id" == name) {
                id = std::stoi(fi.as_string_nonempty_or_throw());
            } else if ("out" == name) {
                out = fi.as_string_nonempty_or_throw(name);
            } else {
                throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }

        std::string output = fun(id, out);
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
        auto what = std::string("CALL ERROR [vahandler_wrapper_3int]"); // std::string(e.what());
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

    // register 'hello' function
    auto name_hello = std::string("example_hello");
    err = wiltoncall_register(name_hello.c_str(), static_cast<int> (name_hello.length()), 
            reinterpret_cast<void*> (example::hello), example::wrapper_fun);
    if (nullptr != err) return err;

    // register 'hello_again' function
    auto name_hello_again = std::string("example_hello_again");
    err = wiltoncall_register(name_hello_again.c_str(), static_cast<int> (name_hello_again.length()), 
            reinterpret_cast<void*> (example::hello_again), example::wrapper_fun);
    if (nullptr != err) return err;

    // register 'startVideoRecord' function
    auto name_startVideoRecord = std::string("startVideoRecord");
    err = wiltoncall_register(name_startVideoRecord.c_str(), static_cast<int> (name_startVideoRecord.length()),
            reinterpret_cast<void*> (example::startVideoRecord), example::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'stopVideoRecord' function
    auto name_stopVideoRecord = std::string("stopVideoRecord");
    err = wiltoncall_register(name_stopVideoRecord.c_str(), static_cast<int> (name_stopVideoRecord.length()),
            reinterpret_cast<void*> (example::stopVideoRecord), example::vahandler_wrapper);
    if (nullptr != err) return err;
    // register 'stopDisplayVideo' function
    auto name_stopDisplayVideo = std::string("stopDisplayVideo");
    err = wiltoncall_register(name_stopDisplayVideo.c_str(), static_cast<int> (name_stopDisplayVideo.length()),
            reinterpret_cast<void*> (example::stopDisplayVideo), example::vahandler_wrapper);
    if (nullptr != err) return err;

    // register 'displayVideo' function
    auto name_displayVideo = std::string("displayVideo");
    err = wiltoncall_register(name_displayVideo.c_str(), static_cast<int> (name_displayVideo.length()),
            reinterpret_cast<void*> (example::displayVideo), example::vahandler_wrapper_3int);
    if (nullptr != err) return err;


    // register 'intiHandler' function
    auto name_intiHandler = std::string("intiHandler");
    err = wiltoncall_register(name_intiHandler.c_str(), static_cast<int> (name_intiHandler.length()),
            reinterpret_cast<void*> (example::intiHandler), example::vahandler_wrapper_init);
    if (nullptr != err) return err;

    // register 'makePhoto' function
    auto name_makePhoto = std::string("makePhoto");
    err = wiltoncall_register(name_makePhoto.c_str(), static_cast<int> (name_makePhoto.length()),
            reinterpret_cast<void*> (example::makePhoto), example::vahandler_wrapper_photo);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
//}
