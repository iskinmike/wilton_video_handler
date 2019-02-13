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

#ifndef VIDEO_HANDLER_UTILS_HPP
#define VIDEO_HANDLER_UTILS_HPP

#include <string>
#include <vector>
extern "C" { // based on: https://stackoverflow.com/questions/24487203/ffmpeg-undefined-reference-to-avcodec-register-all-does-not-link
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace utils {

std::string construct_error(std::string what);

AVFrame* rescale_frame(AVFrame* frame, int new_width, int new_height, AVPixelFormat format, std::vector<uint8_t>& buffer);

class frame_rescaler
{
	SwsContext* sws_ctx;
	int width; 
	int height;
	AVPixelFormat format;
	std::vector<uint8_t> buffer;
public:
    frame_rescaler(int width, int height, AVPixelFormat format);
    void clear_sws_context();
    void rescale_frame_to_existed(AVFrame* frame, AVFrame* out_frame);
    AVFrame* rescale_frame(AVFrame* frame);
    ~frame_rescaler();
};

} // utils

#endif  /* VIDEO_HANDLER_UTILS_HPP */
