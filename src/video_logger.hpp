/*
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

#ifndef VIDEO_LOGGER_HPP
#define VIDEO_LOGGER_HPP

extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h> // installed libavdevice-dev
}

#include <vector>
#include <stdio.h>
#include <stdarg.h>

#include <string>

#include <mutex>
#include <iostream>
#include <fstream>

//#include "wilton/wilton_logging.h"

#ifdef STATICLIB_WINDOWS
// #define UNICODE
// #define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else // !STATICLIB_WINDOWS
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#ifndef STATICLIB_MAC
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/types.h> 
#else // STATICLIB_MAC
#include <copyfile.h>
#endif // !STATICLIB_MAC
#endif // STATICLIB_WINDOWS


#include <memory>
#include <functional>

using logger_handler_type = std::function<void(void*, int, const char*, va_list)>;

namespace {
void cb_logger(void* ptr, int level, const char* fmt, va_list vargs);
void set_calback_logger(){
    av_log_set_callback(cb_logger);
}
} // anonymus namespace


class video_logger
{	
    video_logger(){}
    ~video_logger(){
        drop_callback_function();
    }
    video_logger(video_logger const&) = delete;
    video_logger& operator= (video_logger const&) = delete;
    std::string tmp_log_file;
public:
    static video_logger& instance()
    {
        // Not thread safe in vs2013
        static video_logger vl;
        return vl;
    }
    std::mutex mtx;
	
    void logger(void* ptr, int level, const char* fmt, va_list vargs) {
        std::lock_guard<std::mutex> lock(mtx);        
        std::ofstream file(tmp_log_file.c_str(), std::ios_base::app);
        if (!file.is_open()) return;

        va_list vl, nsvl;
        va_copy(vl, vargs);
        va_copy(nsvl, vargs);
        auto needed_size = vsnprintf(nullptr, 0, fmt, nsvl);
        std::vector<char> message(needed_size + 1);
        auto res = vsnprintf(message.data(), message.size(), fmt, vl);
        if (nullptr != ptr) {
            // https://www.ffmpeg.org/doxygen/2.4/log_8c_source.html#l00239
            AVClass* avc =  *(AVClass **) ptr;
            std::vector<char> prefix(256);
            auto len = snprintf(prefix.data(), prefix.size(), "[%s @ %p] ", avc->item_name(ptr), avc);
            file.write(prefix.data(), len);
        }
        file.write(message.data(), res);
        file.close();
	}
	void setup_callback_function(){
        if (!tmp_log_file.size()) return;
        std::lock_guard<std::mutex> lock(mtx);
        av_log_set_level(AV_LOG_TRACE);
        av_log_set_callback(cb_logger);
	}

    void setup_tmp_file(const std::string& tmp_file_path) {
        if (tmp_log_file.size()) return;
        tmp_log_file = tmp_file_path;
    }

	// https://stackoverflow.com/q/10195343/314015
	void copy_single_file(const std::string& from, const std::string& to) {
	#ifdef STATICLIB_WINDOWS
	    auto err = ::CopyFileA(from.c_str(), to.c_str(), false);
	    if (0 == err) {
	        throw tinydir_exception(TRACEMSG("Error copying file: [" + from + "]," +
	                " target: [" + to + "]" +
	                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
	    }
	#else // !STATICLIB_WINDOWS
	#ifndef STATICLIB_MAC
	    int source = ::open(from.c_str(), O_RDONLY, 0);
	    struct stat stat_source;
	    auto err_stat = ::fstat(source, std::addressof(stat_source));
	    int dest = ::open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, stat_source.st_mode);
	    auto err_sf = ::sendfile(dest, source, 0, stat_source.st_size);

	    (void) err_stat;
	    (void) dest;
	    (void) err_sf;

	    ::close(dest);
	    ::close(source);
	#else // STATICLIB_MAC
	    auto err_cf = ::copyfile(from.c_str(), to.c_str(), nullptr, COPYFILE_ALL);
	    if (0 != err_cf) throw tinydir_exception(TRACEMSG("Error copying file: [" + from + "]," +
	            " target: [" + to + "]" +
	            " error code: [" + sl::support::to_string(err_cf) + "]"));
	#endif // !STATICLIB_MAC
	#endif // STATICLIB_WINDOWS
	}

    void copy_logfile_to(const std::string& file_name) {
        if (!tmp_log_file.size()) return;
        std::string name = file_name;
        auto pos = name.rfind('.');
        name.replace(pos, name.size()-pos, ".log");
        std::lock_guard<std::mutex> lock(mtx);
        copy_single_file(tmp_log_file, name);
	}

    void clear_logfile(){
        if (!tmp_log_file.size()) return;
        remove(tmp_log_file.c_str());
        tmp_log_file.clear();
	}

	void drop_callback_function(){
        std::lock_guard<std::mutex> lock(mtx);
        av_log_set_callback(av_log_default_callback);
        av_log_set_level(AV_LOG_QUIET);
        clear_logfile();
	}
};

namespace {
void cb_logger(void* ptr, int level, const char* fmt, va_list vargs){
    video_logger& vl = video_logger::instance();
    vl.logger(ptr, level, fmt, vargs);
}
} // anonymus namespace


#endif  /* VIDEO_LOGGER_HPP */
