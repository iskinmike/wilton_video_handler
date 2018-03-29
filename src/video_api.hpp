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

#ifndef VIDEO_API_HPP
#define VIDEO_API_HPP

#include "decoder.hpp"
#include "encoder.hpp"
#include "display.hpp"
#include "photo.hpp"
#include <iostream>
#include <memory>

struct video_settings{
    std::string input_file, output_file, format, photo_name, title;
    int pos_x, pos_y;
    int width, height;
    int bit_rate;
};

class video_api
{
    std::shared_ptr<decoder> api_decoder;
    std::shared_ptr<encoder> api_encoder;
    std::shared_ptr<display> api_display;

    // settings
    video_settings settings;

public:
    video_api(video_settings set);
    ~video_api(){}
    std::string start_video_record();
    std::string start_video_display();
    void stop_video_record();
    void stop_video_display();
    std::string make_photo();
};

#endif  /* VIDEO_API_HPP */
