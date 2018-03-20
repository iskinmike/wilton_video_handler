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

#include "wilton/wiltoncall.h"
#include "video_api.h"

namespace example {

std::string hello(const std::string& input) {
    // arbitrary C++ code here
    return input + " from C++!";
}

std::string hello_again(const std::string& input) {
    // arbitrary C++ code here
    VideoAPI api("argv[1]", "argv[2]", "video4linux2", "CAM VIDEO");
    return input + " again from C++!";
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

    // return success
    return nullptr;
}
